#include "GameSrc/Hit/TriangleBvh.h"
#include "GameSrc/Hit/HitMath.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Functions.h"
#include "GameLib/Profiling/Profiler.h"

#include <algorithm>

using namespace GameLib::Math;

namespace{
	const int kVerticesPerTriangle = 3;
	const int kAxisNumber = 3;
	const int kRootNode = 0;
	// 葉に入れる三角形の数の上限。小さいほど木は深くなるが、葉で調べる三角形は減る（4 前後がよく使われる値）。
	const int kMaxLeafTriangles = 4;
	// 下りる途中で覚えておく節の数の上限。中央値で分けるので深さは log2(三角形の数 / 4) 程度
	// （unsigned short のつながりでは三角形は最大 2 万強で、深さ 13 前後）。余裕を見て大きめにする。
	const int kMaxStackDepth = 64;
	// 節の箱を三角形からこれだけ広げる（m）。ぴったりの箱だと、三角形の縁にちょうど当たる線を、浮動小数の誤差で箱の方が外してしまい、
	// 総当たりと答えが食い違う。当たる三角形は変わらず、箱を調べる数がわずかに増えるだけ。
	const float kBoundsPadding = 0.001f;

	// 三角形 triangle の 3 つの頂点。
	void corners( const std::vector< Vector3 >& positions, const std::vector< unsigned short >& indices, int triangle,
		const Vector3** v0, const Vector3** v1, const Vector3** v2 ) {
		int base = triangle * kVerticesPerTriangle;
		*v0 = &positions[ indices[ base ] ];
		*v1 = &positions[ indices[ base + 1 ] ];
		*v2 = &positions[ indices[ base + 2 ] ];
	}
}

void TriangleBvh::build( const std::vector< Vector3 >& positions, const std::vector< unsigned short >& indices ) {
	mNodes.clear();
	mTriangles.clear();
	int triangleCount = static_cast< int >( indices.size() ) / kVerticesPerTriangle;
	if ( triangleCount == 0 ) {
		return;
	}
	std::vector< Vector3 > centers( triangleCount );
	const float kThird = 1.f / kVerticesPerTriangle;
	for ( int i = 0; i < triangleCount; ++i ) {
		const Vector3* v0;
		const Vector3* v1;
		const Vector3* v2;
		corners( positions, indices, i, &v0, &v1, &v2 );
		centers[ i ].setAdd( *v0, *v1 );
		centers[ i ] += *v2;
		centers[ i ] *= kThird;
		mTriangles.push_back( i );
	}
	// 二分木の節の数は「葉の数×2−1」を超えない。先に確保して、分ける途中で並びが動かないようにする。
	mNodes.reserve( static_cast< size_t >( triangleCount ) * 2 );
	mNodes.push_back( Node() );
	split( kRootNode, 0, triangleCount, positions, indices, centers );
}

void TriangleBvh::split( int nodeIndex, int first, int count, const std::vector< Vector3 >& positions,
const std::vector< unsigned short >& indices, const std::vector< Vector3 >& centers ) {
	// 受け持つ三角形を全部囲む箱と、中心だけを囲む箱（分ける向きを決める）。
	Vector3 boundsMin( FLOAT_MAX );
	Vector3 boundsMax( -FLOAT_MAX );
	Vector3 centerMin( FLOAT_MAX );
	Vector3 centerMax( -FLOAT_MAX );
	for ( int i = first; i < first + count; ++i ) {
		const Vector3* v0;
		const Vector3* v1;
		const Vector3* v2;
		corners( positions, indices, mTriangles[ i ], &v0, &v1, &v2 );
		boundsMin.min( *v0 );
		boundsMin.min( *v1 );
		boundsMin.min( *v2 );
		boundsMax.max( *v0 );
		boundsMax.max( *v1 );
		boundsMax.max( *v2 );
		centerMin.min( centers[ mTriangles[ i ] ] );
		centerMax.max( centers[ mTriangles[ i ] ] );
	}
	Node& node = mNodes[ nodeIndex ];
	const Vector3 padding( kBoundsPadding, kBoundsPadding, kBoundsPadding );
	node.boundsMin.setSub( boundsMin, padding );
	node.boundsMax.setAdd( boundsMax, padding );

	Vector3 extent;
	extent.setSub( centerMax, centerMin );
	int axis = 0;
	for ( int a = 1; a < kAxisNumber; ++a ) {
		if ( extent[ a ] > extent[ axis ] ) {
			axis = a;
		}
	}
	// 少なければ葉にする。中心が全部同じ所にある（分けても片方が空になる）ときも葉にする。
	if ( count <= kMaxLeafTriangles || extent[ axis ] <= 0.f ) {
		node.firstChildOrTriangle = first;
		node.triangleCount = count;
		return;
	}
	// 中央値で分ける：真ん中の位置に来る三角形だけを決め、手前に小さい側・後ろに大きい側を集める（全部は並べない）。
	int half = count / 2;
	std::vector< int >::iterator begin = mTriangles.begin() + first;
	std::nth_element( begin, begin + half, begin + count, [ &centers, axis ]( int a, int b ) {
		return centers[ a ][ axis ] < centers[ b ][ axis ];
	} );
	int leftChild = static_cast< int >( mNodes.size() );
	node.firstChildOrTriangle = leftChild;
	node.triangleCount = 0;
	// push_back の後は node（参照）を使わない（reserve してあるので動かないが、念のため）。
	mNodes.push_back( Node() );
	mNodes.push_back( Node() );
	split( leftChild, first, half, positions, indices, centers );
	split( leftChild + 1, first + half, count - half, positions, indices, centers );
}

