#include "GameSrc/Map/TownVisibility.h"
#include "GameSrc/Map/Building.h"
#include "GameSrc/Collision/MeshWallBuilder.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Functions.h"
#include "GameLib/Profiling/Profiler.h"

#include <algorithm>
#include <cmath>

using namespace GameLib::Math;

namespace{
	// ますの 1 辺（m）。道の幅（約 10.6m）の中に 2〜3 ます入る大きさ
	const float kCellSize = 4.f;
	// 建物を見る点の間隔：ますの 1 辺をこの数に分けた格子の点（ますの中に (kViewSteps+1)² 点）。1 なら四隅だけ
	// 2（9 点）にしても Debug の確かめの見落としは 2 → 1 件にしか減らず、作る時間は 0.18 → 0.48 秒になったので 1 にした
	const int kViewSteps = 1;
	// 建物の辺の上に置く「見られる点」の間隔（m）
	const float kTargetSpacing = 1.f;
	// 見られる点を辺から外へずらす幅（m）。点が自分の長方形の上に乗って、線の端が触れた扱いにならないように
	const float kTargetOffset = 0.05f;
	// 遮る長方形を、体の高さの壁を囲む箱から縮める幅（m）。入口のへこみ・角の欠けの分（壁の外を通る線を遮らないように）
	const float kOccluderInset = 0.5f;
	// ゾンビ（ますどうし）を遮る建物の低さの下限（m）。ゾンビの背（約 1.8m）より高い建物だけが遮る
	const float kCharacterHeight = 2.f;
	// 目がこれより低い建物の上に出たら PVS を使わない分の余裕（m）
	const float kEyeHeightMargin = 0.5f;
	const int kBitsPerWord = 32;
	const float kHalf = 0.5f;
	// ますどうしの線を引く点（四隅）の組の答えを、まだ求めていない印
	const signed char kUnknown = -1;

	int wordsFor( int bitNumber ) {
		return ( bitNumber + kBitsPerWord - 1 ) / kBitsPerWord;
	}

	// 上から見た線分 (ax,az)→(bx,bz) が、長方形 [minX,maxX]×[minZ,maxZ] を通るか（2D のスラブ法）
	bool segmentTouchesRect( float ax, float az, float bx, float bz, float minX, float minZ, float maxX, float maxZ ) {
		float enter = 0.f;
		float leave = 1.f;
		const float origin[ 2 ] = { ax, az };
		const float delta[ 2 ] = { bx - ax, bz - az };
		const float lower[ 2 ] = { minX, minZ };
		const float upper[ 2 ] = { maxX, maxZ };
		for ( int axis = 0; axis < 2; ++axis ) {
			if ( delta[ axis ] == 0.f ) {
				if ( origin[ axis ] < lower[ axis ] || origin[ axis ] > upper[ axis ] ) {
					return false;
				}
				continue;
			}
			float t0 = ( lower[ axis ] - origin[ axis ] ) / delta[ axis ];
			float t1 = ( upper[ axis ] - origin[ axis ] ) / delta[ axis ];
			if ( t0 > t1 ) {
				std::swap( t0, t1 );
			}
			enter = std::max( enter, t0 );
			leave = std::min( leave, t1 );
			if ( enter > leave ) {
				return false;
			}
		}
		return true;
	}

	bool rectContains( float x, float z, float minX, float minZ, float maxX, float maxZ ) {
		return x >= minX && x <= maxX && z >= minZ && z <= maxZ;
	}
}

TownVisibility::TownVisibility() :
	mMinX( 0.f ),
	mMinZ( 0.f ),
	mCountX( 0 ),
	mCountZ( 0 ),
	mMaxEyeHeight( 0.f ),
	mBuildingWords( 0 ),
	mCellWords( 0 ) {
}

