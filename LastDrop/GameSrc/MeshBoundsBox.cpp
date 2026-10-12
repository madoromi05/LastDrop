#include "MeshBoundsBox.h"
#include "StaticMeshInstance.h"
#include "StaticMeshData.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Matrix34.h"

using namespace GameLib::Math;

namespace{
	const float kHalf = 0.5f;
}

BoxCollider MeshBoundsBox::make( const StaticMeshInstance& mesh ) {
	const StaticMeshData* data = mesh.data();
	// 頂点を囲む箱（モデルのローカル）を、見た目と同じ置き場所・向きでワールドへ置く。
	// 広さは、歩いて越えられない高さの頂点だけで決める（足元の歩道の板まで囲むと、建物の手前を歩けなくなる）。
	const Vector3& low = data->blockingBoundsMin();
	const Vector3& high = data->blockingBoundsMax();
	Vector3 localCenter;
	localCenter.setAdd( low, high );
	localCenter *= kHalf;
	const Matrix34& world = mesh.worldMatrix();

	BoxCollider box;
	world.mul( &box.center, localCenter );
	box.halfWidthX = ( high.x - low.x ) * kHalf * mesh.scale();
	box.halfWidthZ = ( high.z - low.z ) * kHalf * mesh.scale();
	box.angleY = mesh.angleY();
	// 頂点の一番上（地面から）。置いた後の高さを読むので、置き場所の高さと拡大率が入っている。
	Vector3 top;
	world.mul( &top, Vector3( localCenter.x, high.y, localCenter.z ) );
	box.height = top.y;
	return box;
}
