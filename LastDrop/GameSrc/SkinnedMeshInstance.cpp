#include "SkinnedMeshInstance.h"
#include "SkinnedMeshData.h"
#include "MatrixBlend.h"

#include "GameLib/GameLib.h"
#include "GameLib/Base/DebugStream.h"
#include "GameLib/Scene/Animation.h"

#include <cmath>

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const float kTimeStep = 1.f; // update 1回で進めるアニメ時刻（アニメのキー時刻はゲームのフレーム数）

	// 混ぜる割合を、始めと終わりがゆっくりになる曲線（smoothstep）にする。
	// 直線だと、フェードの始まりと終わりで動きの速さがカクッと変わって見えるため。
	float easeInOut( float t ) {
		return t * t * ( 3.f - 2.f * t );
	}
}

SkinnedMeshInstance::SkinnedMeshInstance( const SkinnedMeshData* data ) :
	mData( data ),
	mTime( 0.f ),
	mFadeFrames( 0.f ),
	mFadeElapsed( 0.f ),
	mHasPose( false ),
	mPosition( 0.f, 0.f, 0.f ),
	mAngleY( 0.f ) {
	ASSERT( mData && mData->isReady() && "SkinnedMeshInstance : data is null or not ready." );
	mSkeleton = mData->createSkeleton();
	mBonePose.resize( mData->boneNumber() );
	mFadeFromPose.resize( mData->boneNumber() );
	mSkinMatrices.resize( mData->boneNumber() );
	mVertexBuffer = Graphics::VertexBuffer::create( mData->vertexNumber() );
}

void SkinnedMeshInstance::setPosition( const Vector3& position ) {
	mPosition = position;
}

void SkinnedMeshInstance::setAngleY( float degree ) {
	mAngleY = degree;
}

void SkinnedMeshInstance::setAnimation( const char* name, float startTime ) {
	Scene::Animation animation = mData->animation( name );
	if ( !animation ) {
		// 名前違いでもエラーにならず静止したままになるので、ここで知らせる。
		GameLib::cout << "SkinnedMeshInstance: animation not found: " << name << GameLib::endl;
	}
	setAnimation( animation, startTime );
}

void SkinnedMeshInstance::setAnimation( Scene::Animation animation, float startTime ) {
	mSkeleton.setAnimation( animation );
	mAnimation = animation;
	mTime = startTime; // 以降の update は、ここから1フレームずつ進む
	mFadeFrames = 0.f;
}

void SkinnedMeshInstance::crossFadeTo( Scene::Animation animation, float startTime, float fadeFrames ) {
	// 混ぜる元は「切り替え直前に実際に見えていたポーズ」を写し取ったもの。
	// 前のアニメを動かし続けて混ぜる方法だと、フェード中にもう一度切り替えたとき（歩き始めてすぐ止まる等）
	// 混ざっていた分が一瞬で消えてポーズが飛ぶ。見えていたポーズから始めれば、何度切り替えても飛ばない。
	bool canFade = mHasPose && fadeFrames > 0.f;
	if ( canFade ) {
		mFadeFromPose = mBonePose;
	}
	setAnimation( animation, startTime );
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
	// update は時刻を使ってから進めるので、今見えているのは1つ前の時刻。
	float shownTime = mTime - kTimeStep;
	float phase = std::fmod( shownTime, period ) / period;
	return ( phase < 0.f ) ? phase + 1.f : phase;
}

void SkinnedMeshInstance::update() {
	mSkeleton.updateAnimation( mTime );
	mTime += kTimeStep;
	mData->computeBonePose( mSkeleton, &mBonePose );
	blendFadingPose();
	mHasPose = true;

	// 置き場所と向きは、混ぜた後のポーズにまとめて掛ける。
	mData->deform( rootMatrix(), mBonePose, &mSkinMatrices, mVertexBuffer );
}

Matrix34 SkinnedMeshInstance::rootMatrix() const {
	Matrix34 root;
	root.setTranslation( mPosition );
	root.rotateY( mAngleY );
	return root;
}

bool SkinnedMeshInstance::boneMatrix( int boneIndex, Matrix34* out ) const {
	if ( !mHasPose || boneIndex < 0 || boneIndex >= static_cast< int >( mBonePose.size() ) ) {
		return false;
	}
	// deform と同じ「置き場所 * ボーンの姿勢」。頂点と同じ式なので、付けた物が手からずれない。
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
	mData->draw( mVertexBuffer );
}