bool TriangleBvh::isEmpty() const {
	return mNodes.empty();
}

const Vector3& TriangleBvh::boundsMin() const {
	return mNodes[ kRootNode ].boundsMin;
}

const Vector3& TriangleBvh::boundsMax() const {
	return mNodes[ kRootNode ].boundsMax;
}

bool TriangleBvh::raycast( const std::vector< Vector3 >& positions, const std::vector< unsigned short >& indices,
const Vector3& origin, const Vector3& delta, float* time, Vector3* normal ) const {
	if ( isEmpty() ) {
		return false;
	}
	// 下りる途中の節と、その箱に入る時刻。見つけた当たりより遠い箱は、取り出した時に飛ばす。
	struct Pending{
		int node;
		float enterTime;
	};
	Pending stack[ kMaxStackDepth ];
	int depth = 0;
	float rootEnter;
	PROFILE_COUNT( "Hit.nodes", 1 ); // 調べた箱の数
	if ( !HitMath::segmentBoxEnterTime( origin, delta, boundsMin(), boundsMax(), &rootEnter ) ) {
		return false;
	}
	stack[ depth++ ] = Pending{ kRootNode, rootEnter };
	bool found = false;
	while ( depth > 0 ) {
		Pending pending = stack[ --depth ];
		if ( found && pending.enterTime > *time ) {
			continue;
		}
		const Node& node = mNodes[ pending.node ];
		if ( node.triangleCount > 0 ) {
			PROFILE_COUNT( "Hit.triangles", static_cast< double >( node.triangleCount ) ); // 調べる三角形の数
			for ( int i = node.firstChildOrTriangle; i < node.firstChildOrTriangle + node.triangleCount; ++i ) {
				const Vector3* v0;
				const Vector3* v1;
				const Vector3* v2;
				corners( positions, indices, mTriangles[ i ], &v0, &v1, &v2 );
				float t;
				Vector3 n;
				if ( !HitMath::segmentTriangle( origin, delta, *v0, *v1, *v2, &t, &n ) ) {
					continue;
				}
				if ( found && t >= *time ) {
					continue;
				}
				*time = t;
				*normal = n;
				found = true;
			}
			continue;
		}
		// 子の 2 つのうち、線分が先に入る方を後に積む（先に取り出して調べ、早く当たりを見つけて遠い方を飛ばすため）。
		Pending children[ 2 ];
		int childCount = 0;
		for ( int c = 0; c < 2; ++c ) {
			int childIndex = node.firstChildOrTriangle + c;
			const Node& child = mNodes[ childIndex ];
			float enter;
			PROFILE_COUNT( "Hit.nodes", 1 );
			if ( !HitMath::segmentBoxEnterTime( origin, delta, child.boundsMin, child.boundsMax, &enter ) ) {
				continue;
			}
			if ( found && enter > *time ) {
				continue;
			}
			children[ childCount++ ] = Pending{ childIndex, enter };
		}
		if ( childCount == 2 && children[ 0 ].enterTime < children[ 1 ].enterTime ) {
			std::swap( children[ 0 ], children[ 1 ] );
		}
		ASSERT( depth + childCount <= kMaxStackDepth && "TriangleBvh : tree is deeper than the stack." );
		for ( int c = 0; c < childCount; ++c ) {
			stack[ depth++ ] = children[ c ];
		}
	}
	return found;
}
