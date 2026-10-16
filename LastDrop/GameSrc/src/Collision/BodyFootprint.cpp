#include "GameSrc/Collision/BodyFootprint.h"
#include "GameSrc/Render/SkinnedMesh/SkinnedMeshData.h"
#include "GameSrc/Math/PlacementMatrix.h"

#include "GameLib/GameLib.h"
#include "GameLib/Base/DebugStream.h"
#include "GameLib/Math/Functions.h"
#include "GameLib/Math/Matrix34.h"
#include <string>
#include <vector>

using namespace GameLib;
using namespace GameLib::Math;


namespace{
	const float kHalf = 0.5f;
	// 対象の頂点が見つからないときの大きさ（m）。人の肩幅・胸の厚みの半分ほど。
	const float kFallbackHalfWidthX = 0.25f;
	const float kFallbackHalfWidthZ = 0.15f;
	// 当たり判定の表示（CollisionDebugDrawer）で描く柱の高さ（m）。人の背丈より少し高くし、ゾンビや自分の体に埋もれないようにする。
	const float kDisplayHeight = 2.f;

	// 囲む頂点の骨（頭の共通部分を除いた名前）。腰・胴・脚。腕・手・頭は含めない。
	// 鎖骨（Shoulder）は胴の一部として入れる（部位の当たり判定の「胴」と同じ範囲）。
	const char* const kFootprintBones[] = {
		"Hips", "Spine", "Spine1", "Spine2", "LeftShoulder", "RightShoulder",
		"LeftUpLeg", "LeftLeg", "LeftFoot", "LeftToeBase",
		"RightUpLeg", "RightLeg", "RightFoot", "RightToeBase",
	};
}

BodyFootprint::BodyFootprint( const SkinnedMeshData* data, const char* boneNamePrefix, float padding ) :
	mLocalCenter( 0.f, 0.f, 0.f ),
	mHalfWidthX( kFallbackHalfWidthX + padding ),
	mHalfWidthZ( kFallbackHalfWidthZ + padding ) {
	ASSERT( data && boneNamePrefix && "BodyFootprint : argument is null." );
	ASSERT( padding >= 0.f && "BodyFootprint : padding is negative." );
	std::vector< bool > mask( data->boneNumber(), false );
	for ( const char* name : kFootprintBones ) {
		int bone = data->findBone( ( std::string( boneNamePrefix ) + name ).c_str() );
		if ( bone >= 0 ) {
			mask[ bone ] = true;
		}
	}
	Vector3 low;
	Vector3 high;
	if ( !data->computeRestBounds( mask, &low, &high ) ) {
		GameLib::cout << "BodyFootprint: no vertex for footprint: " << boneNamePrefix << GameLib::endl;
		return;
	}
	mLocalCenter.set( ( low.x + high.x ) * kHalf, 0.f, ( low.z + high.z ) * kHalf );
	mHalfWidthX = ( high.x - low.x ) * kHalf + padding;
	mHalfWidthZ = ( high.z - low.z ) * kHalf + padding;
}

BoxCollider BodyFootprint::place( const Vector3& position, float angleY ) const {
	return place( position, angleY, mLocalCenter.z + mHalfWidthZ ); // 体の前の辺のまま
}

BoxCollider BodyFootprint::place( const Vector3& position, float angleY, float forwardReach ) const {
	// 前の辺だけを伸ばす点を囲む。中心より手前なら体の中なので、長方形は変わらない（後ろの辺を伸ばさない）。
	Vector3 reach( mLocalCenter.x, 0.f, Math::max( mLocalCenter.z, forwardReach ) );
	return place( position, angleY, reach, reach );
}

BoxCollider BodyFootprint::place( const Vector3& position, float angleY, const Vector3& low, const Vector3& high ) const {
	float right = Math::min( mLocalCenter.x - mHalfWidthX, low.x ); // 骨格の根の空間では右が -X
	float left = Math::max( mLocalCenter.x + mHalfWidthX, high.x );
	float back = Math::min( mLocalCenter.z - mHalfWidthZ, low.z );
	float front = Math::max( mLocalCenter.z + mHalfWidthZ, high.z );
	// 見た目（SkinnedMeshInstance::rootMatrix）と同じ行列で置く。
	Matrix34 root = PlacementMatrix::make( position, angleY );
	BoxCollider box;
	root.mul( &box.center, Vector3( ( right + left ) * kHalf, 0.f, ( back + front ) * kHalf ) );
	box.halfWidthX = ( left - right ) * kHalf;
	box.halfWidthZ = ( front - back ) * kHalf;
	box.angleY = angleY;
	box.height = position.y + kDisplayHeight;
	return box;
}
