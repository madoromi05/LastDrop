#include "GameSrc/Scene/Game/EffectManager.h"
#include "GameSrc/Zombie/ZombieModel.h"
#include "GameSrc/Render/SkinnedMesh/SkinnedMeshInstance.h"
#include "GameSrc/Render/DrawCulling.h"

using namespace GameLib::Math;

namespace{
	// 血しぶきの量と大きさの倍率。頭は多く大きく飛ばす（当たった手応えを一番強くする部位）。
	const float kBodyBloodScale = 1.f;
	const float kHeadBloodScale = 1.5f;
	// 部位が千切れたときに切り口から出す血しぶきの倍率。当たったときより大きく、一番多く出す（BloodSpray::kMaxDropNumber まで）。
	const float kSeverBloodScale = 2.f;
}

EffectManager::EffectManager() {
}

void EffectManager::spawnSurfaceImpact( const Vector3& point, const Vector3& normal, const IRayBlocker* blocker ) {
	mBulletHoles.add( point, normal, blocker );
	mImpactDust.spawn( point, normal );
}

void EffectManager::spawnBloodHit( const Vector3& point, const Vector3& direction, BodyPart part ) {
	mBloodSpray.spawn( point, direction, ( part == BodyPart::Head ) ? kHeadBloodScale : kBodyBloodScale );
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
}

void EffectManager::update( const IRayBlocker* ground ) {
	mBulletHoles.update();
	mImpactDust.update();
	mBloodSpray.update();
	mSeveredLimbs.update( ground );
}

void EffectManager::drawDecals( const Matrix44& projectionView ) {
	mBulletHoles.draw( projectionView );
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
