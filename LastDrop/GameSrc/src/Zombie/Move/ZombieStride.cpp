#include "GameSrc/Zombie/Move/ZombieStride.h"
#include "GameSrc/Render/SkinnedMesh/SkinnedMeshData.h"
#include "GameSrc/Math/FlatMath.h"

#include "GameLib/GameLib.h"
#include "GameLib/Base/DebugStream.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix34.h"
#include "GameLib/Scene/Tree.h"

#include <cmath>

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// 前後のこのフレーム数ずつで平らにする。左右の足が入れ替わる所で、どちらが地面についているかが揺れて
	// 1フレームだけ大きく・小さくなるのをならす（5フレーム＝約 0.08 秒。歩き方の波は残る長さ）。
	const int kSmoothRadius = 2;

	// アニメ時刻 time の足先の位置（骨格の根の空間＝置き場所と向きを掛ける前。単位は m）。
	void toePositions( const SkinnedMeshData* data, Scene::Tree* skeleton, std::vector< Matrix34 >* pose, float time,
	int leftToe, int rightToe, Vector3* left, Vector3* right ) {
		skeleton->updateAnimation( time );
		data->computeBonePose( *skeleton, pose );
		const Matrix34& l = ( *pose )[ leftToe ];
		const Matrix34& r = ( *pose )[ rightToe ];
		left->set( l.m03, l.m13, l.m23 );
		right->set( r.m03, r.m13, r.m23 );
	}
}

ZombieStride::ZombieStride( const SkinnedMeshData* data, Scene::Animation walk, int leftToe, int rightToe ) :
	mPeriod( 0.f ) {
	ASSERT( data && "ZombieStride : data is null." );
	if ( !walk || leftToe < 0 || rightToe < 0 || walk.period() <= 0.f ) {
		GameLib::cout << "ZombieStride: no animation or toe bones. No stride table." << GameLib::endl;
		return;
	}
	mPeriod = walk.period();
	int frameNumber = static_cast< int >( std::ceil( mPeriod ) );
	Scene::Tree skeleton = data->createSkeleton();
	skeleton.setAnimation( walk );
	std::vector< Matrix34 > pose( data->boneNumber() );

	// 1フレームごとに、地面についている足（低い方の足先）が水平に動いた長さ。
	// 向きは見ない（足は後ろへしか動かないので、長さだけで足りる）。
	std::vector< float > raw( frameNumber );
	for ( int f = 0; f < frameNumber; ++f ) {
		Vector3 leftNow, rightNow, leftNext, rightNext;
		toePositions( data, &skeleton, &pose, static_cast< float >( f ), leftToe, rightToe, &leftNow, &rightNow );
		toePositions( data, &skeleton, &pose, std::fmod( static_cast< float >( f + 1 ), mPeriod ), leftToe, rightToe, &leftNext, &rightNext );
		bool leftPlanted = ( leftNow.y + leftNext.y ) <= ( rightNow.y + rightNext.y );
		raw[ f ] = leftPlanted ? FlatMath::distance( leftNow, leftNext ) : FlatMath::distance( rightNow, rightNext );
	}
	mDistances.resize( frameNumber );
	for ( int f = 0; f < frameNumber; ++f ) {
		float sum = 0.f;
		for ( int k = -kSmoothRadius; k <= kSmoothRadius; ++k ) {
			sum += raw[ ( f + k + frameNumber ) % frameNumber ];
		}
		mDistances[ f ] = sum / static_cast< float >( kSmoothRadius * 2 + 1 );
	}
	GameLib::cout << "ZombieStride: " << cycleLength() << " m per " << mPeriod << " frames" << GameLib::endl;
}

bool ZombieStride::isValid() const {
	return !mDistances.empty();
}

float ZombieStride::distanceAt( float time ) const {
	ASSERT( isValid() && "ZombieStride : no table." );
	float t = std::fmod( time, mPeriod );
	if ( t < 0.f ) {
		t += mPeriod;
	}
	int frameNumber = static_cast< int >( mDistances.size() );
	int index = static_cast< int >( t );
	float fraction = t - static_cast< float >( index );
	float a = mDistances[ index % frameNumber ];
	float b = mDistances[ ( index + 1 ) % frameNumber ];
	return a + ( b - a ) * fraction;
}

float ZombieStride::cycleLength() const {
	float sum = 0.f;
	for ( float distance : mDistances ) {
		sum += distance;
	}
	return sum;
}
