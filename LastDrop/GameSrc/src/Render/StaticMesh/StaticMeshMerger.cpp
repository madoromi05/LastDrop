#include "GameSrc/Render/StaticMesh/StaticMeshMerger.h"
#include "GameSrc/Render/StaticMesh/StaticMeshData.h"
#include "GameSrc/Render/MeshLoaderUtil.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Vector2.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Graphics/Vertex.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// source のパーツ1つを、worlds の first 番目から count 枚ぶん置いた形を、destination へ作って GPU へ送る
	// 読んだ頂点で merged の囲む箱を広げる（描くかどうかを決める箱。StaticMeshInstance が使う）
	void mergePart( const StaticMeshPart& sourcePart, const std::vector< Matrix34 >& worlds, int first, int count,
	StaticMeshPart* destination, StaticMeshData* merged ) {
		const int vertexNumber = sourcePart.sourceVertexNumber();
		const int indexNumber = sourcePart.sourceIndexNumber();
		destination->create( vertexNumber * count, indexNumber * count );
		destination->material() = sourcePart.material(); // テクスチャは同じ絵を指す（写しは増えない）
		for ( int instance = 0; instance < count; ++instance ) {
			const Matrix34& world = worlds[ first + instance ];
			const int vertexOffset = instance * vertexNumber;
			for ( int i = 0; i < vertexNumber; ++i ) {
				const Graphics::Vertex& v = sourcePart.sourceVertex( i );
				Vector3 position;
				world.mul( &position, Vector3( v.mPosition.x, v.mPosition.y, v.mPosition.z ) );
				// 法線は向きだけ移す。拡大が入っていると長さが変わるので、長さ1に戻す
				Vector3 normal;
				world.mul33( &normal, v.mNormal );
				normal.normalize();
				destination->setVertex( vertexOffset + i, position, normal, v.mUv );
				destination->setColor( vertexOffset + i, v.mColor );
				merged->extendBounds( position );
			}
			const int indexOffset = instance * indexNumber;
			for ( int i = 0; i < indexNumber; ++i ) {
				destination->setIndex( indexOffset + i, static_cast< unsigned short >( vertexOffset + sourcePart.sourceIndex( i ) ) );	// 1パーツの頂点数は上限以下になるよう count を決めてあるので、16bit に収まる
			}
		}
		destination->upload();
	}

	// 頂点 vertexNumber 個のパーツを、1つのパーツ（インデックスが 16bit）に何枚ぶんまとめられるか
	int instancesPerPart( int vertexNumber ) {
		return MeshLoaderUtil::MAX_VERTEX_NUMBER / vertexNumber;
	}

	// instanceNumber 枚を、1パーツ perPart 枚ずつに分けたときのパーツ数（端数は切り上げ）
	int partNumberFor( int instanceNumber, int perPart ) {
		return ( instanceNumber + perPart - 1 ) / perPart;
	}
}

bool StaticMeshMerger::merge( const StaticMeshData& source, const std::vector< Matrix34 >& worlds, StaticMeshData* merged ) {
	ASSERT( merged && "StaticMeshMerger : merged is null." );
	const int instanceNumber = static_cast< int >( worlds.size() );
	if ( instanceNumber == 0 ) {
		return false; // 置く物が無い（構築に失敗して置けなかった種類もここで終わる）
	}
	ASSERT( source.isReady() && source.keepsSourceVertices() && "StaticMeshMerger : source must be ready and keep its vertices." );
	// 元のパーツ1つが、まとめた後は1つ以上のパーツになる（頂点数が上限を超えるぶんは分ける）。先に総数を数える
	int mergedPartNumber = 0;
	for ( int p = 0; p < source.partNumber(); ++p ) {
		int vertexNumber = source.part( p ).sourceVertexNumber();
		ASSERT( vertexNumber > 0 && "StaticMeshMerger : source part has no vertices kept." );
		mergedPartNumber += partNumberFor( instanceNumber, instancesPerPart( vertexNumber ) );
	}
	merged->create( mergedPartNumber );

	int mergedPart = 0;
	for ( int p = 0; p < source.partNumber(); ++p ) {
		const StaticMeshPart& sourcePart = source.part( p );
		const int perPart = instancesPerPart( sourcePart.sourceVertexNumber() );
		for ( int first = 0; first < instanceNumber; first += perPart ) {
			int count = ( instanceNumber - first < perPart ) ? ( instanceNumber - first ) : perPart;
			mergePart( sourcePart, worlds, first, count, &merged->part( mergedPart ), merged );
			++mergedPart;
		}
	}
	return true;
}
