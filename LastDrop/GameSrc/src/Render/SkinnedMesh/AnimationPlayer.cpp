#include "GameSrc/Render/SkinnedMesh/AnimationPlayer.h"
#include "GameSrc/Render/SkinnedMesh/SkinnedMeshData.h"
#include "GameSrc/Math/MatrixBlend.h"
#include "GameSrc/Math/ScalarMath.h"

#include "GameLib/GameLib.h"
#include "GameLib/Base/DebugStream.h"
#include "GameLib/Profiling/Profiler.h"

#include <cmath>

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const float kTimeStep = 1.f; // update 1回で進めるアニメ時刻（アニメのキー時刻はゲームのフレーム数）
	// PLAY_ONCE で止める時刻を、最後のキーからどれだけ手前にするか（フレーム）
	// ちょうど最後のキーの時刻だと始めへ折り返してしまうが、見た目では区別できないほど小さくする
	const float kLastFrameMargin = 0.001f;
}

AnimationPlayer::AnimationPlayer( const SkinnedMeshData* data ) :
	mData( data ),
	mTime( 0.f ),
	mShownTime( -kTimeStep ),
	mPlaybackRate( kTimeStep ),
	mPlayMode( PLAY_LOOP ),
	mFadeFrames( 0.f ),
	mFadeElapsed( 0.f ),
	mHasPose( false ) {
	ASSERT( mData && mData->isReady() && "AnimationPlayer : data is null or not ready." );
	mSkeleton = mData->createSkeleton();
	mBonePose.resize( mData->boneNumber() );
	mFadeFromPose.resize( mData->boneNumber() );
}

void AnimationPlayer::setAnimation( const char* name, float startTime, PlayMode mode ) {
	Scene::Animation animation = mData->animation( name );
	if ( !animation ) {
		GameLib::cout << "SkinnedMeshInstance: animation not found: " << name << GameLib::endl;	// 名前違いでもエラーにならず静止したままになるので、ここで知らせる
	}
	setAnimation( animation, startTime, mode );
}

void AnimationPlayer::setAnimation( Scene::Animation animation, float startTime, PlayMode mode ) {
	mSkeleton.setAnimation( animation );
	mAnimation = animation;
	mTime = startTime; // 以降の update は、ここから1フレームずつ進む
	mShownTime = startTime - kTimeStep; // まだこのアニメを見せていない（update の前は、始める時刻の1つ前とみなす）
	mPlaybackRate = kTimeStep;
	mPlayMode = mode;
	mFadeFrames = 0.f;
}

void AnimationPlayer::crossFadeTo( Scene::Animation animation, float startTime, float fadeFrames, PlayMode mode ) {
	// 混ぜる元は「切り替え直前に実際に見えていたポーズ」を写し取ったもの
	// 前のアニメを動かし続けて混ぜる方法だと、フェード中にもう一度切り替えたとき（歩き始めてすぐ止まる等）
	// 混ざっていた分が一瞬で消えてポーズが飛ぶ。見えていたポーズから始めれば、何度切り替えても飛ばない
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

void AnimationPlayer::setPlaybackRate( float rate ) {
	ASSERT( rate > 0.f && "SkinnedMeshInstance : playback rate must be positive." );
	mPlaybackRate = kTimeStep * rate;
}

float AnimationPlayer::animationTime() const {
	return mShownTime;
}

float AnimationPlayer::animationPhase() const {
	float period = mAnimation.period();
	if ( period <= 0.f ) {
		return 0.f;
	}
	float phase = std::fmod( mShownTime, period ) / period;
	return ( phase < 0.f ) ? phase + 1.f : phase;
}

bool AnimationPlayer::isAnimationFinished() const {
	return mPlayMode == PLAY_ONCE && mShownTime >= lastFrameTime();
}

float AnimationPlayer::lastFrameTime() const {
	return mAnimation ? mAnimation.period() - kLastFrameMargin : 0.f;
}

float AnimationPlayer::advanceClock() {
	// 1回だけのアニメは、最後のフレームを超えたらそこで止める（時刻を戻すので、止まった後も増え続けない）
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

void AnimationPlayer::advance() {
	advanceClock();
}

void AnimationPlayer::update() {
	float time = advanceClock();
	PROFILE_COUNT( "Anim.poses", 1 ); // アニメの姿勢を求めた回数
	mSkeleton.updateAnimation( time );
	mData->computeBonePose( mSkeleton, &mBonePose );
	blendFadingPose();
	mHasPose = true;
}

void AnimationPlayer::blendFadingPose() {
	if ( mFadeFrames <= 0.f ) {
		return;
	}
	float t = mFadeElapsed / mFadeFrames; // 進みは advanceClock が数える（終われば mFadeFrames が 0 になっている）
	// 混ぜる割合を、始めと終わりがゆっくりになる曲線（smoothstep）にする
	// 直線だと、フェードの始まりと終わりで動きの速さがカクッと変わって見えるため
	float weight = ScalarMath::smoothStep( t );
	for ( size_t b = 0; b < mBonePose.size(); ++b ) {
		MatrixBlend::blend( mFadeFromPose[ b ], mBonePose[ b ], weight, &mBonePose[ b ] );
	}
}

bool AnimationPlayer::hasPose() const {
	return mHasPose;
}

const std::vector< Matrix34 >& AnimationPlayer::pose() const {
	return mBonePose;
}
