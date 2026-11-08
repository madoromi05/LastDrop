#include "GameSrc/Zombie/Animation/ZombieRootMotion.h"
#include "GameSrc/Render/SkinnedMesh/SkinnedMeshData.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Matrix34.h"
#include "GameLib/Math/Functions.h"
#include "GameLib/Scene/Tree.h"

#include <cmath>

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// 最後のフレームを計算する時刻を、アニメの長さからこれだけ手前にする（フレーム）。
	// GameLib のアニメは最後のキーの時刻ちょうどで始めへ折り返すので、その少し手前の姿勢を最後の値にする（SkinnedMeshInstance と同じ）。
	const float kLastFrameMargin = 0.01f;
}

ZombieRootMotion::ZombieRootMotion( const SkinnedMeshData* data, Scene::Animation animation, int rootBone ) {
	ASSERT( data && "ZombieRootMotion : data is null." );
	if ( !animation || rootBone < 0 || animation.period() <= 0.f ) {
		return;
	}
	float period = animation.period();
	int frameNumber = static_cast< int >( std::ceil( period ) );
	Scene::Tree skeleton = data->createSkeleton();
	skeleton.setAnimation( animation );
	std::vector< Matrix34 > pose( data->boneNumber() );
	mOffsets.resize( frameNumber );
	for ( int f = 0; f < frameNumber; ++f ) {
		float time = Math::min( static_cast< float >( f ), period - kLastFrameMargin );
		skeleton.updateAnimation( time );
		data->computeBonePose( skeleton, &pose );
		const Matrix34& root = pose[ rootBone ];
		mOffsets[ f ].set( root.m03, 0.f, root.m23 );
	}
}

bool ZombieRootMotion::isValid() const {
	return !mOffsets.empty();
}

Vector3 ZombieRootMotion::offsetAt( float time ) const {
	ASSERT( isValid() && "ZombieRootMotion : no table." );
	int last = static_cast< int >( mOffsets.size() ) - 1;
	float t = Math::max( 0.f, Math::min( time, static_cast< float >( last ) ) );
	int index = static_cast< int >( t );
	if ( index >= last ) {
		return mOffsets[ last ];
	}
	float fraction = t - static_cast< float >( index );
	Vector3 offset;
	offset.setSub( mOffsets[ index + 1 ], mOffsets[ index ] );
	offset *= fraction;
	offset += mOffsets[ index ];
	return offset;
}

float ZombieRootMotion::farthestDistance() const {
	float farthest = 0.f;
	for ( const Vector3& offset : mOffsets ) {
		Vector3 fromStart;
		fromStart.setSub( offset, mOffsets.front() );
		farthest = Math::max( farthest, fromStart.length() );
	}
	return farthest;
}
