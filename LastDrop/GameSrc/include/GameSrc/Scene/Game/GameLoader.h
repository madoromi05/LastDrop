#ifndef INCLUDED_GAME_LOADER_H
#define INCLUDED_GAME_LOADER_H

class GameWorld;
class MeshCache;

// 出撃のロードを段階に分けて、1 回の描画に 1 段階ずつ進める（FuelEscapeLoading_Plan.txt 6 章）
// 1 つの描画で全部を済ませると、Debug では 1 フレームが 9 秒を超えて Windows が窓を「応答なし」にし、画面も止まって見えた
// 段階に分ければ、段階の間に窓のメッセージが回り、ロード画面（LoadingScreen）も描き直せる
// ここが持つのは段階の順番と「どこまで済んだか」だけ。中身は GameWorld と MeshCache、描くのは LoadingScreen
// 順番：形と絵を読む → 町を組み立てる → ゾンビの歩ける床 → 建物の隠れの表 → 自キャラとゾンビ（ゾンビは歩ける床を借りるので最後）
class GameLoader{
public:
	// world: 段階ごとに組み立てる世界。meshes: 読み込みの進み具合を聞く倉庫。どちらも借りるだけ（nullptr 不可）
	// escapeCarSpot: 今日の脱出の車の置き場所（RaidSession::escapeCarSpot）
	GameLoader( GameWorld* world, const MeshCache* meshes, int escapeCarSpot );
	GameLoader( const GameLoader& ) = delete;
	GameLoader& operator=( const GameLoader& ) = delete;

	// 1 段階ぶん進める（1 回の描画に 1 回）。meshesReady: この描画の MeshCache::update が全部済んだと答えたか
	// 全部の段階が済んだら true（それ以降は何もせず true）
	bool advance( bool meshesReady );
	float progress() const;			// 全体の進み具合（0〜1。ロード画面の棒）
	const char* stageLabel() const;	// 次に行う段階の名前（ロード画面に出す。日本語）

private:
	enum class Stage{
		Meshes,
		Town,
		NavMesh,
		Visibility,
		Actors,
		Done,
	};

	GameWorld* mWorld;
	const MeshCache* mMeshes;
	int mEscapeCarSpot;
	Stage mStage;
};

#endif