void TownVisibility::build( const std::vector< Building >& buildings ) {
	LOAD_SCOPE( "map", "TownVisibility" ); // 0.2 秒を超えたら、ファイルに保存して次から読む作りを足す
	mFootprints.clear();
	mCellOpen.clear();
	mBuildingBits.clear();
	mCellBits.clear();
	mCountX = 0;
	mCountZ = 0;
	if ( buildings.empty() ) {
		return;
	}
	// 段ごとに作る。後の段は前の段の結果を読む
	Vector3 townMin;
	Vector3 townMax;
	buildFootprints( buildings, &townMin, &townMax );
	markOpenCells( townMin, townMax );
	bakeBuildingVisibility();
	bakeCellVisibility();
	int openNumber = static_cast< int >( std::count( mCellOpen.begin(), mCellOpen.end(), true ) );
	GameLib::cout << "TownVisibility: " << openNumber << " / " << cellCount() << " cells, " << static_cast< int >( mFootprints.size() ) << " buildings" << GameLib::endl;
}

void TownVisibility::buildFootprints( const std::vector< Building >& buildings, Vector3* townMin, Vector3* townMax ) {
	// ── 建物の足元と、遮る長方形のます ──
	*townMin = buildings.front().mesh().boundsMin();
	*townMax = buildings.front().mesh().boundsMax();
	mMaxEyeHeight = buildings.front().mesh().boundsMax().y;
	for ( const Building& building : buildings ) {
		const Vector3& boundsMin = building.mesh().boundsMin();
		const Vector3& boundsMax = building.mesh().boundsMax();
		Footprint footprint;
		footprint.minX = boundsMin.x;
		footprint.minZ = boundsMin.z;
		footprint.maxX = boundsMax.x;
		footprint.maxZ = boundsMax.z;
		footprint.height = boundsMax.y;
		WallMesh walls = MeshWallBuilder::make( building.mesh() ); // 移動の当たり判定と同じ作り方の、体の高さの壁
		footprint.blockMinX = walls.boundsMin().x + kOccluderInset;
		footprint.blockMinZ = walls.boundsMin().z + kOccluderInset;
		footprint.blockMaxX = walls.boundsMax().x - kOccluderInset;
		footprint.blockMaxZ = walls.boundsMax().z - kOccluderInset;
		mFootprints.push_back( footprint );
		townMin->x = std::min( townMin->x, boundsMin.x );
		townMin->z = std::min( townMin->z, boundsMin.z );
		townMax->x = std::max( townMax->x, boundsMax.x );
		townMax->z = std::max( townMax->z, boundsMax.z );
		mMaxEyeHeight = std::min( mMaxEyeHeight, boundsMax.y );
	}
	mMaxEyeHeight -= kEyeHeightMargin;
	mOccluders.reset( *townMin, *townMax, kCellSize );
	for ( int i = 0; i < static_cast< int >( mFootprints.size() ); ++i ) {
		const Footprint& f = mFootprints[ i ];
		mOccluders.add( i, Vector3( f.blockMinX, 0.f, f.blockMinZ ), Vector3( f.blockMaxX, 0.f, f.blockMaxZ ) );
	}
}

void TownVisibility::markOpenCells( const Vector3& townMin, const Vector3& townMax ) {
	// ── ます ──
	mMinX = townMin.x;
	mMinZ = townMin.z;
	mCountX = static_cast< int >( std::ceil( ( townMax.x - townMin.x ) / kCellSize ) );
	mCountZ = static_cast< int >( std::ceil( ( townMax.z - townMin.z ) / kCellSize ) );
	mCellOpen.assign( cellCount(), false );
	for ( int z = 0; z < mCountZ; ++z ) {
		for ( int x = 0; x < mCountX; ++x ) {
			float centerX = mMinX + ( x + kHalf ) * kCellSize;
			float centerZ = mMinZ + ( z + kHalf ) * kCellSize;
			bool open = true;
			for ( const Footprint& f : mFootprints ) {
				if ( rectContains( centerX, centerZ, f.blockMinX, f.blockMinZ, f.blockMaxX, f.blockMaxZ ) ) {
					open = false;
					break;
				}
			}
			mCellOpen[ cellIndex( x, z ) ] = open;
		}
	}
}

