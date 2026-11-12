#ifndef INCLUDED_TOWN_MAP_H
#define INCLUDED_TOWN_MAP_H

#include "GameSrc/Audio/GroundMaterial.h"
#include <vector>

// 置き場所と向き
struct TownPlacement{
	float x;
	float y; // 高さ（車道の面が 0）
	float z;
	float yawDegree; // 上から見て反時計回りが正
};

// 地面に並べる部品の種類（道路キットから変換した物と、町の外の土）
enum TownPiece{
	TOWN_PIECE_ROAD,			// 二車線の道路の直線（1枚ぶん）
	TOWN_PIECE_JUNCTION_CENTER,	// 交差点の真ん中（線の無い車道）
	TOWN_PIECE_CROSSWALK,		// 横断歩道（交差点の、道が続く辺）
	TOWN_PIECE_CURB,			// 縁石つきの歩道の帯（道路1枚ぶんの長さ）
	TOWN_PIECE_CURB_JUNCTION,	// 同じ帯を車道の幅の長さにした物（交差点の、道が続かない辺）
	TOWN_PIECE_CURB_STUB,		// 同じ帯を帯の幅の長さにした物（交差点の角で、隣の帯を延ばす）
	TOWN_PIECE_CORNER,			// 交差点の角の丸い歩道
	TOWN_PIECE_CORNER_FILL,		// 丸い歩道の下に敷く車道
	TOWN_PIECE_PAVEMENT,		// ブロックの内側の床
	TOWN_PIECE_EARTH,			// 町の外の土
	TOWN_PIECE_NUM,
};

// 地面の部品1枚の置き方
struct TownTile{
	TownPiece piece;
	TownPlacement placement;
	float scale; // 部品の拡大率（1 で実寸）
};

// 脱出の車を置く候補 1 つ（FuelEscapeLoading_Plan.txt Q13）。名前は画面に出す（「脱出の車：北」）。向きは車の向き
// 自キャラはその日の車のそばから始める（車の鼻先の前に立ち、車の方を向く）
struct TownEscapeCarSpot{
	const char* name;
	TownPlacement placement;
	TownPlacement playerStart; // この車の日の、自キャラの始めの位置と向き
};

// 町のどこに何を置くかを決める1か所（道路・歩道・建物・自キャラ・ゾンビ・車・ガソリンの候補・床の材質）
// 置き場所を決めるだけで、物は作らない（作って並べるのはシーン）。町の形を変えるときは、このクラスの .cpp の定数だけを直す
//
// 町は碁盤の目: 二車線の道を東西・南北に等間隔に通し、道に囲まれたブロックに建物を 2×2 軒ずつ置く
// 外周の道は外へ向かう辺を閉じ（交差点は T字・角になる）、その外側を足元の板の無い建物で隙間なく囲んで、町の外へ出られなくする
// 道路の部品の並びは、道の本数と部品の大きさから組み立てる（1枚ずつ手で書くと、本数を変えたときに直し漏れるため）
// 道の上に置く物（自キャラ・車など）は、座標ではなく道の番号で持つ（町の形を変えても道の上に乗り続けるように）
//
// 高さ: 車道の面が 0、歩道の面はそれより高い。キャラは高さ 0 のまま動くので、歩道の上では足が歩道に沈んで見える
// （段差に乗る仕組みがまだ無いため）
class TownMap{
public:
	TownMap();

	const std::vector< TownTile >& groundTiles() const;				// 道路・歩道・床・土（どれも移動は止めない。弾は TownGround が止める）
	const std::vector< TownPlacement >& buildings() const;			// ブロックの中の建物（向きは、正面がどちらを向くか）
	const std::vector< TownPlacement >& perimeterBuildings() const;	// 町を囲む建物（足元の板の無い形を置く。向きは同上）
	// ゾンビが湧くかもしれない場所（交差点と、隣り合う交差点の真ん中。道の中心。向きは使わない）
	// どこに湧かせるかは ZombieSpawner が、この中から選ぶ
	const std::vector< TownPlacement >& zombieSpawnPoints() const;
	const TownPlacement& tPoseZombie() const;						// 当たり判定の確認用に T ポーズで立たせる1体
	// 漁れる場所（置き場所は毎日同じで、中身だけ毎日変わる。中身を決めるのは WorldBuilder）
	// 地面にはガソリンとライフルを置かない（ガソリンはゴミ箱・木箱の中、ライフルは木箱から出る）
	const std::vector< TownPlacement >& dumpsters() const;			// ゴミ箱（車道の端。向きは、正面〈+Z〉が道の真ん中を向く）
	const std::vector< TownPlacement >& crates() const;				// 木箱（同上）
	// 脱出の車を置く候補（北と南の外周の道の真ん中の車線）と、その日の自キャラの始めの位置。毎日どれに置くかは RaidSession が選ぶ
	const std::vector< TownEscapeCarSpot >& escapeCarSpots() const;
	// 消火栓（見た目の置物。交差点のすぐ先の歩道。足元は歩道の面の高さ。向きは、正面〈+Z〉が車道を向く）
	const std::vector< TownPlacement >& fireHydrants() const;
	const std::vector< GroundZone >& groundZones() const;			// 床の材質を地面の部品と変える所（水たまりなど）。今は置いていない
	// 街灯の柱の置き場所（足元は歩道の面の高さ。向きは道に沿う）。交差点ごとに 1 本と区間の真ん中
	// 柱は WorldBuilder が置き、光は柱の頭（足元 + LightingParams::streetLightHeight）からシーンが出す。届く距離・色・強さも LightingParams
	const std::vector< TownPlacement >& streetLights() const;

private:
	std::vector< TownTile > mGroundTiles;
	std::vector< TownPlacement > mBuildings;
	std::vector< TownPlacement > mPerimeterBuildings;
	std::vector< TownPlacement > mZombieSpawnPoints;
	TownPlacement mTPoseZombie;
	std::vector< TownPlacement > mDumpsters;
	std::vector< TownPlacement > mCrates;
	std::vector< TownEscapeCarSpot > mEscapeCarSpots;
	std::vector< TownPlacement > mFireHydrants;
	std::vector< GroundZone > mGroundZones;
	std::vector< TownPlacement > mStreetLights;
};

#endif
