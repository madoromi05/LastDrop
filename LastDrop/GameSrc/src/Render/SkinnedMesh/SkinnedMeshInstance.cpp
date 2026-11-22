#include "GameSrc/Render/SkinnedMesh/SkinnedMeshInstance.h"
#include "GameSrc/Render/SkinnedMesh/SkinnedMeshData.h"
#include "GameSrc/Render/SkinnedMesh/BonePoseModifier.h"
#include "GameSrc/Math/PlacementMatrix.h"

#include "GameLib/GameLib.h"
#include "GameLib/Profiling/Profiler.h"

using namespace GameLib;
using namespace GameLib::Math;

SkinnedMeshInstance::SkinnedMeshInstance( const SkinnedMeshData* data ) :
	mData( data ),
	mPlayer( data ),
	mPoseModifier( nullptr ),
	mBodyPoseModifier( nullptr ),
	mSkinStale( true ),
	mPosition( 0.f, 0.f, 0.f ),
	mAngleY( 0.f ) {
	ASSERT( mData && mData->isReady() && "SkinnedMeshInstance : data is null or not ready." );
	mShownPose.resize( mData->boneNumber() );
	mBodyPose.resize( mData->boneNumber() );
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

// ── アニメの口（中身は AnimationPlayer） ──

void SkinnedMeshInstance::setAnimation( Scene::Animation animation, float startTime, PlayMode mode ) {
	mPlayer.setAnimation( animation, startTime, mode );
}

void SkinnedMeshInstance::crossFadeTo( Scene::Animation animation, float startTime, float fadeFrames, PlayMode mode ) {
	mPlayer.crossFadeTo( animation, startTime, fadeFrames, mode );
}

void SkinnedMeshInstance::setPlaybackRate( float rate ) {
	mPlayer.setPlaybackRate( rate );
}

float SkinnedMeshInstance::animationTime() const {
	return mPlayer.animationTime();
}

float SkinnedMeshInstance::animationPhase() const {
	return mPlayer.animationPhase();
}

bool SkinnedMeshInstance::isAnimationFinished() const {
	return mPlayer.isAnimationFinished();
}

void SkinnedMeshInstance::advance() {
	mPlayer.advance();
}

// ── 書き換え・置き場所・描画 ──

void SkinnedMeshInstance::setPoseModifier( BonePoseModifier* modifier ) {
	mPoseModifier = modifier;
}

void SkinnedMeshInstance::setBodyPoseModifier( BonePoseModifier* modifier ) {
	mBodyPoseModifier = modifier;
}

const std::vector< Matrix34 >& SkinnedMeshInstance::bodyPose() const {
	return mBodyPoseModifier ? mBodyPose : mPlayer.pose();
}

void SkinnedMeshInstance::update() {
	mPlayer.update();
	mSkinStale = true;

	// 体の書き換え（当たり判定も動く）→ 見た目だけの書き換え、の順に掛ける
	// アニメのポーズ（mPlayer.pose）に直接入れないのは、クロスフェードの混ぜ元に入り込み、混ぜた後にもう一度掛かって二重になるため
	if ( mBodyPoseModifier ) {
		mBodyPose = mPlayer.pose();
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
	if ( !mPlayer.hasPose() || boneIndex < 0 || boneIndex >= static_cast< int >( mShownPose.size() ) ) {
		return false;
	}
	// 頂点の変形（computeSkinMatrices）と同じ「置き場所 * ボーンの姿勢」。同じ式なので、付けた物が手からずれない
	out->setMul( rootMatrix(), mShownPose[ boneIndex ] );
	return true;
}

bool SkinnedMeshInstance::animatedBoneMatrix( int boneIndex, Matrix34* out ) const {
	const std::vector< Matrix34 >& pose = bodyPose();
	if ( !mPlayer.hasPose() || boneIndex < 0 || boneIndex >= static_cast< int >( pose.size() ) ) {
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

void SkinnedMeshInstance::draw() const {
	if ( !mPlayer.hasPose() ) {
		return; // まだ update していない（見せるポーズが無い）
	}
	// 骨の行列は、ポーズか置き場所が変わった後に初めて描く時だけ作る。G-Buffer と影（最大 4 枚）で同じ体を何度描いても 1 回で済む
	// 置き場所と向きは、混ぜて書き換えた後のポーズにまとめて掛ける
	if ( mSkinStale ) {
		PROFILE_COUNT( "Skin.matrices", 1 ); // 骨の行列を作った回数
		mData->computeSkinMatrices( rootMatrix(), mShownPose, &mSkinMatrices );
		mSkinStale = false;
	}
	mData->draw( mSkinMatrices );
}
