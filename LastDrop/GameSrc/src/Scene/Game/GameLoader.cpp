#include "GameSrc/Scene/Game/GameLoader.h"
#include "GameSrc/Scene/Game/World/GameWorld.h"
#include "GameSrc/Render/MeshCache.h"

#include "GameLib/GameLib.h"

namespace{
	const int kStageCount = 5;	// 段階の数（Done は数えない）。形と絵を読む段階は、読み終えたファイルの割合で棒を進める
}

GameLoader::GameLoader( GameWorld* world, const MeshCache* meshes, int escapeCarSpot ) :
	mWorld( world ),
	mMeshes( meshes ),
	mEscapeCarSpot( escapeCarSpot ),
	mStage( Stage::Meshes ) {
	ASSERT( world && meshes && "GameLoader : an argument is null." );
}

bool GameLoader::advance( bool meshesReady ) {
	switch ( mStage ) {
	case Stage::Meshes:
		if ( meshesReady ) {
			mStage = Stage::Town; // 次の描画から組み立てる（読み終えた描画に、さらに重い段階を足さない）
		}
		break;
	case Stage::Town:
		mWorld->buildTown( mEscapeCarSpot );
		mStage = Stage::NavMesh;
		break;
	case Stage::NavMesh:
		mWorld->buildNavMesh();
		mStage = Stage::Visibility;
		break;
	case Stage::Visibility:
		mWorld->buildVisibility();
		mStage = Stage::Actors;
		break;
	case Stage::Actors:
		mWorld->placeActors( mEscapeCarSpot );
		mStage = Stage::Done;
		break;
	case Stage::Done:
		break;
	}
	return mStage == Stage::Done;
}

float GameLoader::progress() const {
	int stage = static_cast< int >( mStage );
	float within = ( mStage == Stage::Meshes ) ? mMeshes->progress() : 0.f;
	float ratio = ( static_cast< float >( stage ) + within ) / static_cast< float >( kStageCount );
	return ( ratio < 1.f ) ? ratio : 1.f;
}
