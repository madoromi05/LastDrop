#include "GameSrc/Zombie/SeveredLimb.h"
#include "GameSrc/Zombie/ZombieDismemberment.h"
#include "GameSrc/Render/SkinnedMesh/SkinnedMeshData.h"
#include "GameSrc/Hit/IRayBlocker.h"
#include "GameSrc/Math/AxisRotation.h"
#include "GameSrc/Core/GameTime.h"

#include "GameLib/GameLib.h"
#include "GameLib/Scene/Animation.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// ── 飛び方（見た目を確かめる前の初期値。SummaryPlan/個別仕様/ZombieDismember_Plan.txt の 4 章）──
	// 弾の向きへの速さと、上への速さ（m/秒）。上へ投げ上げて、弧を描いて落とす。
	const float kFlySpeed = 2.5f / GameTime::kTicksPerSecond;
	const float kUpSpeed = 2.f / GameTime::kTicksPerSecond;
	// 回る速さの範囲（度/秒）。軸はでたらめ。
	const float kMinSpinSpeed = 360.f / GameTime::kTicksPerSecond;
	const float kMaxSpinSpeed = 720.f / GameTime::kTicksPerSecond;
	// 落ちる加速度（m/ティック²）。本当の重力（9.8m/秒²）。
	const float kGravityPerTick = 9.8f / ( GameTime::kTicksPerSecond * GameTime::kTicksPerSecond );

	// 止まったときの、地面から部位の中心までの高さ（m）。腕・脚の太さの半分くらい。
	const float kRestHeight = 0.08f;
	// 地面の高さを聞く縦の線（中心の少し上から下まで。m）。歩道の段差（15cm）より長く上から始める。
	const float kGroundRayAbove = 0.5f;
	const float kGroundRayBelow = 3.f;
	const float kFallbackGroundHeight = 0.f; // 地面が無い・見つからないときの高さ（町の地面は Y=0）

	// 止まってから沈み始めるまで（秒）と、沈むのにかける時間（秒）・深さ（m）。ゾンビの死体と同じ流れ。
	// 深さは腕・脚の太さより深くして、沈み終わりに何も見えないようにする（胴より細いのでゾンビの 0.5m より浅い）。
	const float kRestSeconds = 10.f;
	const float kSinkSeconds = 1.5f;
	const float kSinkDepth = 0.3f;
	const int kRestTicks = static_cast< int >( kRestSeconds * GameTime::kTicksPerSecond );
	const int kSinkTicks = static_cast< int >( kSinkSeconds * GameTime::kTicksPerSecond );

	const Vector3 kOrigin( 0.f, 0.f, 0.f );
	const float kAnimationStartTime = 0.f;
	// 長さがこれより短い軸は回る軸にしない（向きが決まらない）。
	const float kMinAxisLength = 1e-3f;
	const Vector3 kFallbackAxis( 1.f, 0.f, 0.f );

	Vector3 translationOf( const Matrix34& m ) {
		return Vector3( m.m03, m.m13, m.m23 );
	}

	// でたらめな向きの長さ1の軸。
	Vector3 randomAxis( std::mt19937* random ) {
		std::uniform_real_distribution< float > component( -1.f, 1.f );
		Vector3 axis( component( *random ), component( *random ), component( *random ) );
		if ( axis.length() < kMinAxisLength ) {
			return kFallbackAxis;
		}
		axis.normalize();
		return axis;
	}

	// position の真下の地面の高さ。ground が無い・当たらなければ kFallbackGroundHeight。
	float groundHeightBelow( const IRayBlocker* ground, const Vector3& position ) {
		if ( !ground ) {
			return kFallbackGroundHeight;
		}
		Vector3 origin( position.x, position.y + kGroundRayAbove, position.z );
		Vector3 delta( 0.f, -( kGroundRayAbove + kGroundRayBelow ), 0.f );
		float time;
		Vector3 normal;
		if ( !ground->raycast( origin, delta, &time, &normal ) ) {
			return kFallbackGroundHeight;
		}
		return origin.y + delta.y * time;
	}
}

