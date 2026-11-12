#include "GameSrc/Scene/Game/LoadingPhase.h"

LoadingPhase::LoadingPhase( GameWorld* world, const MeshCache* meshes, int escapeCarSpot, UiRenderer* ui ) :
	mLoader( world, meshes, escapeCarSpot ),
	mLoadingScreen( ui ),
	mReady( false ) {
}

bool LoadingPhase::advance( bool meshesReady ) {
	// ロードは 1 回の描画に 1 段階ずつ（間にロード画面を描き、窓のメッセージを回す）
	if ( !mLoader.advance( meshesReady ) ) {
		return false; // まだロード中
	}
	mReady = true;
	return true; // このフレームで完了（呼び出し側が後処理を 1 回行う）
}

void LoadingPhase::drawLoadingScreen( const char* heading ) {
	mLoadingScreen.draw( heading, mLoader.stageLabel(), mLoader.progress() );
}
