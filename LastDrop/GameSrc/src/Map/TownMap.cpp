#include "GameSrc/Map/TownMap.h"

#include <cmath>

namespace{
	// ── 道路キットの部品の大きさ（m）。tools/configs/road_*.json で、実寸のまま変換してある ──
	const float kTileLength = 5.70f;							// 直線の道路・歩道の帯・床の部品1枚の長さ（キット全体の単位）
	const float kRoadWidth = 7.71f;								// 車道の幅（二車線）
	const float kCurbWidth = 1.43f;								// 縁石つきの歩道の帯の幅
	const float kSidewalkHeight = 0.15f;						// 歩道の面の高さ（車道の面から）
	const float kStreetWidth = kRoadWidth + 2.f * kCurbWidth;	// 歩道の帯まで含めた道の幅。交差点は1辺がこの長さの正方形

	// ── 町の形 ──
	const int kColumnNumber = 4; // 南北に走る道の本数（両端は町の外周）
	const int kRowNumber = 4;    // 東西に走る道の本数（同上）
	// ブロック（歩道の帯に囲まれた内側）の1辺が部品何枚ぶんか。交差点と交差点の間に並べる直線の枚数でもある
	// 建物の元の足元の板（11.16m 四方。今は消した形を置く）が部品2枚ぶん（11.40m）に収まるので、4枚で建物 2×2 軒
	const int kBlockTileNumber = 4;
	const float kBlockSize = kBlockTileNumber * kTileLength;
	const float kRoadPitch = kStreetWidth + kBlockSize; // 隣の道までの間隔

	// ── 高さ・大きさ ──
	const float kRoadHeight = 0.f;
	const float kActualSize = 1.f; // 部品を実寸のまま置く拡大率
	// 町の外の土。1m 四方の板を広げて、道路の下に少し沈めて敷く（同じ高さだと道路とちらつく）
	const float kEarthSize = 400.f;
	const float kEarthDrop = 0.05f;

	// ── 向き（度）。部品の +X がどちらを向くか（+X は 90 度で北を向く）──
	// 建物は正面が +X、歩道の帯は歩道側が +X（車道側が -X）
	const float kFaceEast = 0.f;    // +X
	const float kFaceNorth = 90.f;  // -Z
	const float kFaceWest = 180.f;  // -X
	const float kFaceSouth = -90.f; // +Z
	// 直線の道路の部品は南北（z 方向）に走る向きで、横断歩道の部品は南北の道を横切る向き（x 方向に長い）で出来ている
	// どちらも 90 度回すと、東西の道用の向きになる
	const float kAsBuilt = 0.f;
	const float kQuarterTurn = 90.f;

	// ── 交差点。3×3 のますに分けて組み立てる（真ん中＝車道、辺の4ます、角の4ます）──
	// 辺のますは、道が続く辺なら横断歩道、続かない辺（町の外周）なら歩道の帯。これで十字・T字・角が同じ手順で出来る
	// 角のますは、両隣の辺とも道が続くなら丸い歩道、どちらかが続かないなら、続かない辺の歩道の帯をそのまま延ばす
	enum JunctionSide{
		SIDE_NORTH,
		SIDE_EAST,
		SIDE_SOUTH,
		SIDE_WEST,
		SIDE_NUM,
	};
	struct SideLayout{
		float directionX;   // 交差点の中心から見た辺の向き
		float directionZ;
		float curbYaw;      // 道が続かないときに置く歩道の帯の向き（歩道側が外を向く）
		float crosswalkYaw; // 道が続くときに置く横断歩道の向き
	};
	const SideLayout kSideLayouts[ SIDE_NUM ] = {
		{  0.f, -1.f, kFaceNorth, kAsBuilt     },
		{  1.f,  0.f, kFaceEast,  kQuarterTurn },
		{  0.f,  1.f, kFaceSouth, kAsBuilt     },
		{ -1.f,  0.f, kFaceWest,  kQuarterTurn },
	};
	// 角のます。はさむ2辺と、丸い歩道の向き（部品は、丸い側が -X・-Z（北西）を向く形で出来ている＝南東の角の向き）
	struct CornerLayout{
		JunctionSide sideA;
		JunctionSide sideB;
		float roundYaw;
	};
	const CornerLayout kCornerLayouts[] = {
		{ SIDE_NORTH, SIDE_EAST,   90.f },
		{ SIDE_EAST,  SIDE_SOUTH,   0.f },
		{ SIDE_SOUTH, SIDE_WEST,  -90.f },
		{ SIDE_WEST,  SIDE_NORTH, 180.f },
	};
	const float kJunctionCellOffset = 0.5f * ( kRoadWidth + kCurbWidth ); // 交差点の中心から、辺のますの中心まで

