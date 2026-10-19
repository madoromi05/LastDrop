#include "GameSrc/Map/TownMap.h"

#include <cmath>

namespace{
	// ── 道路キットの部品の大きさ（m）。tools/configs/road_*.json で、実寸のまま変換してある ──
	const float kTileLength = 5.70f;     // 直線の道路・歩道の帯・床の部品1枚の長さ（キット全体の単位）
	const float kRoadWidth = 7.71f;      // 車道の幅（二車線）
	const float kCurbWidth = 1.43f;      // 縁石つきの歩道の帯の幅
	const float kSidewalkHeight = 0.15f; // 歩道の面の高さ（車道の面から）
	// 歩道の帯まで含めた道の幅。交差点は1辺がこの長さの正方形。
	const float kStreetWidth = kRoadWidth + 2.f * kCurbWidth;

	// ── 町の形 ──
	const int kColumnNumber = 4; // 南北に走る道の本数（両端は町の外周）
	const int kRowNumber = 4;    // 東西に走る道の本数（同上）
	// ブロック（歩道の帯に囲まれた内側）の1辺が部品何枚ぶんか。交差点と交差点の間に並べる直線の枚数でもある。
	// 建物の足元の板（11.16m 四方）が部品2枚ぶん（11.40m）に収まるので、4枚で建物 2×2 軒。
	const int kBlockTileNumber = 4;
	const float kBlockSize = kBlockTileNumber * kTileLength;
	const float kRoadPitch = kStreetWidth + kBlockSize; // 隣の道までの間隔

	// ── 高さ・大きさ ──
	const float kRoadHeight = 0.f;
	const float kActualSize = 1.f; // 部品を実寸のまま置く拡大率
	// 建物の足元の板を、床の部品より少し浮かせる（同じ高さに重ねると面がちらつく）。
	const float kBuildingLift = 0.03f;
	// 町の外の土。1m 四方の板を広げて、道路の下に少し沈めて敷く（同じ高さだと道路とちらつく）。
	const float kEarthSize = 400.f;
	const float kEarthDrop = 0.05f;

	// ── 向き（度）。部品の +X がどちらを向くか（+X は 90 度で北を向く）──
	// 建物は正面が +X、歩道の帯は歩道側が +X（車道側が -X）。
	const float kFaceEast = 0.f;    // +X
	const float kFaceNorth = 90.f;  // -Z
	const float kFaceWest = 180.f;  // -X
	const float kFaceSouth = -90.f; // +Z
	// 直線の道路の部品は南北（z 方向）に走る向きで、横断歩道の部品は南北の道を横切る向き（x 方向に長い）で出来ている。
	// どちらも 90 度回すと、東西の道用の向きになる。
	const float kAsBuilt = 0.f;
	const float kQuarterTurn = 90.f;

	// ── 交差点。3×3 のますに分けて組み立てる（真ん中＝車道、辺の4ます、角の4ます）──
	// 辺のますは、道が続く辺なら横断歩道、続かない辺（町の外周）なら歩道の帯。これで十字・T字・角が同じ手順で出来る。
	// 角のますは、両隣の辺とも道が続くなら丸い歩道、どちらかが続かないなら、続かない辺の歩道の帯をそのまま延ばす。
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
	// 角のます。はさむ2辺と、丸い歩道の向き（部品は、丸い側が -X・-Z（北西）を向く形で出来ている＝南東の角の向き）。
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

	// ブロックの中の建物4軒。ブロックの中心から見てどちら側か（-1 / +1）と、正面を向ける方角。
	// 風車の形に4方向へ向けて、ブロックを囲むどの道にも入口が1つずつ面するようにする。
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
	// 外周の道の外側の歩道に正面の壁を合わせ、壁どうしが付く間隔で並べて、町の外へ出られなくする。
	const float kBuildingBodyWidth = 7.83f;  // 建物の本体の幅（正面から見た左右。両端の柱まで）
	const float kBuildingFrontDepth = 3.94f; // 建物の中心から正面の壁まで
	// 囲む建物の足元に敷く床。足元の板が無いと、1階の引っ込んだ入口の奥に土が見えてしまうため。
	// 床の部品を2倍に広げて使う（奥行きが入口の引っ込みより深ければ足り、広げた分だけ枚数が減る）。
	const float kPerimeterFloorScale = 2.f;
	const float kPerimeterFloorSize = kPerimeterFloorScale * kTileLength;

