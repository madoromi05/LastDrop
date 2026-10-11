#include "BodyFootprint.h"
#include "SkinnedMeshData.h"

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
	float back = mLocalCenter.z - mHalfWidthZ;
	float front = Math::max( mLocalCenter.z + mHalfWidthZ, forwardReach );
	// 見た目（SkinnedMeshInstance::rootMatrix）と同じ 移動 * 回転 で置く。
	Matrix34 root;
	root.setTranslation( position );
	root.rotateY( angleY );
	BoxCollider box;
	root.mul( &box.center, Vector3( mLocalCenter.x, 0.f, ( back + front ) * kHalf ) );
	box.halfWidthX = mHalfWidthX;
	box.halfWidthZ = ( front - back ) * kHalf;
	box.angleY = angleY;
	box.height = position.y + kDisplayHeight;
	return box;
}
