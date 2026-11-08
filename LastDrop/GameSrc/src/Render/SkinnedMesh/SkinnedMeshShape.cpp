#include "GameSrc/Render/SkinnedMesh/SkinnedMeshShape.h"
#include "GameSrc/Render/SkinnedMesh/SkinnedMeshData.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Matrix34.h"

using namespace GameLib::Math;

namespace{
	// 一番強く付いている骨が boneMask で true の頂点を transform で移して out に入れる（前の中身は捨てる）。
	void collectVertices( const SkinnedMeshData& data, const std::vector< bool >& boneMask, const Matrix34& transform, std::vector< Vector3 >* out ) {
		out->clear();
		for ( int i = 0; i < data.vertexNumber(); ++i ) {
			if ( !boneMask[ data.strongestBone( i ) ] ) {
				continue;
			}
			Vector3 local;
			transform.mul( &local, data.restPosition( i ) );
			out->push_back( local );
		}
	}
}

void SkinnedMeshShape::collectBoneSpaceVertices( const SkinnedMeshData& data, const std::vector< bool >& boneMask, int anchorBone, std::vector< Vector3 >* out ) {
	collectVertices( data, boneMask, data.bindInverse( anchorBone ), out );
}

Vector3 SkinnedMeshShape::boneSpaceJointPosition( const SkinnedMeshData& data, int anchorBone, int bone ) {
	// bindInverse の逆＝レスト姿勢での骨の姿勢。その移動成分が関節の位置（モデルの空間）。
	Matrix34 restPose;
	restPose.setInverse( data.bindInverse( bone ) );
	Vector3 joint( restPose.m03, restPose.m13, restPose.m23 );
	Vector3 local;
	data.bindInverse( anchorBone ).mul( &local, joint );
	return local;
}

bool SkinnedMeshShape::computeRestBounds( const SkinnedMeshData& data, const std::vector< bool >& boneMask, Vector3* boundsMin, Vector3* boundsMax ) {
	Matrix34 identity;
	identity.setIdentity();
	std::vector< Vector3 > points;
	collectVertices( data, boneMask, identity, &points );
	if ( points.empty() ) {
		return false;
	}
	*boundsMin = points[ 0 ];
	*boundsMax = points[ 0 ];
	for ( const Vector3& p : points ) {
		boundsMin->min( p );
		boundsMax->max( p );
	}
	return true;
}