	// 道の上の場所を、道の番号で書いたもの。
	struct StreetSpot{
		float column;    // 南北に走る道の番号（西から 0。小数は道と道の間で、0.5 は 0 番と 1 番の真ん中）
		float row;       // 東西に走る道の番号（北から 0。同上）
		float offsetX;   // そこからのずれ（m）
		float offsetZ;
		float yawDegree; // キャラ・車の向き。0 で +Z（南）、90 で +X（東）を向く
	};

	// ── 自キャラとゾンビ。町の中央の北側の通り（東西の道 1 番）の真ん中から、東を向いて始める ──
	// 脱出用の車と同じ通りから始める（ユーザー指定: 車と始めの位置は近くに）。
	const StreetSpot kPlayerStart = { 1.5f, 1.f, 0.f, 0.f, 90.f };
	// 自キャラに気付くと歩いて追ってくるゾンビ。始めの視界の奥（東の交差点の手前）に、こちら向きに置く。
	const StreetSpot kWalkingZombies[] = {
		{ 1.5f, 1.f, 11.0f, -1.2f,  -90.f },
		{ 1.5f, 1.f, 13.5f,  1.3f, -115.f },
		{ 1.5f, 1.f, 16.0f, -1.7f,  -70.f },
	};
	// T ポーズの1体は、始めの位置から 5m 前・少し右（始めの視界に収まる所）に、こちら向きに置く。
	const StreetSpot kTPoseZombie = { 1.5f, 1.f, 5.f, 1.8f, -90.f };

	// ── ゲームループ（ガソリンを集めて車で脱出）──
	// 脱出用の車は、始めの位置の左前に、道に沿って北側の車線へ寄せて停める（南側の車線には T ポーズのゾンビが居るため）。
	// 前後の位置は、始めの位置にも奥の歩くゾンビにも車体が掛からない所。
	const float kLaneOffset = 0.25f * kRoadWidth; // 道の中心から車線の中心まで
	const StreetSpot kEscapeCar = { 1.5f, 1.f, 4.5f, -kLaneOffset, 90.f };
	// 何度でも拾えるガソリン（ユーザー指定）。車の後ろ（始めの位置の側）の同じ車線に、車体から少し離して置く。
	const StreetSpot kEndlessFuel = { 1.5f, 1.f, 1.6f, -kLaneOffset, 20.f };
	// ガソリンが出るかもしれない場所（10か所）。町じゅうを歩かせるよう、外周の四隅と各辺の真ん中、内側の通りに散らす。
	// どれも車道の上（建物・ゾンビ・車・始めの位置と重ならない所）。ずれは、道の真ん中ではなく端に寄せるため。
	const StreetSpot kFuelSpawnPoints[] = {
		{ 0.0f, 0.0f,  1.0f,  1.5f,   0.f }, // 北西の角
		{ 1.5f, 0.0f, -2.0f, -1.5f,  40.f }, // 北の辺
		{ 3.0f, 0.0f, -1.0f,  1.5f, -20.f }, // 北東の角
		{ 0.0f, 1.5f,  1.0f,  3.0f,  70.f }, // 西の辺
		{ 3.0f, 1.5f, -1.0f, -3.0f,  10.f }, // 東の辺
		{ 0.0f, 3.0f,  1.0f, -1.5f, -45.f }, // 南西の角
		{ 1.5f, 3.0f,  2.0f,  1.5f,  90.f }, // 南の辺
		{ 3.0f, 3.0f, -1.0f, -1.5f,  15.f }, // 南東の角
		{ 0.5f, 1.0f,  0.0f, -2.0f, -60.f }, // 内側（北西）
		{ 2.5f, 2.0f,  0.0f,  2.0f,  30.f }, // 内側（南東）
	};

	// 道の番号から、その道の中心の座標。町の中心が原点になるように振る。
	float roadX( float column ) {
		return ( column - 0.5f * ( kColumnNumber - 1 ) ) * kRoadPitch;
	}

	float roadZ( float row ) {
		return ( row - 0.5f * ( kRowNumber - 1 ) ) * kRoadPitch;
	}

	// 町の中心から、外周の道の外側の歩道の端まで。
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

