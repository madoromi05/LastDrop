#include "GameSrc/DebugMode/TownEditFrame.h"

#ifdef ZF_EDITOR

#include "GameSrc/Math/PlacementMatrix.h"
#include "GameSrc/Hit/HitMath.h"

using namespace GameLib::Math;

namespace{
	const int kKindNumber = static_cast< int >( TownEditKind::Count );
	const float kHalf = 0.5f;

	// 枠の箱の大きさ（m）。物の向きで見た左右（X）の半分・足元からの高さ・前後（Z）の半分。物のモデルの頂点を囲む箱を少し丸めた値
	struct FrameSize{
		float halfWidth;
		float height;
		float halfDepth;
	};
	const FrameSize kPersonFrame = { 0.4f, 1.8f, 0.4f };	// 人 1 人ぶん（立つ所の印。モデルが無い）
	const FrameSize kBuildingFrame = { 4.2f, 14.f, 4.f };	// 建物。ブロックの中も町を囲むのも、足元の板の無い同じ形（building_noground.txt。WorldBuilder）
	const FrameSize kFrameSizes[ kKindNumber ] = {
		kBuildingFrame,				// Building
		kBuildingFrame,				// PerimeterBuilding
		{ 0.9f, 1.25f, 0.55f },		// Dumpster
		{ 0.55f, 0.65f, 0.35f },	// Crate
		{ 0.15f, 0.8f, 0.17f },		// FireHydrant
		{ 0.26f, 3.9f, 0.26f },		// StreetLight：柱とランタン
		kPersonFrame,				// ZombieSpawn
		{ 1.25f, 3.f, 2.25f },		// EscapeCar：UAZ
		kPersonFrame,				// PlayerStart
	};

	const float kPickRange = 1000.f;			// 線の長さ（m）。町の端から端より長くして、見えている物はどれも届くようにする
	const float kGroundHeight = 0.f;			// 足す物を置く地面（車道の面。TownPlacement の y の基準）
	const float kAddFallbackDistance = 5.f;		// 線が地面に当たらないときに、目から前へ置く距離（m）

	const FrameSize& sizeOf( TownEditKind kind ) {
		return kFrameSizes[ static_cast< int >( kind ) ];
	}

	float squareDistanceToRay( const Vector3& eye, const Vector3& direction, const Vector3& p ) {	// p から線までの距離の 2 乗。線は eye から direction へだけ伸び、後ろへは伸びない
		Vector3 toPoint;
		toPoint.setSub( p, eye );
		const float along = toPoint.dot( direction );
		if ( along <= 0.f ) {
			return toPoint.squareLength();
		}
		Vector3 closest;
		closest.setMadd( eye, direction, along );
		Vector3 gap;
		gap.setSub( p, closest );
		return gap.squareLength();
	}
}

Matrix34 TownEditFrame::boxMatrix( TownEditKind kind, const TownPlacement& at ) {
	const FrameSize& size = sizeOf( kind );
	Matrix34 box = PlacementMatrix::make( at.position(), at.yawDegree );
	box.translate( 0.f, kHalf * size.height, 0.f );	// 足元から上へ立てる（-1〜1 の立方体の底を足元に合わせる）
	box.scale( size.halfWidth, kHalf * size.height, size.halfDepth );
	return box;
}

Vector3 TownEditFrame::center( TownEditKind kind, const TownPlacement& at ) {
	return PlacementMatrix::translationOf( boxMatrix( kind, at ) );
}

bool TownEditFrame::hit( const Vector3& eye, const Vector3& direction, TownEditKind kind, const TownPlacement& at, float* distance ) {
	Matrix34 toLocal;
	toLocal.setInverse( boxMatrix( kind, at ) );
	Vector3 delta;
	delta.setMul( direction, kPickRange );
	Vector3 localOrigin;
	Vector3 localDelta;
	toLocal.mul( &localOrigin, eye );
	toLocal.mul33( &localDelta, delta );
	float time = 0.f;
	Vector3 localNormal;	// 向きは使わない
	if ( !HitMath::segmentUnitCube( localOrigin, localDelta, &time, &localNormal ) ) {
		return false;
	}
	*distance = time * kPickRange;
	return true;
}

int TownEditFrame::pick( const Vector3& eye, const Vector3& direction, const std::vector< Target >& targets ) {
	int nearestHit = kNoTarget;
	float nearestHitDistance = 0.f;
	int nearestCenter = kNoTarget;
	float nearestCenterSquare = 0.f;
	for ( int i = 0; i < static_cast< int >( targets.size() ); ++i ) {
		const Target& target = targets[ i ];
		float distance = 0.f;
		if ( hit( eye, direction, target.kind, target.at, &distance ) && ( nearestHit == kNoTarget || distance < nearestHitDistance ) ) {
			nearestHit = i;
			nearestHitDistance = distance;
		}
		const float square = squareDistanceToRay( eye, direction, center( target.kind, target.at ) );
		if ( nearestCenter == kNoTarget || square < nearestCenterSquare ) {
			nearestCenter = i;
			nearestCenterSquare = square;
		}
	}
	return ( nearestHit != kNoTarget ) ? nearestHit : nearestCenter;
}

Vector3 TownEditFrame::addPoint( const Vector3& eye, const Vector3& direction ) {
	if ( direction.y != 0.f ) {
		const float along = ( kGroundHeight - eye.y ) / direction.y;
		if ( along > 0.f ) {
			Vector3 ground;
			ground.setMadd( eye, direction, along );
			ground.y = kGroundHeight;	// 割り算の誤差で地面から浮かせない
			return ground;
		}
	}
	Vector3 ahead;
	ahead.setMadd( eye, direction, kAddFallbackDistance );
	ahead.y = kGroundHeight;
	return ahead;
}

#endif // ZF_EDITOR