	// ブロックの中の建物4軒。ブロックの中心から見てどちら側か（-1 / +1）と、正面を向ける方角
	// 風車の形に4方向へ向けて、ブロックを囲むどの道にも入口が1つずつ面するようにする
	struct BlockLot{
		float sideX;
		float sideZ;
		float yawDegree;
	};
	const BlockLot kBlockLots[] = {
		{ -1.f, -1.f, kFaceNorth },
		{  1.f, -1.f, kFaceEast  },
		{  1.f,  1.f, kFaceSouth },
		{ -1.f,  1.f, kFaceWest  },
	};
	const float kLotOffset = 0.25f * kBlockSize; // ブロックの中心から建物の中心まで

	// ── 町を囲む建物（足元の板の無い形。tools/configs/building_noground.json）──
	// 外周の道の外側の歩道に正面の壁を合わせ、壁どうしが付く間隔で並べて、町の外へ出られなくする
	const float kBuildingBodyWidth = 7.83f;  // 建物の本体の幅（正面から見た左右。両端の柱まで）
	const float kBuildingFrontDepth = 3.94f; // 建物の中心から正面の壁まで
	// 囲む建物の足元に敷く床。足元の板が無いと、1階の引っ込んだ入口の奥に土が見えてしまうため
	// 床の部品を2倍に広げて使う（奥行きが入口の引っ込みより深ければ足り、広げた分だけ枚数が減る）
	const float kPerimeterFloorScale = 2.f;
	const float kPerimeterFloorSize = kPerimeterFloorScale * kTileLength;

	// 道の上の場所を、道の番号で書いたもの
	struct StreetSpot{
		float column;    // 南北に走る道の番号（西から 0。小数は道と道の間で、0.5 は 0 番と 1 番の真ん中）
		float row;       // 東西に走る道の番号（北から 0。同上）
		float offsetX;   // そこからのずれ（m）
		float offsetZ;
		float yawDegree; // キャラ・車の向き。0 で +Z（南）、90 で +X（東）を向く
	};

	// ── ゾンビ ──
	// 自キャラの始めの位置は、その日の脱出の車のそば（下の kEscapeCarSpots）
	// 歩くゾンビは手で置かない（見えない所へ ZombieSpawner が湧かせる。湧く場所の候補は道の本数から作る＝addZombieSpawnPoints）
	// ゾンビが湧く場所の候補を、交差点どうしの間のどこに置くか（0.5 で真ん中）
	const float kSpawnBetweenJunctions = 0.5f;
	const StreetSpot kTPoseZombie = { 1.5f, 1.f, 5.f, 1.8f, -90.f };	// T ポーズの1体は、町の中央の北側の通りから 5m 前・少し右に、西向きに置く

	// 町に置く車は、その日の脱出の車 1 台だけ（下の kEscapeCarSpots）
	// 乗れない置物の車は置かない（脱出できる車と見分けが付かないため）
	const float kLaneOffset = 0.25f * kRoadWidth; // 道の中心から車線の中心まで
	// ── ゴミ箱と木箱（置き場所は毎日同じ） ──
	// どれも車道の端（歩道の縁石の手前）に、長い辺を道に沿わせて置く。正面（+Z）は道の真ん中へ向ける
	// （北の端は 0・南の端は 180・西の端は 90・東の端は -90）。ずれの 3.1〜3.2m は、車道の幅の半分（3.86m）から箱の奥行きの半分を引いた所
	const float kCurbSide = 3.1f;  // ゴミ箱（奥行き約 1m）
	const float kCrateSide = 3.2f; // 木箱（奥行き約 0.6m）
	const StreetSpot kDumpsters[] = {
		{ 0.5f, 0.0f,  0.0f,      -kCurbSide,   0.f }, // 北の外周の道の西の区間
		{ 2.5f, 1.0f, -2.0f,       kCurbSide, 180.f }, // 東西の道 1 番の東の区間
		{ 1.0f, 2.5f, -kCurbSide,  3.0f,       90.f }, // 南北の道 1 番の南の区間
		{ 3.0f, 2.5f, -kCurbSide, -4.0f,       90.f }, // 東の外周の道の南の区間（町の内側の端）
		{ 0.0f, 0.5f,  kCurbSide,  2.0f,      -90.f }, // 西の外周の道の北の区間（町の内側の端）
		{ 1.5f, 2.0f,  3.0f,      -kCurbSide,   0.f }, // 東西の道 2 番の真ん中の区間
	};
	const StreetSpot kCrates[] = {
		{ 2.0f, 0.5f,  kCrateSide,  0.0f,       -90.f }, // 南北の道 2 番の北の区間
		{ 0.5f, 2.0f,  2.0f,        kCrateSide, 180.f }, // 東西の道 2 番の西の区間
		{ 2.5f, 3.0f,  0.0f,       -kCrateSide,   0.f }, // 南の外周の道の東の区間（町の内側の端）
		{ 1.0f, 1.5f,  kCrateSide,  0.0f,       -90.f }, // 南北の道 1 番の真ん中の区間
	};

