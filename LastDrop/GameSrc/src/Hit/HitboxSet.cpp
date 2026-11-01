#include "GameSrc/Hit/HitboxSet.h"
#include "GameSrc/Hit/HitboxLayout.h"
#include "GameSrc/Render/SkinnedMesh/SkinnedMeshInstance.h"
#include "GameSrc/Hit/HitMath.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Functions.h"

using namespace GameLib;
using namespace GameLib::Math;

HitboxSet::HitboxSet( const HitboxLayout* layout ) :
	mLayout( layout ) {
	ASSERT( mLayout && "HitboxSet : layout is null." );
	Placement notPlaced;
	notPlaced.world.setIdentity();
	notPlaced.toLocal.setIdentity();
	notPlaced.placed = false;
	notPlaced.removed = false;
	mPlacements.assign( mLayout->boxes().size(), notPlaced );
}

void HitboxSet::update( const SkinnedMeshInstance& mesh, PoseSource source ) {
	const std::vector< HitboxLayout::Box >& boxes = mLayout->boxes();
	for ( size_t i = 0; i < boxes.size(); ++i ) {
		Placement& placement = mPlacements[ i ];
		if ( placement.removed ) {
			continue;
		}
		Matrix34 bone;
		placement.placed = ( source == POSE_SHOWN ) ?
			mesh.boneMatrix( boxes[ i ].anchorBone, &bone ) :
			mesh.animatedBoneMatrix( boxes[ i ].anchorBone, &bone );
		if ( !placement.placed ) {
			continue;
		}
		placement.world.setMul( bone, boxes[ i ].local );
		placement.toLocal.setInverse( placement.world );
	}
}

void HitboxSet::removePart( BodyPart part ) {
	const std::vector< HitboxLayout::Box >& boxes = mLayout->boxes();
	for ( size_t i = 0; i < boxes.size(); ++i ) {
		if ( boxes[ i ].part == part ) {
			mPlacements[ i ].removed = true;
			mPlacements[ i ].placed = false; // 当たり判定・表示は placed だけを見る
		}
	}
}

bool HitboxSet::raycast( const Vector3& origin, const Vector3& delta, HitResult* hit ) const {
	const std::vector< HitboxLayout::Box >& boxes = mLayout->boxes();
	bool found = false;
	for ( size_t i = 0; i < boxes.size(); ++i ) {
		const Placement& placement = mPlacements[ i ];
		if ( !placement.placed ) {
			continue;
		}
		// 線分を箱の空間へ移す（始点は移動込み、向きは移動なし）。
		Vector3 localOrigin;
		Vector3 localDelta;
		placement.toLocal.mul( &localOrigin, origin );
		placement.toLocal.mul33( &localDelta, delta );
		float time;
		Vector3 localNormal;
		if ( !HitMath::segmentUnitCube( localOrigin, localDelta, &time, &localNormal ) ) {
			continue;
		}
		if ( found && time >= hit->time ) {
			continue;
		}
		hit->part = boxes[ i ].part;
		hit->time = time;
		hit->point.setMadd( origin, delta, time );
		if ( localNormal.squareLength() > 0.f ) {
			HitMath::normalToWorld( placement.toLocal, localNormal, &hit->normal );
		} else {
			// 始点が箱の中で、入った面が無い。弾が来た向きへ返す。
			hit->normal.setNeg( delta );
			hit->normal.normalize();
		}
		found = true;
	}
	return found;
}

int HitboxSet::boxNumber() const {
	return static_cast< int >( mPlacements.size() );
}

bool HitboxSet::boxMatrix( int index, Matrix34* out ) const {
	const Placement& placement = mPlacements[ index ];
	if ( !placement.placed ) {
		return false;
	}
	*out = placement.world;
	return true;
}

BodyPart HitboxSet::boxPart( int index ) const {
	return mLayout->boxes()[ index ].part;
}

bool HitboxSet::partCenter( BodyPart part, Vector3* out ) const {
	ASSERT( out && "HitboxSet : out is null." );
	for ( int i = 0; i < boxNumber(); ++i ) {
		Matrix34 box;
		if ( boxPart( i ) == part && boxMatrix( i, &box ) ) {
			out->set( box.m03, box.m13, box.m23 ); // -1〜1 の立方体の中心が置かれた所
			return true;
		}
	}
	return false;
}

bool HitboxSet::bounds( Vector3* boundsMin, Vector3* boundsMax ) const {
	ASSERT( boundsMin && boundsMax && "HitboxSet : bounds output is null." );
	bool found = false;
	for ( int i = 0; i < boxNumber(); ++i ) {
		Matrix34 box;
		if ( !boxMatrix( i, &box ) ) {
			continue;
		}
		// -1〜1 の立方体を行列で置いた箱を、軸に沿って囲む：中心は移動の列、半分の幅は各行の 3 つの係数の絶対値の和。
		Vector3 center( box.m03, box.m13, box.m23 );
		Vector3 half(
			Math::abs( box.m00 ) + Math::abs( box.m01 ) + Math::abs( box.m02 ),
			Math::abs( box.m10 ) + Math::abs( box.m11 ) + Math::abs( box.m12 ),
			Math::abs( box.m20 ) + Math::abs( box.m21 ) + Math::abs( box.m22 ) );
		Vector3 low;
		Vector3 high;
		low.setSub( center, half );
		high.setAdd( center, half );
		if ( found ) {
			boundsMin->min( low );
			boundsMax->max( high );
		} else {
			*boundsMin = low;
			*boundsMax = high;
			found = true;
		}
	}
	return found;
}
