#include "GameSrc/Scene/Game/World/EffectManager.h"

using namespace GameLib::Math;

namespace{
	// 血しぶきの量と大きさの倍率（1 回は BloodSpray::kMaxScale まで）
	// 弾が当たった音は 1 つなので、頭の手応えは血の量で出す（頭は 1.5 → 上限）
	const float kBodyBloodScale = 0.8f;						// ふつう（少し控えめにして、重い命中との差を作る）
	const float kHeadBloodScale = BloodSpray::kMaxScale;	// 頭に当たった（倒れていない）
	const float kHeavyBloodScale = 1.2f;					// 部位が飛んだ・倒した（切り口からは別に kSeverBloodScale が出る）
	const float kSeverBloodScale = BloodSpray::kMaxScale;	// 部位が千切れたときに切り口から出す血しぶきの倍率。当たったときより大きく、一番多く出す（BloodSpray::kMaxDropNumber まで）
	// 頭が弾けたときは、上限の血しぶきを弾の向き・真上・撃った側の 3 方向へ出す（1 回では上限を超えられないため）
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

	// 血の跡の幅の半分（m）
	// 滴の跡は飛び散った血の半分（落ちた滴だけなので小さい）
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

void EffectManager::spawnSeverBlood( const Vector3& jointPosition, const Vector3& direction ) {
	mBloodSpray.spawn( jointPosition, direction, kSeverBloodScale );
}

void EffectManager::detach( const IRayBlocker* blocker ) {
	mBulletHoles.removeAttachedTo( blocker );
	mBloodStains.removeAttachedTo( blocker );
}

void EffectManager::update() {
	mBulletHoles.update();
	mImpactDust.update();
	mBloodSpray.update();
	mBloodStains.update();
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
