#include "GameSrc/Render/SkinnedMesh/SkinnedMeshInstance.h"
#include "GameSrc/Render/SkinnedMesh/SkinnedMeshData.h"
#include "GameSrc/Math/MatrixBlend.h"
#include "GameSrc/Render/SkinnedMesh/BonePoseModifier.h"
#include "GameSrc/Math/PlacementMatrix.h"
#include "GameSrc/Math/ScalarMath.h"

#include "GameLib/GameLib.h"
#include "GameLib/Base/DebugStream.h"
#include "GameLib/Scene/Animation.h"
#include "GameLib/Profiling/Profiler.h"

#include <cmath>

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const float kTimeStep = 1.f; // update 1回で進めるアニメ時刻（アニメのキー時刻はゲームのフレーム数）
	// PLAY_ONCE で止める時刻を、最後のキーからどれだけ手前にするか（フレーム）。
	// ちょうど最後のキーの時刻だと始めへ折り返してしまうが、見た目では区別できないほど小さくする。
	const float kLastFrameMargin = 0.001f;
}

SkinnedMeshInstance::SkinnedMeshInstance( const SkinnedMeshData* data ) :
	mData( data ),
	mTime( 0.f ),
	mShownTime( -kTimeStep ),
	mPlaybackRate( kTimeStep ),
	mPlayMode( PLAY_LOOP ),
	mPoseModifier( nullptr ),
	mBodyPoseModifier( nullptr ),
	mFadeFrames( 0.f ),
	mFadeElapsed( 0.f ),
	mHasPose( false ),
	mSkinStale( true ),
	mPosition( 0.f, 0.f, 0.f ),
	mAngleY( 0.f ) {
	ASSERT( mData && mData->isReady() && "SkinnedMeshInstance : data is null or not ready." );
	mSkeleton = mData->createSkeleton();
	mBonePose.resize( mData->boneNumber() );
	mShownPose.resize( mData->boneNumber() );
	mBodyPose.resize( mData->boneNumber() );
	mFadeFromPose.resize( mData->boneNumber() );
	mSkinMatrices.resize( mData->boneNumber() );
}

void SkinnedMeshInstance::setPosition( const Vector3& position ) {
	mPosition = position;
	mSkinStale = true;
}

void SkinnedMeshInstance::setAngleY( float degree ) {
	mAngleY = degree;
	mSkinStale = true;
}

void SkinnedMeshInstance::setAnimation( const char* name, float startTime, PlayMode mode ) {
	Scene::Animation animation = mData->animation( name );
	if ( !animation ) {
		// 名前違いでもエラーにならず静止したままになるので、ここで知らせる。
		GameLib::cout << "SkinnedMeshInstance: animation not found: " << name << GameLib::endl;
	}
	setAnimation( animation, startTime, mode );
}

void SkinnedMeshInstance::setAnimation( Scene::Animation animation, float startTime, PlayMode mode ) {
	mSkeleton.setAnimation( animation );
	mAnimation = animation;
	mTime = startTime; // 以降の update は、ここから1フレームずつ進む
	mShownTime = startTime - kTimeStep; // まだこのアニメを見せていない（update の前は、始める時刻の1つ前とみなす）
	mPlaybackRate = kTimeStep;
	mPlayMode = mode;
	mFadeFrames = 0.f;
}

void SkinnedMeshInstance::crossFadeTo( Scene::Animation animation, float startTime, float fadeFrames, PlayMode mode ) {
	// 混ぜる元は「切り替え直前に実際に見えていたポーズ」を写し取ったもの。
	// 前のアニメを動かし続けて混ぜる方法だと、フェード中にもう一度切り替えたとき（歩き始めてすぐ止まる等）
	// 混ざっていた分が一瞬で消えてポーズが飛ぶ。見えていたポーズから始めれば、何度切り替えても飛ばない。
	bool canFade = mHasPose && fadeFrames > 0.f;
	if ( canFade ) {
		mFadeFromPose = mBonePose;
	}
	setAnimation( animation, startTime, mode );
	if ( canFade ) {
		mFadeFrames = fadeFrames;
		mFadeElapsed = 0.f;
	}
}

float SkinnedMeshInstance::animationPhase() const {
	float period = mAnimation.period();
	if ( period <= 0.f ) {
		return 0.f;
	}
	float phase = std::fmod( mShownTime, period ) / period;
	return ( phase < 0.f ) ? phase + 1.f : phase;
}

void SkinnedMeshInstance::setPoseModifier( BonePoseModifier* modifier ) {
	mPoseModifier = modifier;
}

void SkinnedMeshInstance::setBodyPoseModifier( BonePoseModifier* modifier ) {
	mBodyPoseModifier = modifier;
}

const std::vector< Matrix34 >& SkinnedMeshInstance::bodyPose() const {
	return mBodyPoseModifier ? mBodyPose : mBonePose;
}

bool SkinnedMeshInstance::isAnimationFinished() const {
	return mPlayMode == PLAY_ONCE && mShownTime >= lastFrameTime();
}

float SkinnedMeshInstance::lastFrameTime() const {
	return mAnimation ? mAnimation.period() - kLastFrameMargin : 0.f;
}