	// ── 消火栓（見た目の置物。漁れない）──
	// 交差点のすぐ先の、南北の道の歩道の帯（縁石の上）に置く。交差点の真四角の中は丸い歩道や横断歩道なので避け、
	// 帯がまっすぐになる所（交差点の辺から少し先）に立てる。正面（+Z。大きい放水口の側）は車道へ向ける
	// 置く角は交差点ごとに変えて、同じ並びに見えないようにする。南北の道が外へ続かない側（外周の道の外）には置かない
	struct HydrantSpot{
		int column;   // 交差点の番号（南北に走る道。西から 0）
		int row;      // 交差点の番号（東西に走る道。北から 0）
		float sideX;  // 南北の道のどちら側の歩道か（-1 西・+1 東）
		float sideZ;  // 交差点のどちら側の先か（-1 北・+1 南）
	};
	const HydrantSpot kHydrants[] = {
		{ 1, 1,  1.f, -1.f }, // 中の十字路（北西）の北東の角
		{ 2, 1, -1.f,  1.f }, // 中の十字路（北東）の南西の角
		{ 1, 2, -1.f,  1.f }, // 中の十字路（南西）の南西の角
		{ 2, 2,  1.f, -1.f }, // 中の十字路（南東）の北東の角
		{ 0, 1,  1.f,  1.f }, // 西の外周の T 字路の南東の角
		{ 3, 2, -1.f, -1.f }, // 東の外周の T 字路の北西の角
	};
	const float kHydrantFromCurb = 0.45f;		// 縁石の外側の線（車道の端）から、消火栓の中心まで。奥行き約 0.32m なので、縁石から少し離れて帯の上に収まる
	const float kHydrantPastJunction = 0.8f;	// 交差点の辺（歩道の帯が始まる所）から、消火栓の中心まで
	const float kFaceRoadFromEast = -90.f;		// 東側の歩道から、車道（-X）へ向ける（StreetSpot の向きの決まり。90 で +X）
	const float kFaceRoadFromWest = 90.f;

	// ── 街灯（ランタン型の柱。光は柱の頭から） ──
	// 交差点ごとに 1 本と、交差点どうしの区間の真ん中に 1 本ずつ。どれも歩道の帯（縁石の上）に立てる
	// 交差点の柱は東西の道の歩道の、町の中心側の角に立てる（消火栓は南北の道の歩道なので重ならない）
	// 区間の柱は、外周の道なら町の内側の歩道、中の道なら区間ごとに左右を入れ替える（同じ側に並んで見えないように）
	const float kLampFromCurb = 0.5f;						// 縁石の線（車道の端）から柱の中心まで。台座の半径 約 0.26m が帯（1.43m）に収まる
	const float kLampPastJunction = 1.5f;					// 交差点の辺（歩道の帯が始まる所）から、交差点の柱の中心まで
	// ゴミ箱・木箱・消火栓の中心からこれより近ければ、柱を道に沿って kLampDodge だけ遠ざける（箱は車道の端・柱は歩道で、横に約 1.3m しか離れないため）
	const float kLampClearance = 3.f;
	const float kLampDodge = 4.f;
	const float kLampDarkCarDistance = 0.5f * kBlockSize;	// 脱出の車の置き場所からこれより近い区間の柱は立てない（「車は光なし」の決まり）。区間の長さの半分
	// 柱の向き（ランタンは柱の真上なので見た目だけ）。道に沿わせる
	const float kLampAlongEastWest = kAsBuilt;
	const float kLampAlongNorthSouth = kQuarterTurn;

