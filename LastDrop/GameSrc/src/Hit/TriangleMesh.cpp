#include "GameSrc/Hit/TriangleMesh.h"
#include "GameSrc/Hit/HitMath.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Functions.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const int kVerticesPerTriangle = 3;

#ifndef NDEBUG
	// 確かめる線分の数。三角形の多いメッシュ（車）でも、総当たりで読み込みが目に見えて遅くならない程度
	const int kVerifyRayCount = 200;
	// 木と総当たりで、当たった時刻がこれ以上違えば別の答えとみなす（計算の順番の違いで出る誤差より十分大きい）
	const float kVerifyTimeTolerance = 1e-4f;
	// 線分の端を選ぶ範囲を、メッシュを囲む箱より外へ広げる割合（外から入る線分・外れる線分も試すため）
	const float kVerifyBoundsSpread = 0.2f;

	// 決まった並びの乱数（毎回同じ線分で確かめる。0〜1）
	class VerifyRandom{
	public:
		VerifyRandom() : mState( 12345u ){}
		float next(){
			mState = mState * 1664525u + 1013904223u; // 線形合同法の、よく使われる係数
			return static_cast< float >( mState >> 8 ) / static_cast< float >( 1u << 24 );
		}
	private:
		unsigned mState;
	};
#endif
}

void TriangleMesh::set( const std::vector< Vector3 >& positions, const std::vector< unsigned short >& indices ) {
	ASSERT( indices.size() % kVerticesPerTriangle == 0 && "TriangleMesh : index count must be a multiple of 3." );
	mPositions = positions;
	mIndices = indices;
	mTree.build( mPositions, mIndices );
#ifndef NDEBUG
	verifyTree();
#endif
}

bool TriangleMesh::isEmpty() const {
	return mIndices.empty();
}

void TriangleMesh::appendTriangles( std::vector< Vector3 >* corners ) const {
	for ( unsigned short index : mIndices ) {
		corners->push_back( mPositions[ index ] );
	}
}

bool TriangleMesh::raycast( const Vector3& origin, const Vector3& delta, float* time, Vector3* normal ) const {
	return mTree.raycast( mPositions, mIndices, origin, delta, time, normal );
}

#ifndef NDEBUG
bool TriangleMesh::raycastAll( const Vector3& origin, const Vector3& delta, float* time, Vector3* normal ) const {
	bool found = false;
	for ( size_t i = 0; i < mIndices.size(); i += kVerticesPerTriangle ) {
		float t;
		Vector3 n;
		if ( !HitMath::segmentTriangle( origin, delta,
			mPositions[ mIndices[ i ] ], mPositions[ mIndices[ i + 1 ] ], mPositions[ mIndices[ i + 2 ] ], &t, &n ) ) {
			continue;
		}
		if ( found && t >= *time ) {
			continue;
		}
		*time = t;
		*normal = n;
		found = true;
	}
	return found;
}

void TriangleMesh::verifyTree() const {
	if ( isEmpty() ) {
		return;
	}
	// 囲む箱を少し広げた範囲から、始点と終点を選ぶ
	Vector3 size;
	size.setSub( mTree.boundsMax(), mTree.boundsMin() );
	Vector3 low;
	low.setMadd( mTree.boundsMin(), size, -kVerifyBoundsSpread );
	Vector3 span;
	span.setMul( size, 1.f + kVerifyBoundsSpread * 2.f );
	VerifyRandom random;
	for ( int i = 0; i < kVerifyRayCount; ++i ) {
		Vector3 from( low.x + span.x * random.next(), low.y + span.y * random.next(), low.z + span.z * random.next() );
		Vector3 to( low.x + span.x * random.next(), low.y + span.y * random.next(), low.z + span.z * random.next() );
		Vector3 delta;
		delta.setSub( to, from );
		float treeTime = 0.f;
		float allTime = 0.f;
		Vector3 normal;
		bool treeFound = mTree.raycast( mPositions, mIndices, from, delta, &treeTime, &normal );
		bool allFound = raycastAll( from, delta, &allTime, &normal );
		ASSERT( treeFound == allFound && "TriangleMesh : the tree and the brute force disagree on a hit." );
		ASSERT( ( !treeFound || Math::abs( treeTime - allTime ) <= kVerifyTimeTolerance ) &&
			"TriangleMesh : the tree and the brute force disagree on the hit time." );
	}
}
#endif