float SkinnedMeshInstance::advanceClock() {
	// 1回だけのアニメは、最後のフレームを超えたらそこで止める（時刻を戻すので、止まった後も増え続けない）。
	if ( mPlayMode == PLAY_ONCE && mTime > lastFrameTime() ) {
		mTime = lastFrameTime();
	}
	mShownTime = mTime;
	mTime += mPlaybackRate;
	if ( mFadeFrames > 0.f ) {
		mFadeElapsed += kTimeStep;
		if ( mFadeElapsed >= mFadeFrames ) {
			mFadeFrames = 0.f; // フェード終了。今のアニメのポーズをそのまま使う
		}
	}
	return mShownTime;
}

void SkinnedMeshInstance::advance() {
	advanceClock();
}

void SkinnedMeshInstance::update() {
	float time = advanceClock();
	PROFILE_COUNT( "Anim.poses", 1 ); // アニメの姿勢を求めた回数（OptimizationB_Plan.txt 5 章）
	mSkeleton.updateAnimation( time );
	mData->computeBonePose( mSkeleton, &mBonePose );
	blendFadingPose();
	mHasPose = true;
	mSkinStale = true;

	// 体の書き換え（当たり判定も動く）→ 見た目だけの書き換え、の順に掛ける。
	if ( mBodyPoseModifier ) {
		mBodyPose = mBonePose;
		mBodyPoseModifier->apply( &mBodyPose );
	}
	mShownPose = bodyPose();
	if ( mPoseModifier ) {
		mPoseModifier->apply( &mShownPose );
	}
}

Matrix34 SkinnedMeshInstance::rootMatrix() const {
	return PlacementMatrix::make( mPosition, mAngleY );
}

Vector3 SkinnedMeshInstance::toRootDirection( const Vector3& worldDirection ) const {
	Matrix34 worldToRoot;
	worldToRoot.setInverse( rootMatrix() );
	Vector3 rootDirection;
	worldToRoot.mul33( &rootDirection, worldDirection );
	return rootDirection;
}

Vector3 SkinnedMeshInstance::toRootPoint( const Vector3& worldPoint ) const {
	Matrix34 worldToRoot;
	worldToRoot.setInverse( rootMatrix() );
	Vector3 rootPoint;
	worldToRoot.mul( &rootPoint, worldPoint );
	return rootPoint;
}

bool SkinnedMeshInstance::boneMatrix( int boneIndex, Matrix34* out ) const {
	if ( !mHasPose || boneIndex < 0 || boneIndex >= static_cast< int >( mShownPose.size() ) ) {
		return false;
	}
	// 頂点の変形（computeSkinMatrices）と同じ「置き場所 * ボーンの姿勢」。同じ式なので、付けた物が手からずれない。
	out->setMul( rootMatrix(), mShownPose[ boneIndex ] );
	return true;
}

bool SkinnedMeshInstance::animatedBoneMatrix( int boneIndex, Matrix34* out ) const {
	const std::vector< Matrix34 >& pose = bodyPose();
	if ( !mHasPose || boneIndex < 0 || boneIndex >= static_cast< int >( pose.size() ) ) {
		return false;
	}
	out->setMul( rootMatrix(), pose[ boneIndex ] );
	return true;
}

void SkinnedMeshInstance::shownWorldPose( std::vector< Matrix34 >* out ) const {
	ASSERT( out && "SkinnedMeshInstance : out is null." );
	int boneNumber = mData->boneNumber();
	out->resize( boneNumber );
	for ( int b = 0; b < boneNumber; ++b ) {
		if ( !boneMatrix( b, &( *out )[ b ] ) ) {
			( *out )[ b ] = rootMatrix(); // まだ update していない（置いた直後）。根の位置に置く
		}
	}
}

void SkinnedMeshInstance::blendFadingPose() {
	if ( mFadeFrames <= 0.f ) {
		return;
	}
	float t = mFadeElapsed / mFadeFrames; // 進みは advanceClock が数える（終われば mFadeFrames が 0 になっている）
	// 混ぜる割合を、始めと終わりがゆっくりになる曲線（smoothstep）にする。
	// 直線だと、フェードの始まりと終わりで動きの速さがカクッと変わって見えるため。
	float weight = ScalarMath::smoothStep( t );
	for ( size_t b = 0; b < mBonePose.size(); ++b ) {
		MatrixBlend::blend( mFadeFromPose[ b ], mBonePose[ b ], weight, &mBonePose[ b ] );
	}
}

void SkinnedMeshInstance::draw() const {
	if ( !mHasPose ) {
		return; // まだ update していない（見せるポーズが無い）
	}
	// 骨の行列は、ポーズか置き場所が変わった後に初めて描く時だけ作る。G-Buffer と影（最大 4 枚）で同じ体を何度描いても 1 回で済む
	// （OptimizationB_Plan.txt 8 章）。置き場所と向きは、混ぜて書き換えた後のポーズにまとめて掛ける。
	if ( mSkinStale ) {
		PROFILE_COUNT( "Skin.matrices", 1 ); // 骨の行列を作った回数（OptimizationB_Plan.txt 5 章）
		mData->computeSkinMatrices( rootMatrix(), mShownPose, &mSkinMatrices );
		mSkinStale = false;
	}
	mData->draw( mSkinMatrices );
}

void SkinnedMeshInstance::setPlaybackRate( float rate ) {
	ASSERT( rate > 0.f && "SkinnedMeshInstance : playback rate must be positive." );
	mPlaybackRate = kTimeStep * rate;
}

float SkinnedMeshInstance::animationTime() const {
	return mShownTime;
}