	// 脱出の車の置き場所の候補（FuelEscapeLoading_Plan.txt Q13）
	// 町の反対側どうし＝北と南の外周の道の真ん中（交差点 1 番と 2 番の間）。毎日このうち 1 か所にだけ車を置く（RaidSession が選ぶ）
	// 車は道に沿わせて、町の内側の車線へ寄せる（道の中心はゾンビの湧く場所の候補なので、車体を掛けない）
	// 名前は画面に出す（「脱出の車：北」）。北は -Z（TownMap の向きの決まり）
	// 自キャラはその日の車の鼻先の前に、車の方を向いて立って始める（どれが脱出の車かすぐ分かるように）
	struct EscapeCarSpot{
		const char* name;
		StreetSpot spot;
		StreetSpot playerStart;
	};
	const float kSideMiddle = 1.5f; // 外周の道の、交差点 1 番と 2 番の真ん中
	const float kNorthRoad = 0.f;
	const float kSouthRoad = static_cast< float >( kRowNumber - 1 );
	const float kAlongRoad = 0.f;   // 道に沿った向きには動かさない
	const float kCarFacingEast = 90.f; // 車の向き（StreetSpot の決まり。90 で +X）
	const float kCarFacingWest = -90.f;
	// 車の中心から自キャラまで（車の向きの前へ。UAZ の長さ 約 4m の半分 ＋ 鼻先から約 4m 空けて、車の全体が視界に入るように）
	const float kStartAheadOfCar = 6.f;
	const EscapeCarSpot kEscapeCarSpots[] = {
		// 南側（町の内側）の車線。東を向く車の東に、西（車）を向いて立つ
		{ "北", { kSideMiddle, kNorthRoad, kAlongRoad,  kLaneOffset, kCarFacingEast },
		        { kSideMiddle, kNorthRoad, kStartAheadOfCar,  kLaneOffset, kCarFacingWest } },
		// 北側（町の内側）の車線。西を向く車の西に、東（車）を向いて立つ
		{ "南", { kSideMiddle, kSouthRoad, kAlongRoad, -kLaneOffset, kCarFacingWest },
		        { kSideMiddle, kSouthRoad, -kStartAheadOfCar, -kLaneOffset, kCarFacingEast } },
	};

	// 道の番号から、その道の中心の座標。町の中心が原点になるように振る
	float roadX( float column ) {
		return ( column - 0.5f * ( kColumnNumber - 1 ) ) * kRoadPitch;
	}

	float roadZ( float row ) {
		return ( row - 0.5f * ( kRowNumber - 1 ) ) * kRoadPitch;
	}

	// 町の中心から、外周の道の外側の歩道の端まで
	float townHalfSizeX() {
		return roadX( static_cast< float >( kColumnNumber - 1 ) ) + 0.5f * kStreetWidth;
	}

	float townHalfSizeZ() {
		return roadZ( static_cast< float >( kRowNumber - 1 ) ) + 0.5f * kStreetWidth;
	}

	TownPlacement toPlacement( const StreetSpot& spot ) {
		return { roadX( spot.column ) + spot.offsetX, kRoadHeight, roadZ( spot.row ) + spot.offsetZ, spot.yawDegree };
	}

	void addTile( std::vector< TownTile >* tiles, TownPiece piece, float x, float y, float z, float yawDegree, float scale ) {
		tiles->push_back( { piece, { x, y, z, yawDegree }, scale } );
	}

	// 交差点1つを、ますごとの部品で組み立てる。isOpen は、4辺それぞれの先に道が続くか（添え字は JunctionSide）
	void addJunction( std::vector< TownTile >* tiles, float centerX, float centerZ, const bool isOpen[ SIDE_NUM ] ) {
		addTile( tiles, TOWN_PIECE_JUNCTION_CENTER, centerX, kRoadHeight, centerZ, kAsBuilt, kActualSize );
		for ( int i = 0; i < SIDE_NUM; ++i ) {
			const SideLayout& side = kSideLayouts[ i ];
			float x = centerX + side.directionX * kJunctionCellOffset;
			float z = centerZ + side.directionZ * kJunctionCellOffset;
			if ( isOpen[ i ] ) {
				addTile( tiles, TOWN_PIECE_CROSSWALK, x, kRoadHeight, z, side.crosswalkYaw, kActualSize );
			} else {
				addTile( tiles, TOWN_PIECE_CURB_JUNCTION, x, kRoadHeight, z, side.curbYaw, kActualSize );
			}
		}
		for ( const CornerLayout& corner : kCornerLayouts ) {
			const SideLayout& sideA = kSideLayouts[ corner.sideA ];
			const SideLayout& sideB = kSideLayouts[ corner.sideB ];
			float x = centerX + ( sideA.directionX + sideB.directionX ) * kJunctionCellOffset;
			float z = centerZ + ( sideA.directionZ + sideB.directionZ ) * kJunctionCellOffset;
			if ( isOpen[ corner.sideA ] && isOpen[ corner.sideB ] ) {
				addTile( tiles, TOWN_PIECE_CORNER, x, kRoadHeight, z, corner.roundYaw, kActualSize );
				addTile( tiles, TOWN_PIECE_CORNER_FILL, x, kRoadHeight, z, kAsBuilt, kActualSize );
			} else {
				// 道が続かない辺の歩道の帯を、角のますまで延ばす（両方とも続かない町の四隅では、どちらの向きでも同じ）
				const SideLayout& closedSide = isOpen[ corner.sideA ] ? sideB : sideA;
				addTile( tiles, TOWN_PIECE_CURB_STUB, x, kRoadHeight, z, closedSide.curbYaw, kActualSize );
			}
		}
	}

