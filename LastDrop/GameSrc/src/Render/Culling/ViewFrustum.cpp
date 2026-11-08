#include "GameSrc/Render/Culling/ViewFrustum.h"

using namespace GameLib::Math;

namespace{
	const int kColumnNumber = 4; // 行列の列の数（x・y・z の係数と、定数）
	// 行列の行の番号。行 r にワールドの点を掛けると、画面の座標（クリップ座標）の x・y・z・w になる。
	const int kRowX = 0;
	const int kRowY = 1;
	const int kRowZ = 2;
	const int kRowW = 3;
	const float kPlus = 1.f;
	const float kMinus = -1.f;
	const float kUnused = 0.f;

	// 面を「行 base ＋ sign × 行 other」の係数で作る。
	struct PlaneSource{
		int base;
		float sign;
		int other;
	};
	// 映る範囲は、クリップ座標で -w <= x <= w、-w <= y <= w、0 <= z <= w（GameLib の射影は奥行きが 0〜w）。
	// それぞれの不等式を「（行の組み合わせ）・点 >= 0」の形に直したもの。
	const PlaneSource kPlaneSources[] = {
		{ kRowW, kPlus,  kRowX }, // 左:   w + x >= 0
		{ kRowW, kMinus, kRowX }, // 右:   w - x >= 0
		{ kRowW, kPlus,  kRowY }, // 下:   w + y >= 0
		{ kRowW, kMinus, kRowY }, // 上:   w - y >= 0
		{ kRowZ, kUnused, kRowZ }, // 手前: z >= 0
		{ kRowW, kMinus, kRowZ }, // 奥:   w - z >= 0
	};
}

ViewFrustum::ViewFrustum( const Matrix44& projectionView ) {
	static_assert( sizeof( kPlaneSources ) / sizeof( kPlaneSources[ 0 ] ) == kPlaneNumber, "kPlaneSources must list every plane" );
	for ( int i = 0; i < kPlaneNumber; ++i ) {
		const PlaneSource& source = kPlaneSources[ i ];
		float coefficient[ kColumnNumber ];
		for ( int column = 0; column < kColumnNumber; ++column ) {
			coefficient[ column ] = projectionView( source.base, column ) + source.sign * projectionView( source.other, column );
		}
		mPlanes[ i ].normal.set( coefficient[ 0 ], coefficient[ 1 ], coefficient[ 2 ] );
		mPlanes[ i ].distance = coefficient[ 3 ];
	}
}

bool ViewFrustum::mayContain( const Vector3& boundsMin, const Vector3& boundsMax ) const {
	for ( const Plane& plane : mPlanes ) {
		// 箱の8隅のうち、面の映る側へ一番寄った隅。それでも映らない側にあるなら、箱はまるごと映らない。
		Vector3 nearest(
			( plane.normal.x >= 0.f ) ? boundsMax.x : boundsMin.x,
			( plane.normal.y >= 0.f ) ? boundsMax.y : boundsMin.y,
			( plane.normal.z >= 0.f ) ? boundsMax.z : boundsMin.z );
		if ( plane.normal.dot( nearest ) + plane.distance < 0.f ) {
			return false;
		}
	}
	return true;
}
