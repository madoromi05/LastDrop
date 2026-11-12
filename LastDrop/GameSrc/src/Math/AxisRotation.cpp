#include "GameSrc/Math/AxisRotation.h"

#include "GameLib/Math/Functions.h"

using namespace GameLib;
using namespace GameLib::Math;

Matrix34 AxisRotation::about( const Vector3& pivot, const Vector3& axis, float degree ) {
	float c = Math::cos( degree );
	float s = Math::sin( degree );
	float t = 1.f - c;
	Matrix34 m;
	m.setIdentity();
	m.m00 = c + t * axis.x * axis.x;          m.m01 = t * axis.x * axis.y - s * axis.z; m.m02 = t * axis.x * axis.z + s * axis.y;
	m.m10 = t * axis.y * axis.x + s * axis.z; m.m11 = c + t * axis.y * axis.y;          m.m12 = t * axis.y * axis.z - s * axis.x;
	m.m20 = t * axis.z * axis.x - s * axis.y; m.m21 = t * axis.z * axis.y + s * axis.x; m.m22 = c + t * axis.z * axis.z;
	// 中心を原点へ動かして回し、戻す（= 移動成分は pivot - R*pivot）
	Vector3 rotatedPivot;
	m.mul33( &rotatedPivot, pivot );
	m.m03 = pivot.x - rotatedPivot.x;
	m.m13 = pivot.y - rotatedPivot.y;
	m.m23 = pivot.z - rotatedPivot.z;
	return m;
}