	// 道と道が交わる所すべてに交差点を置く。町の外周では、外へ向かう辺を閉じる（T字・角になる）
	void addJunctions( std::vector< TownTile >* tiles ) {
		for ( int row = 0; row < kRowNumber; ++row ) {
			for ( int column = 0; column < kColumnNumber; ++column ) {
				bool isOpen[ SIDE_NUM ];
				isOpen[ SIDE_NORTH ] = row > 0;
				isOpen[ SIDE_EAST ] = column + 1 < kColumnNumber;
				isOpen[ SIDE_SOUTH ] = row + 1 < kRowNumber;
				isOpen[ SIDE_WEST ] = column > 0;
				addJunction( tiles, roadX( static_cast< float >( column ) ), roadZ( static_cast< float >( row ) ), isOpen );
			}
		}
	}

	// 交差点と交差点の間の直線と、その両側の歩道の帯
	void addStraights( std::vector< TownTile >* tiles ) {
		const float curbOffset = 0.5f * ( kRoadWidth + kCurbWidth ); // 道の中心から帯の中心まで
		// 東西に走る道
		for ( int row = 0; row < kRowNumber; ++row ) {
			float z = roadZ( static_cast< float >( row ) );
			for ( int column = 0; column + 1 < kColumnNumber; ++column ) {
				for ( int i = 0; i < kBlockTileNumber; ++i ) {
					float x = roadX( static_cast< float >( column ) ) + 0.5f * kStreetWidth + ( i + 0.5f ) * kTileLength;
					addTile( tiles, TOWN_PIECE_ROAD, x, kRoadHeight, z, kQuarterTurn, kActualSize );
					addTile( tiles, TOWN_PIECE_CURB, x, kRoadHeight, z - curbOffset, kFaceNorth, kActualSize );
					addTile( tiles, TOWN_PIECE_CURB, x, kRoadHeight, z + curbOffset, kFaceSouth, kActualSize );
				}
			}
		}
		// 南北に走る道
		for ( int column = 0; column < kColumnNumber; ++column ) {
			float x = roadX( static_cast< float >( column ) );
			for ( int row = 0; row + 1 < kRowNumber; ++row ) {
				for ( int i = 0; i < kBlockTileNumber; ++i ) {
					float z = roadZ( static_cast< float >( row ) ) + 0.5f * kStreetWidth + ( i + 0.5f ) * kTileLength;
					addTile( tiles, TOWN_PIECE_ROAD, x, kRoadHeight, z, kAsBuilt, kActualSize );
					addTile( tiles, TOWN_PIECE_CURB, x + curbOffset, kRoadHeight, z, kFaceEast, kActualSize );
					addTile( tiles, TOWN_PIECE_CURB, x - curbOffset, kRoadHeight, z, kFaceWest, kActualSize );
				}
			}
		}
	}

	// 道に囲まれたブロックごとに、歩道の高さの床1枚と、その上の建物を置く
	void addBlocks( std::vector< TownTile >* tiles, std::vector< TownPlacement >* buildings ) {
		for ( int row = 0; row + 1 < kRowNumber; ++row ) {
			for ( int column = 0; column + 1 < kColumnNumber; ++column ) {
				float centerX = roadX( column + 0.5f );
				float centerZ = roadZ( row + 0.5f );
				// 床は部品1枚をブロックの大きさへ広げる（枚数を並べるより描く数を減らす）。建物は足元の板の無い形なので、
				// 建物の間・引っ込んだ入口の奥にはこの床が見える
				addTile( tiles, TOWN_PIECE_PAVEMENT, centerX, kSidewalkHeight, centerZ, kAsBuilt, static_cast< float >( kBlockTileNumber ) );
				for ( const BlockLot& lot : kBlockLots ) {
					buildings->push_back( {
						centerX + lot.sideX * kLotOffset,
						kSidewalkHeight, // 壁の下の端を床（歩道の高さ）に合わせる。板が無いので浮かせなくてよい
						centerZ + lot.sideZ * kLotOffset,
						lot.yawDegree } );
				}
			}
		}
	}

