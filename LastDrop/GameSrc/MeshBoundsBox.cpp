#include "MeshBoundsBox.h"
#include "StaticMeshData.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Matrix34.h"

using namespace GameLib::Math;

namespace{
	const float kHalf = 0.5f;
}

BoxCollider MeshBoundsBox::make( const StaticMeshData* data, const Vector3& position, float angleY ) {
	ASSERT( data && data->isReady() && "MeshBoundsBox : data is not ready." );
	// 頂点を囲む箱（モデルのローカル）を、見た目と同じ置き場所・向きでワールドへ置く。
	// 広さは、歩いて越えられない高さの頂点だけで決める（足元の歩道の板まで囲むと、建物の手前を歩けなくなる）。
	const Vector3& low = data->blockingBoundsMin();
	const Vector3& high = data->blockingBoundsMax();
	Vector3 localCenter;
	localCenter.setAdd( low, high );
	localCenter *= kHalf;
	Matrix34 world; // StaticMeshInstance::draw と同じ順（移動 * 回転）
	world.setTranslation( position );
	world.rotateY( angleY );

	BoxCollider box;
	world.mul( &box.center, localCenter );
	box.halfWidthX = ( high.x - low.x ) * kHalf;
	box.halfWidthZ = ( high.z - low.z ) * kHalf;
	box.angleY = angleY;
	box.height = position.y + high.y; // 頂点の一番上（地面から）
	return box;
}