void TownVisibility::bakeBuildingVisibility() {
	// ── 見る点（ますを kViewSteps に分けた格子の点）ごとの、見えうる建物。隣り合うますで同じ点を使うので 1 回だけ求める ──
	const int cellNumber = cellCount();
	const int buildingNumber = static_cast< int >( mFootprints.size() );
	const float viewStep = kCellSize / kViewSteps;
	const int viewCountX = mCountX * kViewSteps + 1;
	const int viewNumber = viewCountX * ( mCountZ * kViewSteps + 1 );
	std::vector< bool > viewUsed( viewNumber, false );
	for ( int z = 0; z < mCountZ; ++z ) {
		for ( int x = 0; x < mCountX; ++x ) {
			if ( !mCellOpen[ cellIndex( x, z ) ] ) {
				continue;
			}
			for ( int sz = 0; sz <= kViewSteps; ++sz ) {
				for ( int sx = 0; sx <= kViewSteps; ++sx ) {
					viewUsed[ ( z * kViewSteps + sz ) * viewCountX + x * kViewSteps + sx ] = true;
				}
			}
		}
	}
	mBuildingWords = wordsFor( buildingNumber );
	std::vector< std::uint32_t > viewBuildings( static_cast< size_t >( viewNumber ) * mBuildingWords, 0u );
	for ( int v = 0; v < viewNumber; ++v ) {
		if ( !viewUsed[ v ] ) {
			continue;
		}
		float x = mMinX + ( v % viewCountX ) * viewStep;
		float z = mMinZ + ( v / viewCountX ) * viewStep;
		for ( int b = 0; b < buildingNumber; ++b ) {
			if ( seesBuilding( x, z, b ) ) {
				setBit( &viewBuildings, v, mBuildingWords, b );
			}
		}
	}
	// ますの見えうる建物 ＝ ますの中の見る点のどれかから見えうる建物（ビットを語ごとに足し合わせる）
	mBuildingBits.assign( static_cast< size_t >( cellNumber ) * mBuildingWords, 0u );
	for ( int z = 0; z < mCountZ; ++z ) {
		for ( int x = 0; x < mCountX; ++x ) {
			int cell = cellIndex( x, z );
			if ( !mCellOpen[ cell ] ) {
				continue;
			}
			for ( int sz = 0; sz <= kViewSteps; ++sz ) {
				for ( int sx = 0; sx <= kViewSteps; ++sx ) {
					int v = ( z * kViewSteps + sz ) * viewCountX + x * kViewSteps + sx;
					for ( int w = 0; w < mBuildingWords; ++w ) {
						mBuildingBits[ static_cast< size_t >( cell ) * mBuildingWords + w ] |= viewBuildings[ static_cast< size_t >( v ) * mBuildingWords + w ];
					}
				}
			}
		}
	}
}

