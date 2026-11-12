#include "GameSrc/DebugMode/TownVisibilityCheck.h"
#include "GameSrc/Map/TownVisibility.h"
#include "GameSrc/Map/Building.h"
#include "GameSrc/Hit/HitWorld.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Vector3.h"

using namespace GameLib::Math;

namespace{
	// 確かめる点の数・目の高さ（m）・建物の面の上に置く点の間隔と高さ（m）
	const int kVerifyPoints = 50;
	const float kVerifyEyeHeight = 1.6f;
	const float kVerifySpacing = 4.f;
	const float kVerifyHeightRatios[] = { 0.2f, 0.5f, 0.85f };	// 面の上の点の高さ（建物の高さに対する割合）。屋根より上の点（建物ではない所）を見ないように、高さの中に収める
	const int kVerifyReportedMisses = 5;						// 出力へ詳しく書く見落としの数
	const float kVerifyOffset = 0.1f;							// 面の点を箱から外へずらす幅（m）
	const unsigned kVerifySeed = 12345u;
	const int kAttemptsPerPoint = 100;							// 建物の足元のますに当たったら引き直す回数の上限（点 1 つあたり）
	const unsigned kRandomSteps = 1000u;						// ますの中の位置を、1 辺をこの数に分けた所から選ぶ
}

void TownVisibilityCheck::run( const TownVisibility& visibility, const std::vector< Building >& buildings, const HitWorld& hitWorld ) {
	if ( !visibility.isReady() ) {
		return;
	}
	unsigned random = kVerifySeed;
	auto next = [ &random ]() { random = random * 1664525u + 1013904223u; return random >> 8; };
	int tested = 0;
	int misses = 0;
	int hiddenPairs = 0;
	for ( int attempt = 0; tested < kVerifyPoints && attempt < kVerifyPoints * kAttemptsPerPoint; ++attempt ) {
		int cell = static_cast< int >( next() % static_cast< unsigned >( visibility.cellCount() ) );
		if ( !visibility.isOpenCell( cell ) ) {
			continue;
		}
		Vector3 boundsMin;
		Vector3 boundsMax;
		visibility.cellBounds( cell, &boundsMin, &boundsMax );
		float cellSize = boundsMax.x - boundsMin.x;
		Vector3 eye( boundsMin.x + cellSize * ( next() % kRandomSteps ) / static_cast< float >( kRandomSteps ), kVerifyEyeHeight,
			boundsMin.z + cellSize * ( next() % kRandomSteps ) / static_cast< float >( kRandomSteps ) );
		if ( visibility.cellAt( eye ) != cell ) {
			continue;
		}
		++tested;
		for ( int b = 0; b < static_cast< int >( buildings.size() ); ++b ) {
			if ( visibility.showsBuilding( cell, b ) ) {
				continue;
			}
			++hiddenPairs;
			// 見られる長方形と高さ（TownVisibility の足元と同じ。建物の見た目を囲む箱）
			const Vector3& low = buildings[ b ].mesh().boundsMin();
			const Vector3& high = buildings[ b ].mesh().boundsMax();
			// 箱の 4 面の上の点へ 3D の線を撃ち、何にも当たらないか、最初に当たるのがその建物なら「3D では見えた」
			bool seen = false;
			Vector3 seenTarget;
			for ( float ratio : kVerifyHeightRatios ) {
				float height = high.y * ratio;
				for ( int side = 0; side < 4 && !seen; ++side ) {
					bool alongX = ( side < 2 );
					float fixed = ( side == 0 ) ? low.z - kVerifyOffset : ( side == 1 ) ? high.z + kVerifyOffset :
						( side == 2 ) ? low.x - kVerifyOffset : high.x + kVerifyOffset;
					float from = alongX ? low.x : low.z;
					float to = alongX ? high.x : high.z;
					for ( float s = from; s <= to && !seen; s += kVerifySpacing ) {
						Vector3 target = alongX ? Vector3( s, height, fixed ) : Vector3( fixed, height, s );
						Vector3 delta;
						delta.setSub( target, eye );
						HitWorld::Hit hit;
						seen = !hitWorld.raycastBlockers( eye, delta, &hit ) || hit.blocker == &buildings[ b ];
						seenTarget = target;
					}
				}
			}
			if ( seen ) {
				if ( misses < kVerifyReportedMisses ) {
					GameLib::cout << "  miss: eye (" << eye.x << ", " << eye.z << ") cell " << cell << " building " << b << " target (" << seenTarget.x
						<< ", " << seenTarget.y << ", " << seenTarget.z << ")" << GameLib::endl;
				}
				++misses;
			}
		}
	}
	GameLib::cout << "TownVisibility verify: " << tested << " points, " << hiddenPairs << " hidden buildings, " << misses
		<< " seen in 3D (should be 0)" << GameLib::endl;
}