	// 交差点1つを、ますごとの部品で組み立てる。isOpen は、4辺それぞれの先に道が続くか（添え字は JunctionSide）。
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
				// 道が続かない辺の歩道の帯を、角のますまで延ばす（両方とも続かない町の四隅では、どちらの向きでも同じ）。
				const SideLayout& closedSide = isOpen[ corner.sideA ] ? sideB : sideA;
				addTile( tiles, TOWN_PIECE_CURB_STUB, x, kRoadHeight, z, closedSide.curbYaw, kActualSize );
			}
		}
	}

	// 道と道が交わる所すべてに交差点を置く。町の外周では、外へ向かう辺を閉じる（T字・角になる）。
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

	// 交差点と交差点の間の直線と、その両側の歩道の帯。
	void addStraights( std::vector< TownTile >* tiles ) {
		const float curbOffset = 0.5f * ( kRoadWidth + kCurbWidth ); // 道の中心から帯の中心まで
		// 東西に走る道。
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
		// 南北に走る道。
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

	// 道に囲まれたブロックごとに、歩道の高さの床1枚と、その上の建物を置く。
	void addBlocks( std::vector< TownTile >* tiles, std::vector< TownPlacement >* buildings ) {
		for ( int row = 0; row + 1 < kRowNumber; ++row ) {
			for ( int column = 0; column + 1 < kColumnNumber; ++column ) {
				float centerX = roadX( column + 0.5f );
				float centerZ = roadZ( row + 0.5f );
				// 床は部品1枚をブロックの大きさへ広げる（ほとんどが建物の足元の板に隠れるので、枚数を並べるより描く数を減らす）。
				addTile( tiles, TOWN_PIECE_PAVEMENT, centerX, kSidewalkHeight, centerZ, kAsBuilt, static_cast< float >( kBlockTileNumber ) );
				for ( const BlockLot& lot : kBlockLots ) {
					buildings->push_back( {
						centerX + lot.sideX * kLotOffset,
						kSidewalkHeight + kBuildingLift,
						centerZ + lot.sideZ * kLotOffset,
						lot.yawDegree } );
				}
			}
		}
	}

	// 町を建物で囲む。辺ごとに、町の内側を向けた建物を、壁どうしが付く間隔で端から端まで並べる。
	// 列は辺の長さ以上になる軒数にして真ん中に合わせるので、両端は町の角から少しはみ出す。
	// はみ出した分は隣の辺の列の裏に隠れ、角にすき間が出来ない。
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

	// 囲む建物の足元に、歩道の高さの床を帯の形に敷く（町の4辺の外側）。
	// 辺の長さが床の大きさで割り切れないので、4本の帯を風車の形に組む: 各辺とも片方の角から敷き始め、
	// 隣の辺の帯の横腹に突き当てる。余った長さは反対の角から町の外へはみ出す（建物の裏で見えない）。
	// 突き合わせて敷くので帯どうしが重ならず、同じ高さの面がちらつかない。
	void addPerimeterFloor( std::vector< TownTile >* tiles ) {
		const float halfX = townHalfSizeX();
		const float halfZ = townHalfSizeZ();
		const float size = kPerimeterFloorSize;
		const float centerLineX = halfX + 0.5f * size; // 東西の辺に沿う帯の中心線（町の中心からの距離）
		const float centerLineZ = halfZ + 0.5f * size;
		// 帯の長さは、辺の長さに、敷き終わりの側の角のます1つぶんを足したもの。
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

TownMap::TownMap() :
	mPlayerStart( toPlacement( kPlayerStart ) ),
	mTPoseZombie( toPlacement( kTPoseZombie ) ),
	mEscapeCar( toPlacement( kEscapeCar ) ),
	mEndlessFuel( toPlacement( kEndlessFuel ) ) {
	addTile( &mGroundTiles, TOWN_PIECE_EARTH, 0.f, -kEarthDrop, 0.f, kAsBuilt, kEarthSize );
	addJunctions( &mGroundTiles );
	addStraights( &mGroundTiles );
	addBlocks( &mGroundTiles, &mBuildings );
	addPerimeterBuildings( &mPerimeterBuildings );
	addPerimeterFloor( &mGroundTiles );

	for ( const StreetSpot& spot : kWalkingZombies ) {
		mWalkingZombies.push_back( toPlacement( spot ) );
	}
	for ( const StreetSpot& spot : kFuelSpawnPoints ) {
		mFuelSpawnPoints.push_back( toPlacement( spot ) );
	}
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

const TownPlacement& TownMap::playerStart() const {
	return mPlayerStart;
}

const std::vector< TownPlacement >& TownMap::walkingZombies() const {
	return mWalkingZombies;
}

const TownPlacement& TownMap::tPoseZombie() const {
	return mTPoseZombie;
}

const TownPlacement& TownMap::escapeCar() const {
	return mEscapeCar;
}

const TownPlacement& TownMap::endlessFuel() const {
	return mEndlessFuel;
}

const std::vector< TownPlacement >& TownMap::fuelSpawnPoints() const {
	return mFuelSpawnPoints;
}

const std::vector< GroundZone >& TownMap::groundZones() const {
	return mGroundZones;
}
