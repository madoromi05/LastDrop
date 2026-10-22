#include "GameSrc/Render/SkinnedMesh/AnimationPoseSampler.h"
#include "GameSrc/Render/SkinnedMesh/SkinnedMeshData.h"

#include "GameLib/GameLib.h"
#include "GameLib/Base/DebugStream.h"

using namespace GameLib;

AnimationPoseSampler::AnimationPoseSampler( const SkinnedMeshData* data, const char* animationName ) :
	mData( data ),
	mSkeleton( data->createSkeleton() ),
	mAnimation( data->animation( animationName ) ) {
	if ( !mAnimation ) {
		// 変換し忘れても落ちずに、そのアニメが無いだけにする。
		GameLib::cout << "AnimationPoseSampler: animation not found: " << animationName << GameLib::endl;
		return;
	}
	mSkeleton.setAnimation( mAnimation );
}

bool AnimationPoseSampler::isReady() const {
	return mAnimation ? true : false;
}

bool AnimationPoseSampler::sample( float frame, std::vector< Math::Matrix34 >* pose ) {
	ASSERT( pose && "AnimationPoseSampler::sample : pose is null." );
	if ( !isReady() ) {
		return false;
	}
	mSkeleton.updateAnimation( frame );
	// computeBonePose は骨の数だけ並んでいる前提で書き込む（大きさを変えない）ので、ここで揃える。
	pose->resize( mData->boneNumber() );
	mData->computeBonePose( mSkeleton, pose );
	return true;
}