	// 町を建物で囲む。辺ごとに、町の内側を向けた建物を、壁どうしが付く間隔で端から端まで並べる
	// 列は辺の長さ以上になる軒数にして真ん中に合わせるので、両端は町の角から少しはみ出す
	// はみ出した分は隣の辺の列の裏に隠れ、角にすき間が出来ない
	void addPerimeterBuildings( std::vector< TownPlacement >* buildings ) {
		const float halfX = townHalfSizeX();
		const float halfZ = townHalfSizeZ();
		const float height = kSidewalkHeight; // 正面の壁の下の端を、歩道の面に合わせる
		const int numberAlongX = static_cast< int >( std::ceil( 2.f * halfX / kBuildingBodyWidth ) );
		for ( int i = 0; i < numberAlongX; ++i ) {
			float x = ( i - 0.5f * ( numberAlongX - 1 ) ) * kBuildingBodyWidth;
			buildings->push_back( { x, height, -halfZ - kBuildingFrontDepth, kFaceSouth } );
			buildings->push_back( { x, height, halfZ + kBuildingFrontDepth, kFaceNorth } );
		}
		const int numberAlongZ = static_cast< int >( std::ceil( 2.f * halfZ / kBuildingBodyWidth ) );
		for ( int i = 0; i < numberAlongZ; ++i ) {
			float z = ( i - 0.5f * ( numberAlongZ - 1 ) ) * kBuildingBodyWidth;
			buildings->push_back( { -halfX - kBuildingFrontDepth, height, z, kFaceEast } );
			buildings->push_back( { halfX + kBuildingFrontDepth, height, z, kFaceWest } );
		}
	}

	// 囲む建物の足元に、歩道の高さの床を帯の形に敷く（町の4辺の外側）
	// 辺の長さが床の大きさで割り切れないので、4本の帯を風車の形に組む: 各辺とも片方の角から敷き始め、
	// 隣の辺の帯の横腹に突き当てる。余った長さは反対の角から町の外へはみ出す（建物の裏で見えない）
	// 突き合わせて敷くので帯どうしが重ならず、同じ高さの面がちらつかない
	void addPerimeterFloor( std::vector< TownTile >* tiles ) {
		const float halfX = townHalfSizeX();
		const float halfZ = townHalfSizeZ();
		const float size = kPerimeterFloorSize;
		const float centerLineX = halfX + 0.5f * size; // 東西の辺に沿う帯の中心線（町の中心からの距離）
		const float centerLineZ = halfZ + 0.5f * size;
		// 帯の長さは、辺の長さに、敷き終わりの側の角のます1つぶんを足したもの
		const int numberAlongX = static_cast< int >( std::ceil( ( 2.f * halfX + size ) / size ) );
		for ( int i = 0; i < numberAlongX; ++i ) {
			float offset = ( i + 0.5f ) * size;
			addTile( tiles, TOWN_PIECE_PAVEMENT, halfX - offset, kSidewalkHeight, -centerLineZ, kAsBuilt, kPerimeterFloorScale ); // 北の辺。東の角から西へ
			addTile( tiles, TOWN_PIECE_PAVEMENT, -halfX + offset, kSidewalkHeight, centerLineZ, kAsBuilt, kPerimeterFloorScale ); // 南の辺。西の角から東へ
		}
		const int numberAlongZ = static_cast< int >( std::ceil( ( 2.f * halfZ + size ) / size ) );
		for ( int i = 0; i < numberAlongZ; ++i ) {
			float offset = ( i + 0.5f ) * size;
			addTile( tiles, TOWN_PIECE_PAVEMENT, -centerLineX, kSidewalkHeight, -halfZ + offset, kAsBuilt, kPerimeterFloorScale ); // 西の辺。北の角から南へ
			addTile( tiles, TOWN_PIECE_PAVEMENT, centerLineX, kSidewalkHeight, halfZ - offset, kAsBuilt, kPerimeterFloorScale ); // 東の辺。南の角から北へ
		}
	}
}