void TownVisibility::bakeCellVisibility() {
	// ── ますどうし（ゾンビ用）。四隅のどれかどうしが見通せれば見えうる。点の組の答えは覚えて使い回す ──
	const int cellNumber = cellCount();
	const int vertexCountX = mCountX + 1;
	const int vertexNumber = vertexCountX * ( mCountZ + 1 );
	mCellWords = wordsFor( cellNumber );
	mCellBits.assign( static_cast< size_t >( cellNumber ) * mCellWords, 0u );
	std::vector< signed char > vertexPairs( static_cast< size_t >( vertexNumber ) * vertexNumber, kUnknown );
	auto vertexSees = [ & ]( int a, int b ) {
		signed char& known = vertexPairs[ static_cast< size_t >( a ) * vertexNumber + b ];
		if ( known == kUnknown ) {
			float ax = mMinX + ( a % vertexCountX ) * kCellSize;
			float az = mMinZ + ( a / vertexCountX ) * kCellSize;
			float bx = mMinX + ( b % vertexCountX ) * kCellSize;
			float bz = mMinZ + ( b / vertexCountX ) * kCellSize;
			known = isBlocked( ax, az, bx, bz, kCharacterHeight, kNoCell ) ? 0 : 1;
			vertexPairs[ static_cast< size_t >( b ) * vertexNumber + a ] = known;
		}
		return known == 1;
	};
	for ( int a = 0; a < cellNumber; ++a ) {
		if ( !mCellOpen[ a ] ) {
			continue;
		}
		int ax = a % mCountX;
		int az = a / mCountX;
		const int cornersA[ 4 ] = { az * vertexCountX + ax, az * vertexCountX + ax + 1, ( az + 1 ) * vertexCountX + ax, ( az + 1 ) * vertexCountX + ax + 1 };
		for ( int b = 0; b < cellNumber; ++b ) {
			// 建物の中のます（ゾンビは入れない）は表に載せない。showsPoint は、ゾンビ自身のますが閉じていれば見える扱いにする
			if ( !mCellOpen[ b ] ) {
				continue;
			}
			int bx = b % mCountX;
			int bz = b / mCountX;
			const int cornersB[ 4 ] = { bz * vertexCountX + bx, bz * vertexCountX + bx + 1, ( bz + 1 ) * vertexCountX + bx, ( bz + 1 ) * vertexCountX + bx + 1 };
			bool visible = false;
			for ( int i = 0; i < 4 && !visible; ++i ) {
				for ( int j = 0; j < 4 && !visible; ++j ) {
					visible = ( cornersA[ i ] == cornersB[ j ] ) || vertexSees( cornersA[ i ], cornersB[ j ] );
				}
			}
			if ( visible ) {
				setBit( &mCellBits, a, mCellWords, b );
			}
		}
	}
}

int TownVisibility::cellCount() const {
	return mCountX * mCountZ;
}

bool TownVisibility::isOpenCell( int cell ) const {
	ASSERT( cell >= 0 && cell < cellCount() && "TownVisibility::isOpenCell : bad cell." );
	return mCellOpen[ cell ];
}

bool TownVisibility::isReady() const {
	return mCountX > 0;
}

int TownVisibility::cellAt( const Vector3& eye ) const {
	if ( !isReady() || eye.y > mMaxEyeHeight ) {
		return kNoCell;
	}
	int cell = cellIndex( static_cast< int >( std::floor( ( eye.x - mMinX ) / kCellSize ) ),
		static_cast< int >( std::floor( ( eye.z - mMinZ ) / kCellSize ) ) );
	return ( cell != kNoCell && mCellOpen[ cell ] ) ? cell : kNoCell;
}

bool TownVisibility::showsBuilding( int viewCell, int building ) const {
	if ( viewCell == kNoCell ) {
		return true;
	}
	return hasBit( mBuildingBits, viewCell, mBuildingWords, building );
}

bool TownVisibility::showsPoint( int viewCell, const Vector3& position ) const {
	if ( viewCell == kNoCell ) {
		return true;
	}
	int cellX = static_cast< int >( std::floor( ( position.x - mMinX ) / kCellSize ) );
	int cellZ = static_cast< int >( std::floor( ( position.z - mMinZ ) / kCellSize ) );
	int own = cellIndex( cellX, cellZ );
	if ( own == kNoCell || !mCellOpen[ own ] ) {
		return true; // 町の外・建物の足元にいる物は、表に無いので描く（多めに描く側）
	}
	for ( int dz = -1; dz <= 1; ++dz ) {
		for ( int dx = -1; dx <= 1; ++dx ) {
			int cell = cellIndex( cellX + dx, cellZ + dz );
			if ( cell != kNoCell && hasBit( mCellBits, viewCell, mCellWords, cell ) ) {
				return true;
			}
		}
	}
	return false;
}

void TownVisibility::cellBounds( int cell, Vector3* boundsMin, Vector3* boundsMax ) const {
	ASSERT( boundsMin && boundsMax && "TownVisibility : output is null." );
	ASSERT( cell >= 0 && cell < mCountX * mCountZ && "TownVisibility : bad cell." );
	float x = mMinX + ( cell % mCountX ) * kCellSize;
	float z = mMinZ + ( cell / mCountX ) * kCellSize;
	boundsMin->set( x, 0.f, z );
	boundsMax->set( x + kCellSize, 0.f, z + kCellSize );
}

