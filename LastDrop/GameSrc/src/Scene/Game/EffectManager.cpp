#include "GameSrc/Scene/Game/EffectManager.h"
#include "GameSrc/Zombie/ZombieModel.h"
#include "GameSrc/Render/SkinnedMesh/SkinnedMeshInstance.h"
#include "GameSrc/Render/DrawCulling.h"

using namespace GameLib::Math;

namespace{
	// 血しぶきの量と大きさの倍率（1 回は BloodSpray::kMaxScale まで。HitFeel_Plan.txt）。
	// 弾が当たった音を 1 つにしたので、頭の手応えは血の量で出す（2026-11-06 ユーザー指定。頭は 1.5 → 上限）。
	const float kBodyBloodScale = 0.8f;                 // ふつう（少し控えめにして、重い命中との差を作る）
	const float kHeadBloodScale = BloodSpray::kMaxScale; // 頭に当たった（倒れていない）
	const float kHeavyBloodScale = 1.2f;                // 部位が飛んだ・倒した（切り口からは別に kSeverBloodScale が出る）
	// 部位が千切れたときに切り口から出す血しぶきの倍率。当たったときより大きく、一番多く出す（BloodSpray::kMaxDropNumber まで）。
	const float kSeverBloodScale = BloodSpray::kMaxScale;
	// 頭が弾けたときは、上限の血しぶきを弾の向き・真上・撃った側の 3 方向へ出す（1 回では上限を超えられないため）。
	const float kBurstBloodScale = BloodSpray::kMaxScale;
	const Vector3 kUp( 0.f, 1.f, 0.f );
	const float kReverse = -1.f;

	float bloodHitScale( EffectManager::BloodHitSize size ) {
		switch ( size ) {
			case EffectManager::BloodHitSize::Head:  return kHeadBloodScale;
			case EffectManager::BloodHitSize::Heavy: return kHeavyBloodScale;
			case EffectManager::BloodHitSize::Burst: return kBurstBloodScale;
			default:                                 return kBodyBloodScale;
		}
	}

	// 血の跡の幅の半分（m。BloodStain_Plan.txt。ZombieHitFeedback_Plan.txt 4 章の 25cm・頭 35cm）。
	// 滴の跡は飛び散った血の半分（落ちた滴だけなので小さい）。
	const float kSplatterHalfSize = 0.125f;
	const float kLargeSplatterHalfSize = 0.175f;
	const float kDripSizeRatio = 0.5f;

	float splatterHalfSize( EffectManager::BloodStainSize size ) {
		return ( size == EffectManager::BloodStainSize::Large ) ? kLargeSplatterHalfSize : kSplatterHalfSize;
	}
}

EffectManager::EffectManager() {
}

void EffectManager::spawnSurfaceImpact( const Vector3& point, const Vector3& normal, const IRayBlocker* blocker ) {
	mBulletHoles.add( point, normal, blocker );
	mImpactDust.spawn( point, normal );
}

void EffectManager::spawnBloodHit( const Vector3& point, const Vector3& direction, BloodHitSize size ) {
	mBloodSpray.spawn( point, direction, bloodHitScale( size ) );
	if ( size != BloodHitSize::Burst ) {
		return;
	}
	Vector3 back;
	back.setMul( direction, kReverse );
	mBloodSpray.spawn( point, kUp, kBurstBloodScale );   // 真上
	mBloodSpray.spawn( point, back, kBurstBloodScale );  // 撃った側
}

void EffectManager::spawnBloodSplatter( const Vector3& point, const Vector3& normal, const Vector3& direction,
const IRayBlocker* blocker, BloodStainSize size ) {
	mBloodStains.addSplatter( point, normal, direction, splatterHalfSize( size ), blocker );
}

void EffectManager::spawnBloodDrip( const Vector3& point, const Vector3& normal, const IRayBlocker* blocker, BloodStainSize size ) {
	mBloodStains.addDrip( point, normal, splatterHalfSize( size ) * kDripSizeRatio, blocker );
}

void EffectManager::spawnSeveredLimb( const SkinnedMeshInstance& zombieMesh, const ZombieModel& model, BodyPart part,
const Vector3& direction ) {
	zombieMesh.shownWorldPose( &mSeverPose );
	const ZombieSeverLayout::Cut& cut = model.severLayout().cut( part );
	const Matrix34& joint = mSeverPose[ cut.jointBone ];
	mBloodSpray.spawn( Vector3( joint.m03, joint.m13, joint.m23 ), direction, kSeverBloodScale );
	mSeveredLimbs.spawn( model.data(), cut, mSeverPose, direction );
}

void EffectManager::detach( const IRayBlocker* blocker ) {
	mBulletHoles.removeAttachedTo( blocker );
	mBloodStains.removeAttachedTo( blocker );
}

void EffectManager::update( const IRayBlocker* ground ) {
	mBulletHoles.update();
	mImpactDust.update();
	mBloodSpray.update();
	mBloodStains.update();
	mSeveredLimbs.update( ground );
}

void EffectManager::drawDecals( const Matrix44& projectionView ) {
	mBulletHoles.draw( projectionView );
	mBloodStains.draw( projectionView ); // 弾痕の後（血が穴の上に乗る）
}

void EffectManager::drawImpactDust( const Matrix44& projectionView, const Vector3& viewDirection, const Vector3& eye ) {
	mImpactDust.draw( projectionView, viewDirection, eye );
}

void EffectManager::drawBloodSpray( const Matrix44& projectionView, const Vector3& viewDirection, const Vector3& eye ) {
	mBloodSpray.draw( projectionView, viewDirection, eye );
}

void EffectManager::drawSeveredLimbs( const DrawCulling& culling ) const {
	mSeveredLimbs.draw( culling );
}
