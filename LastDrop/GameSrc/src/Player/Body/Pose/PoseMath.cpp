#include "GameSrc/Player/Body/Pose/PoseMath.h"
#include "GameSrc/Math/MatrixBlend.h"

using namespace GameLib::Math;

void PoseMath::transformBones( const Matrix34& transform, const std::vector< int >& bones, std::vector< Matrix34 >* pose ) {
	for ( int b : bones ) {
		Matrix34 moved;
		moved.setMul( transform, ( *pose )[ b ] );
		( *pose )[ b ] = moved;
	}
}

void PoseMath::transformBonesPartially( const Matrix34& transform, float weight, const std::vector< int >& bones, std::vector< Matrix34 >* pose ) {
	Matrix34 none;
	none.setIdentity();
	Matrix34 partial;
	MatrixBlend::blend( none, transform, weight, &partial );
	transformBones( partial, bones, pose );
}

void PoseMath::weaponAxes( const Matrix34& gun, Vector3* barrel, Vector3* side ) {
	// 銃の向き（銃口 = モデルの +Z、上 = +Y）。持ち方の縮尺が入っているので長さ1にする
	barrel->set( gun.m02, gun.m12, gun.m22 );
	barrel->normalize();
	side->setCross( Vector3( gun.m01, gun.m11, gun.m21 ), *barrel );
	side->normalize();
}

Matrix34 PoseMath::alignWeapon( const Matrix34& gun, const Matrix34& axes, const Vector3& anchor, const Vector3& goal ) {
	// 銃の向きを、長さ1で直交する3軸にする（持ち方の縮尺を取り除く）
	Vector3 barrel;
	Vector3 gunSide;
	weaponAxes( gun, &barrel, &gunSide );
	Vector3 gunUp;
	gunUp.setCross( barrel, gunSide );

	// 回転: 銃の3軸を axes の3軸へ重ねる（= axes × 銃の軸の転置）。銃の傾き（カント）も axes に揃う
	Matrix34 align;
	align.setIdentity();
	const int kAxisNumber = 3;
	const int kSideAxis = 0;
	const int kUpAxis = 1;
	const int kBarrelAxis = 2;
	for ( int row = 0; row < kAxisNumber; ++row ) {
		for ( int column = 0; column < kAxisNumber; ++column ) {
			align( row, column ) = axes( row, kSideAxis ) * gunSide[ column ]
				+ axes( row, kUpAxis ) * gunUp[ column ]
				+ axes( row, kBarrelAxis ) * barrel[ column ];
		}
	}
	// 移動: 回した後の anchor が goal へ来るようにする
	Vector3 rotatedAnchor;
	align.mul33( &rotatedAnchor, anchor );
	align.m03 = goal.x - rotatedAnchor.x;
	align.m13 = goal.y - rotatedAnchor.y;
	align.m23 = goal.z - rotatedAnchor.z;
	return align;
}
