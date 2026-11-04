#include "GameSrc/Weapon/WeaponModelComponent.h"
#include "GameSrc/Render/StaticMesh/StaticMeshData.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Functions.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// 骨格は cm（根に 0.01 倍が入っている）、銃は m なので、骨の空間では銃を100倍して等倍に戻す。
	const float kWeaponScaleInBone = 100.f;
	// 銃のモデルの銃口の向き（+Z）。どの銃も +Z を前にして変換する（tools/configs/*.json）。
	const Vector3 kMuzzleAxis( 0.f, 0.f, 1.f );
}

WeaponModelComponent::WeaponModelComponent( const StaticMeshData* weapon, const StaticMeshData* magazine, const WeaponGrip& grip ) :
	mWeapon( weapon ),
	mMagazine( magazine ),
	mGrip( &grip ) {
	ASSERT( weapon && "WeaponModelComponent : weapon is null." );
}

const StaticMeshData* WeaponModelComponent::weapon() const {
	return mWeapon;
}

const StaticMeshData* WeaponModelComponent::magazine() const {
	return mMagazine;
}

Matrix34 WeaponModelComponent::gripInHand() const {
	// 握りの中心を原点へ動かし、大きさ・向きを合わせてから手の位置へ置く。
	Matrix34 grip;
	grip.setTranslation( mGrip->gripInHand );
	grip.rotateY( mGrip->rotationY );
	grip.rotateX( mGrip->rotationX );
	grip.rotateZ( mGrip->rotationZ );
	grip.scale( kWeaponScaleInBone );
	grip.translate( -mGrip->gripOnWeapon.x, -mGrip->gripOnWeapon.y, -mGrip->gripOnWeapon.z );
	return grip;
}

const Vector3& WeaponModelComponent::sightEye() const {
	return mGrip->sightEye;
}

void WeaponModelComponent::bounds( Vector3* boundsMin, Vector3* boundsMax ) const {
	ASSERT( boundsMin && boundsMax && "WeaponModelComponent::bounds : out is null." );
	*boundsMin = mWeapon->boundsMin();
	*boundsMax = mWeapon->boundsMax();
	// 弾倉も含める（弾倉を分ける前と同じ大きさにする）。
	if ( mMagazine ) {
		const Vector3& low = mMagazine->boundsMin();
		const Vector3& high = mMagazine->boundsMax();
		boundsMin->set( Math::min( boundsMin->x, low.x ), Math::min( boundsMin->y, low.y ), Math::min( boundsMin->z, low.z ) );
		boundsMax->set( Math::max( boundsMax->x, high.x ), Math::max( boundsMax->y, high.y ), Math::max( boundsMax->z, high.z ) );
	}
}

void WeaponModelComponent::muzzle( const Matrix34& placement, Vector3* position, Vector3* direction ) const {
	ASSERT( position && direction && "WeaponModelComponent::muzzle : out is null." );
	placement.mul( position, mGrip->muzzlePoint );     // 点なので、移動も掛ける
	placement.mul33( direction, kMuzzleAxis );   // 向きなので、移動は掛けない
	direction->normalize();                      // 持ち方・骨の縮尺を取り除いて長さ1に
}
