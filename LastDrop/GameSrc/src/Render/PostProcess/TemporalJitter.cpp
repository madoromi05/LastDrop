#include "GameSrc/Render/PostProcess/TemporalJitter.h"

using namespace GameLib::Math;

namespace{
	const int kBaseX = 2; // Halton 列の底（横と縦で違う素数にすると、2 次元に偏りなく散る）
	const int kBaseY = 3;
	const float kCenter = 0.5f;    // Halton 列（0〜1）を −0.5〜0.5 画素にする
	const float kNdcPerPixel = 2.f; // 画面の端から端までが −1〜1（2）なので、1 画素は 2 / 画素数

	// Halton 列の index 番目（1 から）。index を base 進数で書いた桁を、小数点の反対側へ並べ直した数（0〜1）
	float halton( int index, int base ) {
		float result = 0.f;
		float fraction = 1.f / base;
		for ( int i = index; i > 0; i /= base ) {
			result += fraction * ( i % base );
			fraction /= base;
		}
		return result;
	}
}

TemporalJitter::TemporalJitter() :
	mFrameIndex( 0 ),
	mOffsetX( 0.f ),
	mOffsetY( 0.f ) {
}

void TemporalJitter::advance( bool enabled ) {
	if ( !enabled ) {
		mFrameIndex = 0;
		mOffsetX = 0.f;
		mOffsetY = 0.f;
		return;
	}
	mFrameIndex = ( mFrameIndex + 1 ) % kSampleNumber;
	// 0 番目は (0, 0) になるので 1 番目から使う（1 周の中に「ずらし無し」が入らないように）
	mOffsetX = halton( mFrameIndex + 1, kBaseX ) - kCenter;
	mOffsetY = halton( mFrameIndex + 1, kBaseY ) - kCenter;
}

Matrix44 TemporalJitter::apply( const Matrix44& projectionView, int width, int height ) const {
	// クリップの x・y に「ずらし（画面の −1〜1 の数）× w」を足す。行列の 1 行目・2 行目に 4 行目（w を作る行）を足せば同じになる
	// 画面の y は下が正、クリップの y は上が正なので、y は向きを反転する
	const float ndcX = mOffsetX * kNdcPerPixel / static_cast< float >( width );
	const float ndcY = -mOffsetY * kNdcPerPixel / static_cast< float >( height );
	Matrix44 jittered = projectionView;
	jittered.m00 += ndcX * projectionView.m30;
	jittered.m01 += ndcX * projectionView.m31;
	jittered.m02 += ndcX * projectionView.m32;
	jittered.m03 += ndcX * projectionView.m33;
	jittered.m10 += ndcY * projectionView.m30;
	jittered.m11 += ndcY * projectionView.m31;
	jittered.m12 += ndcY * projectionView.m32;
	jittered.m13 += ndcY * projectionView.m33;
	return jittered;
}

int TemporalJitter::frameIndex() const {
	return mFrameIndex;
}
