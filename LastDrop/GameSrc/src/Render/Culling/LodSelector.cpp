#include "GameSrc/Render/Culling/LodSelector.h"
#include "GameSrc/Render/StaticMesh/StaticMeshLods.h"

namespace{
	LodParams gLodParams;
	const float kHalf = 0.5f;
	const int kCoarsestLevel = StaticMeshLods::kLevelNumber - 1;

	// level 段へ移る境目（画素）。level は 1 以上
	float boundaryOf( int level ) {
		return ( level == 1 ) ? gLodParams.lod1Pixels : gLodParams.lod2Pixels;
	}
}

LodParams& lodParams() {
	return gLodParams;
}

bool& lodEnabled() {
	static bool sEnabled = true; // 起動中ずっと 1 つ（F6 で書き換える）
	return sEnabled;
}

LodSelector::LodSelector() :
	mLevel( 0 ) {
}

int LodSelector::select( float screenPixels ) {
	const float margin = gLodParams.hysteresis * kHalf;
	// 粗くするのは境目より幅の半分だけ小さくなってから、細かくするのは幅の半分だけ大きくなってから
	while ( mLevel < kCoarsestLevel && screenPixels < boundaryOf( mLevel + 1 ) * ( 1.f - margin ) ) {
		++mLevel;
	}
	while ( mLevel > 0 && screenPixels > boundaryOf( mLevel ) * ( 1.f + margin ) ) {
		--mLevel;
	}
	return mLevel;
}

int LodSelector::current() const {
	return mLevel;
}
