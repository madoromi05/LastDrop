#include "GameSrc/Render/SkinnedMesh/SkinnedMeshInstance.h"
#include "GameSrc/Render/SkinnedMesh/SkinnedMeshData.h"
#include "GameSrc/Math/MatrixBlend.h"
#include "GameSrc/Render/SkinnedMesh/BonePoseModifier.h"
#include "GameSrc/Math/PlacementMatrix.h"

#include "GameLib/GameLib.h"
#include "GameLib/Base/DebugStream.h"
#include "GameLib/Scene/Animation.h"

#include <cmath>

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const float kTimeStep = 1.f; // update 1回で進めるアニメ時刻（アニメのキー時刻はゲームのフレーム数）
	// PLAY_ONCE で止める時刻を、最後のキーからどれだけ手前にするか（フレーム）。
	// ちょうど最後のキーの時刻だと始めへ折り返してしまうが、見た目では区別できないほど小さくする。
	const float kLastFrameMargin = 0.001f;

	// 混ぜる割合を、始めと終わりがゆっくりになる曲線（smoothstep）にする。
	// 直線だと、フェードの始まりと終わりで動きの速さがカクッと変わって見えるため。
	float easeInOut( float t ) {
		return t * t * ( 3.f - 2.f * t );
	}
}

SkinnedMeshInstance::SkinnedMeshInstance( const SkinnedMeshData* data ) :
	mData( data ),
	mTime( 0.f ),
	mShownTime( -kTimeStep ),
	mPlaybackRate( kTimeStep ),
	mPlayMode( PLAY_LOOP ),
	mPoseModifier( nullptr ),
	mFadeFrames( 0.f ),
	mFadeElapsed( 0.f ),
	mHasPose( false ),
	mPosition( 0.f, 0.f, 0.f ),
	mAngleY( 0.f ) {
	ASSERT( mData && mData->isReady() && "SkinnedMeshInstance : data is null or not ready." );
	mSkeleton = mData->createSkeleton();
	mBonePose.resize( mData->boneNumber() );
	mShownPose.resize( mData->boneNumber() );
	mFadeFromPose.resize( mData->boneNumber() );
	mSkinMatrices.resize( mData->boneNumber() );
}

void SkinnedMeshInstance::setPosition( const Vector3& position ) {
	mPosition = position;
}

void SkinnedMeshInstance::setAngleY( float degree ) {
	mAngleY = degree;
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

bool SkinnedMeshInstance::isAnimationFinished() const {
	return mPlayMode == PLAY_ONCE && mShownTime >= lastFrameTime();
}

float SkinnedMeshInstance::lastFrameTime() const {
	return mAnimation ? mAnimation.period() - kLastFrameMargin : 0.f;
}

void SkinnedMeshInstance::update() {
	// 1回だけのアニメは、最後のフレームを超えたらそこで止める（時刻を戻すので、止まった後も増え続けない）。
	if ( mPlayMode == PLAY_ONCE && mTime > lastFrameTime() ) {
		mTime = lastFrameTime();
	}
	mSkeleton.updateAnimation( mTime );
	mShownTime = mTime;
	mTime += mPlaybackRate;
	mData->computeBonePose( mSkeleton, &mBonePose );
	blendFadingPose();
	mHasPose = true;

	mShownPose = mBonePose;
	if ( mPoseModifier ) {
		mPoseModifier->apply( &mShownPose );
	}
}

Matrix34 SkinnedMeshInstance::rootMatrix() const {
	return PlacementMatrix::make( mPosition, mAngleY );
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
	if ( !mHasPose || boneIndex < 0 || boneIndex >= static_cast< int >( mBonePose.size() ) ) {
		return false;
	}
	out->setMul( rootMatrix(), mBonePose[ boneIndex ] );
	return true;
}

void SkinnedMeshInstance::blendFadingPose() {
	if ( mFadeFrames <= 0.f ) {
		return;
	}
	mFadeElapsed += kTimeStep;
	float t = mFadeElapsed / mFadeFrames;
	if ( t >= 1.f ) {
		mFadeFrames = 0.f; // フェード終了。今のアニメのポーズをそのまま使う
		return;
	}
	float weight = easeInOut( t );
	for ( size_t b = 0; b < mBonePose.size(); ++b ) {
		MatrixBlend::blend( mFadeFromPose[ b ], mBonePose[ b ], weight, &mBonePose[ b ] );
	}
}

void SkinnedMeshInstance::draw() const {
	if ( !mHasPose ) {
		return; // まだ update していない（見せるポーズが無い）
	}
	// 置き場所と向きは、混ぜて書き換えた後のポーズにまとめて掛ける。
	mData->computeSkinMatrices( rootMatrix(), mShownPose, &mSkinMatrices );
	mData->draw( mSkinMatrices );
}

void SkinnedMeshInstance::setPlaybackRate( float rate ) {
	ASSERT( rate > 0.f && "SkinnedMeshInstance : playback rate must be positive." );
	mPlaybackRate = kTimeStep * rate;
}

float SkinnedMeshInstance::animationTime() const {
	return mShownTime;
}
