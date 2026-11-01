#include "GameSrc/Weapon/WeaponModelComponent.h"
#include "GameSrc/Render/StaticMesh/StaticMeshData.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Functions.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// ── 銃の持ち方（右手の骨の空間）──
	// 構え（rifle aiming idle の1フレーム目）で、右手が握りを、左手がハンドガードの下を支える位置を
	// Blender で求めた値。Shooter Pack の歩き・走りでも同じ持ち方のまま右手に付いていく。
	const Vector3 kGripOnWeapon( 0.f, 1.438f, -0.169f );   // 銃のモデル上の握りの中心（m。rifle.txt の座標）
	const Vector3 kGripInHand( 0.354f, 0.38f, -9.102f );  // 握りの中心を置く、手の骨から見た位置（cm。骨格の単位）
	// 手の骨から見た銃の向き（度）。骨格と同じく Y→X→Z の順に掛ける（Matrix34 の rotateY / X / Z の順）。
	const float kGripRotationY = -156.062f;
	const float kGripRotationX = 2.115f;
	const float kGripRotationZ = 89.224f;
	// 骨格は cm（根に 0.01 倍が入っている）、銃は m なので、骨の空間では銃を100倍して等倍に戻す。
	const float kWeaponScaleInBone = 100.f;
	// 覗いたときに目を置く点（銃のモデル座標。m）。AKM のサイト線（リアサイト z≈0.05 → フロントサイト z≈0.43、
	// どちらも上端 y≈1.597）を、頂点の上端の並びから求め、ほお付けの位置（銃床の上）まで後ろへ延ばした所。
	// z は、カメラの近い方の面（目の 5cm 先。FirstPersonCamera の kNearClip）が機関部（上端 y≈1.59）に掛からない所にする。
	// 掛かると面で切れて銃の中が見える。-0.28 なら面は z=-0.23（上端 y≈1.545）で、覗いた視野角 45 度の面の下端（y≈1.579）より 3.4cm 低い。
	// サイトが上下にずれるなら y を調整する（z を前へ戻すと、また機関部が切れる）。
	const Vector3 kSightEyeOnWeapon( 0.f, 1.600f, -0.28f );
	// 銃口の先端（銃のモデル座標。m）。rifle.txt の一番前の頂点（z = 0.471）での銃身の中心。
	const Vector3 kMuzzlePoint( 0.f, 1.541f, 0.471f );
	// 銃のモデルの銃口の向き（+Z）。
	const Vector3 kMuzzleAxis( 0.f, 0.f, 1.f );
}

WeaponModelComponent::WeaponModelComponent( const StaticMeshData* weapon, const StaticMeshData* magazine ) :
	mWeapon( weapon ),
	mMagazine( magazine ) {
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
	grip.setTranslation( kGripInHand );
	grip.rotateY( kGripRotationY );
	grip.rotateX( kGripRotationX );
	grip.rotateZ( kGripRotationZ );
	grip.scale( kWeaponScaleInBone );
	grip.translate( -kGripOnWeapon.x, -kGripOnWeapon.y, -kGripOnWeapon.z );
	return grip;
}

const Vector3& WeaponModelComponent::sightEye() const {
	return kSightEyeOnWeapon;
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
	placement.mul( position, kMuzzlePoint );     // 点なので、移動も掛ける
	placement.mul33( direction, kMuzzleAxis );   // 向きなので、移動は掛けない
	direction->normalize();                      // 持ち方・骨の縮尺を取り除いて長さ1に
}