namespace{
	// ゾンビが湧く場所の候補を、交差点と、東西・南北に隣り合う交差点の真ん中に置く（道の本数から作るので、町の形を変えても直さなくてよい）
	void addZombieSpawnPoints( std::vector< TownPlacement >* points ) {
		const float kNoOffset = 0.f;
		const float kNoYaw = 0.f; // 向きは ZombieSpawner が湧かせるときに決める
		for ( int row = 0; row < kRowNumber; ++row ) {
			for ( int column = 0; column < kColumnNumber; ++column ) {
				float x = static_cast< float >( column );
				float z = static_cast< float >( row );
				points->push_back( toPlacement( { x, z, kNoOffset, kNoOffset, kNoYaw } ) );
				if ( column + 1 < kColumnNumber ) {
					points->push_back( toPlacement( { x + kSpawnBetweenJunctions, z, kNoOffset, kNoOffset, kNoYaw } ) );
				}
				if ( row + 1 < kRowNumber ) {
					points->push_back( toPlacement( { x, z + kSpawnBetweenJunctions, kNoOffset, kNoOffset, kNoYaw } ) );
				}
			}
		}
	}


	// 消火栓の置き場所（足元は歩道の面の高さ）
	TownPlacement hydrantPlacement( const HydrantSpot& spot ) {
		float x = roadX( static_cast< float >( spot.column ) ) + spot.sideX * ( 0.5f * kRoadWidth + kHydrantFromCurb );
		float z = roadZ( static_cast< float >( spot.row ) ) + spot.sideZ * ( 0.5f * kStreetWidth + kHydrantPastJunction );
		float yaw = ( spot.sideX > 0.f ) ? kFaceRoadFromEast : kFaceRoadFromWest;
		return { x, kSidewalkHeight, z, yaw };
	}

	// 道の番号 index から見た、町の中心の側（+1 は番号が増える向き）。本数 count の真ん中より手前なら +1
	float towardCenter( int index, int count ) {
		return ( index < 0.5f * ( count - 1 ) ) ? 1.f : -1.f;
	}

	float squaredFlatDistance( const TownPlacement& a, const TownPlacement& b ) {
		float dx = a.x - b.x;
		float dz = a.z - b.z;
		return dx * dx + dz * dz;
	}

	// 柱 lamp が置物（props の中のどれか）に近すぎれば、道に沿って（alongX・alongZ は道の向きの単位ベクトル）置物から遠ざける
	TownPlacement dodgeProps( TownPlacement lamp, float alongX, float alongZ, const std::vector< const std::vector< TownPlacement >* >& props ) {
		const float clearance = kLampClearance * kLampClearance;
		for ( const std::vector< TownPlacement >* list : props ) {
			for ( const TownPlacement& prop : *list ) {
				if ( squaredFlatDistance( lamp, prop ) >= clearance ) {
					continue;
				}
				float along = ( lamp.x - prop.x ) * alongX + ( lamp.z - prop.z ) * alongZ;
				float away = ( along >= 0.f ) ? 1.f : -1.f; // 置物より先にあれば先へ、手前にあれば手前へ
				lamp.x += away * kLampDodge * alongX;
				lamp.z += away * kLampDodge * alongZ;
				return lamp; // 1 回ずらせば、区間の中の置物どうしは離れているので足りる
			}
		}
		return lamp;
	}

	bool nearEscapeCar( const TownPlacement& lamp, const std::vector< TownEscapeCarSpot >& cars ) {
		for ( const TownEscapeCarSpot& car : cars ) {
			if ( squaredFlatDistance( lamp, car.placement ) < kLampDarkCarDistance * kLampDarkCarDistance ) {
				return true;
			}
		}
		return false;
	}

