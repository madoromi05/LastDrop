#ifndef INCLUDED_LOADING_PHASE_H
#define INCLUDED_LOADING_PHASE_H

#include "GameSrc/Scene/Game/GameLoader.h"
#include "GameSrc/Scene/LoadingScreen.h"

class GameWorld;
class MeshCache;
class UiRenderer;

// 出撃のロード段階（メッシュの組み立て→世界を並べる）を受け持つ。終わるまで draw は世界を描かずロード画面だけ
// 2 回目以降（もう一度・Retry の出撃）はもう構築済みですぐ ready。完了したフレームに GameScene が後処理（弾倉の復元・カーソルのロック）を 1 回行う
class LoadingPhase{
public:
	// world/meshes/ui: 借りるだけ（所有しない）。escapeCarSpot: 今日の脱出の車の置き場所（RaidSession が選ぶ）。どれも nullptr 不可
	LoadingPhase( GameWorld* world, const MeshCache* meshes, int escapeCarSpot, UiRenderer* ui );
	bool isReady() const { return mReady; }
	// 1 段階進める（isReady でない間だけ呼ぶ）。meshesReady: テクスチャまで読み終えたか（GameResources::meshes().update）
	// 完了したフレームだけ true を返す（呼び出し側がそのとき後処理を行う）。まだなら false（この描画はロード画面だけ）
	bool advance( bool meshesReady );
	// ロード中の画面を描く（世界は描かない）。heading: 見出しに出す文字（今回の条件の名前）。goal: 見出しの下の目的の文
	void drawLoadingScreen( const char* heading, const char* goal );

private:
	GameLoader mLoader;
	LoadingScreen mLoadingScreen;
	bool mReady; // 世界の配置が済んだか
};

#endif
