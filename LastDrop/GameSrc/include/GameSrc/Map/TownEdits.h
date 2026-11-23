#ifndef INCLUDED_TOWN_EDITS_H
#define INCLUDED_TOWN_EDITS_H

#include "GameSrc/Map/TownMap.h"
#include <string>
#include <vector>

// 手直しできる物の種類。ファイルの kind の綴りは townEditKindName と同じ
enum class TownEditKind{
	Building,
	PerimeterBuilding,
	Dumpster,
	Crate,
	FireHydrant,
	StreetLight,
	ZombieSpawn,
	EscapeCar,
	PlayerStart,
	Count,
};

const char* townEditKindName( TownEditKind kind );					// ファイルの kind の綴り
bool townEditKindFromName( const char* name, TownEditKind* out );	// 綴りから種類を求める。知らない綴りなら false で out は変えない
bool townEditKindCanAddOrDelete( TownEditKind kind );				// 足す・消すができる種類か（脱出の車と自キャラの始めの位置は数を変えないので false）

// 手直しの対象の名札。TownMap が組み立てたときの元の座標で、物を見分ける（0.05m 以内を同じ物とみなす）
struct TownEditKey{
	TownEditKind kind;
	float fromX;
	float fromZ;
};

// 消しすぎを止める下限。漁れる場所とゾンビの湧く候補が、使う側の必要な数を下回らないようにする
struct TownEditLimits{
	int minLootSpots;		// ゴミ箱と木箱の数の合計の下限
	int minCrates;			// 木箱の数の下限
	int minZombieSpawns;	// ゾンビの湧く候補の数の下限
};

// 今の調整値（ガソリン・ライフルの数）と日の条件（一番多いゾンビの数）から下限を求める
TownEditLimits townEditLimits();

// 下限と比べる数。どれも、その種類の物が町にいくつあるか
struct TownEditCounts{
	int lootSpots;		// ゴミ箱と木箱の数の合計
	int crates;			// 木箱の数
	int zombieSpawns;	// ゾンビの湧く候補の数
};

// TownMap の並びへのポインタの束。使わない並びは nullptr のままにする（その種類の手直しは当たらず警告になる）
struct TownMapLists{
	std::vector< TownPlacement >* buildings;
	std::vector< TownPlacement >* perimeterBuildings;
	std::vector< TownPlacement >* dumpsters;
	std::vector< TownPlacement >* crates;
	std::vector< TownPlacement >* fireHydrants;
	std::vector< TownPlacement >* streetLights;
	std::vector< TownPlacement >* zombieSpawnPoints;
	std::vector< TownEscapeCarSpot >* escapeCarSpots;
};

// 町の手直しの中身。直した物だけを Move・Delete・Add の並びで持ち、TownMap が組み立てた並びへ当てる
// ファイルの読み書きは持たない。当てる順は Delete → Move → Add で、名札は当てる前の並びの元の座標で探す
// 当たらない・2 つ以上当たる・同じ物へ 2 回・消した物への Move・下限を割る Delete は、その行を使わず警告に返す
class TownEdits{
public:
	struct Move{
		TownEditKey key;
		TownPlacement to;
	};
	struct Addition{
		TownEditKind kind;
		TownPlacement at;
		unsigned id;	// add で振る、この手直しの中で変わらない番号。removeAdded で添字が詰まっても、同じ足した物を指し続けるため
	};

	// 並びの最後へ足す。同じ名札の行は上書きせず並ぶ（重複はファイルの読み込みで見つけるため）。置き換えるなら先に revert する
	void move( const TownEditKey& key, const TownPlacement& to );
	void remove( const TownEditKey& key );
	int add( TownEditKind kind, const TownPlacement& at );	// 足した物の番号（additions() の添字）を返す
	void removeAdded( int index );							// 足した物を取り消す（後ろの番号は 1 つ詰まる）
	void moveAdded( int index, const TownPlacement& to );
	void revert( const TownEditKey& key );					// その名札の Move と Delete を全部消す
	void clear();
	bool isEmpty() const;
	unsigned revision() const;	// 中身を変えるたびに増える数。F6 が、手直しが変わったか（床と表の状態を求め直すか）を見分ける

	const TownPlacement* movedTo( const TownEditKey& key ) const;	// 最初に見つかった Move の行き先。無ければ nullptr
	bool isRemoved( const TownEditKey& key ) const;
	const std::vector< Move >& moves() const;
	const std::vector< TownEditKey >& removals() const;
	const std::vector< Addition >& additions() const;
	// 組み立てたままの数 original から、消す行の分を引いた数で、kind をもう 1 つ消しても limits を割らないか（apply と同じ数え方）
	// 名札が当たるかは見ない（当たらない行も引くので、厳しい側に倒れる）。足した物は数えない（apply は足す前に消す数を確かめるため）
	bool canRemoveOneMore( TownEditKind kind, const TownEditCounts& original, const TownEditLimits& limits ) const;

	// lists へ手直しを当てる。使わなかった行ごとに 1 つ、理由の文を返す
	// 文は英字で「Move Crate (8.20, -16.69): no object matches the key」の形。F6 の Town タブとログに出す
	std::vector< std::string > apply( TownMapLists* lists, const TownEditLimits& limits ) const;

private:
	std::vector< Move > mMoves;
	std::vector< TownEditKey > mRemovals;
	std::vector< Addition > mAdditions;
	unsigned mRevision = 0;
	unsigned mNextAdditionId = 0;	// clear でも戻さない（読み直す前の足した物と、同じ番号にしないため）
};

#endif