SeveredLimb::SeveredLimb( const SkinnedMeshData* data, const ZombieSeverLayout::Cut& cut, const std::vector< Matrix34 >& worldPose,
const Vector3& direction, std::mt19937* random ) :
	mMesh( data ),
	mSpinPerTick( 0.f ),
	mSpinAngle( 0.f ),
	mState( State::Flying ),
	mStateTicks( 0 ) {
	ASSERT( random && "SeveredLimb : random is null." );
	ASSERT( static_cast< int >( worldPose.size() ) == data->boneNumber() && "SeveredLimb : pose size mismatch." );
	// 骨格の根を原点・向き 0 に置くので、骨の姿勢をワールドのまま渡せる（置き場所の行列が単位行列になる）。
	mMesh.setAnimation( Scene::Animation(), kAnimationStartTime ); // アニメは使わない（姿勢は apply で全部書き換える）
	mMesh.setPoseModifier( this );

	// 残す骨は切れた瞬間の姿勢のまま、ほかは切り口の1点へ潰す。中心は残す骨の原点の平均。
	Matrix34 collapsed = ZombieDismemberment::collapsedAt( translationOf( worldPose[ cut.jointBone ] ) );
	mBasePose.assign( worldPose.size(), collapsed );
	mStartCenter.set( 0.f, 0.f, 0.f );
	for ( int bone : cut.bones ) {
		mBasePose[ bone ] = worldPose[ bone ];
		mStartCenter += translationOf( worldPose[ bone ] );
	}
	if ( !cut.bones.empty() ) {
		mStartCenter *= 1.f / static_cast< float >( cut.bones.size() );
	}
	mCenter = mStartCenter;

	mVelocity.setMul( direction, kFlySpeed );
	mVelocity.y += kUpSpeed;
	mSpinAxis = randomAxis( random );
	std::uniform_real_distribution< float > spin( kMinSpinSpeed, kMaxSpinSpeed );
	mSpinPerTick = spin( *random );
	mMotion.setIdentity();
	mMesh.update(); // 出たティックの描画から見えるように、姿勢を作っておく
}

void SeveredLimb::update( const IRayBlocker* ground ) {
	float sinkOffset = 0.f;
	switch ( mState ) {
		case State::Flying:
			fly( ground );
			break;
		case State::Resting:
			++mStateTicks;
			if ( mStateTicks >= kRestTicks ) {
				mState = State::Sinking;
				mStateTicks = 0;
			}
			break;
		case State::Sinking: {
			++mStateTicks;
			float progress = static_cast< float >( mStateTicks ) / static_cast< float >( kSinkTicks );
			sinkOffset = kSinkDepth * ( ( progress > 1.f ) ? 1.f : progress );
			break;
		}
	}
	updateMotion( sinkOffset );
	mMesh.update();
}

void SeveredLimb::fly( const IRayBlocker* ground ) {
	mVelocity.y -= kGravityPerTick;
	mCenter += mVelocity;
	mSpinAngle += mSpinPerTick;
	// 上へ飛んでいる間は地面に着かないので、聞かない（地面の判定は部品を全部調べるので重い）。
	if ( mVelocity.y > 0.f ) {
		return;
	}
	float restY = groundHeightBelow( ground, mCenter ) + kRestHeight;
	if ( mCenter.y <= restY ) {
		// 跳ねない。回るのもここで止める（寝かせ直しはしない）。
		mCenter.y = restY;
		mState = State::Resting;
		mStateTicks = 0;
	}
}

void SeveredLimb::updateMotion( float sinkOffset ) {
	// 切れた瞬間の中心を軸に回してから、中心を今の位置へ動かす（= 回転 R・移動 今の中心 - R*始めの中心）。
	mMotion = AxisRotation::about( kOrigin, mSpinAxis, mSpinAngle );
	Vector3 rotatedStart;
	mMotion.mul33( &rotatedStart, mStartCenter );
	mMotion.m03 = mCenter.x - rotatedStart.x;
	mMotion.m13 = mCenter.y - sinkOffset - rotatedStart.y;
	mMotion.m23 = mCenter.z - rotatedStart.z;
}

void SeveredLimb::apply( std::vector< Matrix34 >* pose ) {
	for ( size_t b = 0; b < pose->size(); ++b ) {
		( *pose )[ b ].setMul( mMotion, mBasePose[ b ] );
	}
}

void SeveredLimb::draw() const {
	mMesh.draw();
}

bool SeveredLimb::isGone() const {
	return mState == State::Sinking && mStateTicks >= kSinkTicks;
}