bool TownVisibility::isBlocked( float ax, float az, float bx, float bz, float minHeight, int ignore ) const {
	mOccluders.collect( Vector3( ax, 0.f, az ), Vector3( bx - ax, 0.f, bz - az ), &mCandidates );
	for ( int index : mCandidates ) {
		const Footprint& f = mFootprints[ index ];
		if ( index == ignore || f.height < minHeight ) {
			continue;
		}
		float minX = f.blockMinX;
		float minZ = f.blockMinZ;
		float maxX = f.blockMaxX;
		float maxZ = f.blockMaxZ;
		// 線の端が長方形の中にある（ますの隅が建物の足元に入った）なら、その建物では遮らない（見える物を消さない側）
		if ( rectContains( ax, az, minX, minZ, maxX, maxZ ) || rectContains( bx, bz, minX, minZ, maxX, maxZ ) ) {
			continue;
		}
		if ( segmentTouchesRect( ax, az, bx, bz, minX, minZ, maxX, maxZ ) ) {
			return true;
		}
	}
	return false;
}

bool TownVisibility::seesBuilding( float x, float z, int building ) const {
	const Footprint& f = mFootprints[ building ];
	if ( rectContains( x, z, f.minX, f.minZ, f.maxX, f.maxZ ) ) {
		return true; // 見る点が建物の足元の中（庇の下など）
	}
	// 見る点を向いた辺（最大 2 本）の上に、間隔ごとの点を置いて線を引く
	struct Edge{
		float ax, az, bx, bz;
	};
	Edge edges[ 2 ];
	int edgeNumber = 0;
	if ( x < f.minX ) {
		edges[ edgeNumber++ ] = { f.minX - kTargetOffset, f.minZ, f.minX - kTargetOffset, f.maxZ };
	} else if ( x > f.maxX ) {
		edges[ edgeNumber++ ] = { f.maxX + kTargetOffset, f.minZ, f.maxX + kTargetOffset, f.maxZ };
	}
	if ( z < f.minZ ) {
		edges[ edgeNumber++ ] = { f.minX, f.minZ - kTargetOffset, f.maxX, f.minZ - kTargetOffset };
	} else if ( z > f.maxZ ) {
		edges[ edgeNumber++ ] = { f.minX, f.maxZ + kTargetOffset, f.maxX, f.maxZ + kTargetOffset };
	}
	for ( int e = 0; e < edgeNumber; ++e ) {
		const Edge& edge = edges[ e ];
		float length = std::sqrt( ( edge.bx - edge.ax ) * ( edge.bx - edge.ax ) + ( edge.bz - edge.az ) * ( edge.bz - edge.az ) );
		int steps = std::max( 1, static_cast< int >( std::ceil( length / kTargetSpacing ) ) );
		for ( int s = 0; s <= steps; ++s ) {
			float t = static_cast< float >( s ) / steps;
			float tx = edge.ax + ( edge.bx - edge.ax ) * t;
			float tz = edge.az + ( edge.bz - edge.az ) * t;
			if ( !isBlocked( x, z, tx, tz, f.height, building ) ) {
				return true;
			}
		}
	}
	return false;
}

int TownVisibility::cellIndex( int cellX, int cellZ ) const {
	if ( cellX < 0 || cellX >= mCountX || cellZ < 0 || cellZ >= mCountZ ) {
		return kNoCell;
	}
	return cellZ * mCountX + cellX;
}

bool TownVisibility::hasBit( const std::vector< std::uint32_t >& bits, int row, int rowWords, int column ) const {
	return ( bits[ static_cast< size_t >( row ) * rowWords + column / kBitsPerWord ] >> ( column % kBitsPerWord ) ) & 1u;
}

void TownVisibility::setBit( std::vector< std::uint32_t >* bits, int row, int rowWords, int column ) {
	( *bits )[ static_cast< size_t >( row ) * rowWords + column / kBitsPerWord ] |= 1u << ( column % kBitsPerWord );
}

bool& pvsEnabled() {
	static bool sEnabled = true; // 起動中ずっと 1 つ（F6 で書き換える）
	return sEnabled;
}