	// 街灯の柱を並べる（足元は歩道の面の高さ）。props: 避ける置物の並び。cars: 脱出の車の置き場所（近くの区間には立てない）
	void addStreetLamps( std::vector< TownPlacement >* lamps, const std::vector< const std::vector< TownPlacement >* >& props,
	const std::vector< TownEscapeCarSpot >& cars ) {
		const float curbToLamp = 0.5f * kRoadWidth + kLampFromCurb;          // 道の中心から柱まで（道を横切る向き）
		const float junctionToLamp = 0.5f * kStreetWidth + kLampPastJunction; // 交差点の中心から柱まで（道に沿う向き）
		const float kEast = 1.f;
		const float kSouth = 1.f;
		const float kNone = 0.f;
		// 交差点の柱：東西の道の、町の中心側の角
		for ( int row = 0; row < kRowNumber; ++row ) {
			for ( int column = 0; column < kColumnNumber; ++column ) {
				TownPlacement lamp = {
					roadX( static_cast< float >( column ) ) + towardCenter( column, kColumnNumber ) * junctionToLamp,
					kSidewalkHeight,
					roadZ( static_cast< float >( row ) ) + towardCenter( row, kRowNumber ) * curbToLamp,
					kLampAlongEastWest };
				lamps->push_back( dodgeProps( lamp, kEast, kNone, props ) );
			}
		}
		// 区間の真ん中の柱。東西の道（row）の区間と、南北の道（column）の区間
		for ( int row = 0; row < kRowNumber; ++row ) {
			bool perimeter = ( row == 0 || row == kRowNumber - 1 );
			for ( int column = 0; column + 1 < kColumnNumber; ++column ) {
				float side = perimeter ? towardCenter( row, kRowNumber ) : ( ( ( row + column ) % 2 == 0 ) ? kSouth : -kSouth );
				TownPlacement lamp = { roadX( column + 0.5f ), kSidewalkHeight, roadZ( static_cast< float >( row ) ) + side * curbToLamp, kLampAlongEastWest };
				if ( !nearEscapeCar( lamp, cars ) ) {
					lamps->push_back( dodgeProps( lamp, kEast, kNone, props ) );
				}
			}
		}
		for ( int column = 0; column < kColumnNumber; ++column ) {
			bool perimeter = ( column == 0 || column == kColumnNumber - 1 );
			for ( int row = 0; row + 1 < kRowNumber; ++row ) {
				float side = perimeter ? towardCenter( column, kColumnNumber ) : ( ( ( row + column ) % 2 == 0 ) ? kEast : -kEast );
				TownPlacement lamp = { roadX( static_cast< float >( column ) ) + side * curbToLamp, kSidewalkHeight, roadZ( row + 0.5f ), kLampAlongNorthSouth };
				if ( !nearEscapeCar( lamp, cars ) ) {
					lamps->push_back( dodgeProps( lamp, kNone, kSouth, props ) );
				}
			}
		}
	}
}

TownMap::TownMap() :
	mTPoseZombie( toPlacement( kTPoseZombie ) ) {
	addTile( &mGroundTiles, TOWN_PIECE_EARTH, 0.f, -kEarthDrop, 0.f, kAsBuilt, kEarthSize );
	addJunctions( &mGroundTiles );
	addStraights( &mGroundTiles );
	addBlocks( &mGroundTiles, &mBuildings );
	addPerimeterBuildings( &mPerimeterBuildings );
	addPerimeterFloor( &mGroundTiles );

	addZombieSpawnPoints( &mZombieSpawnPoints );
	for ( const StreetSpot& spot : kDumpsters ) {
		mDumpsters.push_back( toPlacement( spot ) );
	}
	for ( const StreetSpot& spot : kCrates ) {
		mCrates.push_back( toPlacement( spot ) );
	}
	for ( const EscapeCarSpot& car : kEscapeCarSpots ) {
		mEscapeCarSpots.push_back( { car.name, toPlacement( car.spot ), toPlacement( car.playerStart ) } );
	}
	for ( const HydrantSpot& spot : kHydrants ) {
		mFireHydrants.push_back( hydrantPlacement( spot ) );
	}
	addStreetLamps( &mStreetLights, { &mDumpsters, &mCrates, &mFireHydrants }, mEscapeCarSpots );	// 街灯は、避ける置物（ゴミ箱・木箱・消火栓）と脱出の車の置き場所を並べ終えてから
}

const std::vector< TownTile >& TownMap::groundTiles() const {
	return mGroundTiles;
}

const std::vector< TownPlacement >& TownMap::buildings() const {
	return mBuildings;
}

const std::vector< TownPlacement >& TownMap::perimeterBuildings() const {
	return mPerimeterBuildings;
}

const std::vector< TownPlacement >& TownMap::zombieSpawnPoints() const {
	return mZombieSpawnPoints;
}

const TownPlacement& TownMap::tPoseZombie() const {
	return mTPoseZombie;
}

const std::vector< TownPlacement >& TownMap::dumpsters() const {
	return mDumpsters;
}

const std::vector< TownPlacement >& TownMap::crates() const {
	return mCrates;
}

const std::vector< TownEscapeCarSpot >& TownMap::escapeCarSpots() const {
	return mEscapeCarSpots;
}

const std::vector< TownPlacement >& TownMap::fireHydrants() const {
	return mFireHydrants;
}

const std::vector< GroundZone >& TownMap::groundZones() const {
	return mGroundZones;
}

const std::vector< TownPlacement >& TownMap::streetLights() const {
	return mStreetLights;
}
