#include "GameSrc/Hit/GroundGrid.h"

#include "GameLib/GameLib.h"

#include <algorithm>
#include <cmath>

using namespace GameLib::Math;

namespace{
	// 線分の時刻（0〜1）の範囲
	const float kSegmentBegin = 0.f;
	const float kSegmentEnd = 1.f;

	// 線分 origin + delta * t の 1 つの軸を、ますの範囲 minValue〜maxValue に入る t へ絞る（スラブ法）
	// 範囲に少しも入らなければ false。begin / end: 絞る t の範囲（入出力）
	bool clipAxis( float origin, float delta, float minValue, float maxValue, float* begin, float* end ) {
		if ( delta == 0.f ) {
			return origin >= minValue && origin <= maxValue; // 軸に平行。範囲の外なら一度も入らない
		}
		float t0 = ( minValue - origin ) / delta;
		float t1 = ( maxValue - origin ) / delta;
		if ( t0 > t1 ) {
			std::swap( t0, t1 );
		}
		*begin = std::max( *begin, t0 );
		*end = std::min( *end, t1 );
		return *begin <= *end;
	}

	// 1 つの軸の、ますをたどる設定（Amanatides & Woo の DDA）
	// step: 隣のますへの向き（−1 / 0 / 1）。nextTime: 次のますの境目に着く t。stepTime: ます 1 つ分進む t
	void setupAxis( float origin, float delta, float gridMin, float cellSize, int cell,
		int* step, float* nextTime, float* stepTime ) {
		if ( delta > 0.f ) {
			*step = 1;
			*nextTime = ( gridMin + ( cell + 1 ) * cellSize - origin ) / delta;
			*stepTime = cellSize / delta;
		} else if ( delta < 0.f ) {
			*step = -1;
			*nextTime = ( gridMin + cell * cellSize - origin ) / delta;
			*stepTime = -cellSize / delta;
		} else {
			*step = 0;
			*nextTime = HUGE_VALF; // この軸では境目に着かない
			*stepTime = HUGE_VALF;
		}
	}
}

GroundGrid::GroundGrid() :
	mMinX( 0.f ),
	mMinZ( 0.f ),
	mCellSize( 1.f ),
	mCountX( 0 ),
	mCountZ( 0 ),
	mQuery( 0 ) {
}

void GroundGrid::reset( const Vector3& boundsMin, const Vector3& boundsMax, float cellSize ) {
	ASSERT( cellSize > 0.f && "GroundGrid : cellSize must be positive." );
	mMinX = boundsMin.x;
	mMinZ = boundsMin.z;
	mCellSize = cellSize;
	// 端がちょうど境目でも、最後のますに入るよう 1 つ足す
	mCountX = static_cast< int >( std::floor( ( boundsMax.x - boundsMin.x ) / cellSize ) ) + 1;
	mCountZ = static_cast< int >( std::floor( ( boundsMax.z - boundsMin.z ) / cellSize ) ) + 1;
	mCells.assign( static_cast< size_t >( mCountX * mCountZ ), std::vector< int >() );
	mVisitedQuery.clear();
	mQuery = 0;
}

void GroundGrid::add( int item, const Vector3& boundsMin, const Vector3& boundsMax ) {
	ASSERT( item >= 0 && "GroundGrid : item must not be negative." );
	ASSERT( mCountX > 0 && "GroundGrid : reset is not called." );
	int beginX = cellIndexX( boundsMin.x );
	int endX = cellIndexX( boundsMax.x );
	int beginZ = cellIndexZ( boundsMin.z );
	int endZ = cellIndexZ( boundsMax.z );
	for ( int z = beginZ; z <= endZ; ++z ) {
		for ( int x = beginX; x <= endX; ++x ) {
			mCells[ static_cast< size_t >( z * mCountX + x ) ].push_back( item );
		}
	}
	if ( static_cast< int >( mVisitedQuery.size() ) <= item ) {
		mVisitedQuery.resize( static_cast< size_t >( item ) + 1, 0 );
	}
}

void GroundGrid::collect( const Vector3& origin, const Vector3& delta, std::vector< int >* items ) const {
	ASSERT( items && "GroundGrid : items is null." );
	items->clear();
	if ( mCells.empty() ) {
		return;
	}
	beginQuery();
	// 線分のうち、ますの範囲の中を通る所だけにする
	float begin = kSegmentBegin;
	float end = kSegmentEnd;
	float maxX = mMinX + mCountX * mCellSize;
	float maxZ = mMinZ + mCountZ * mCellSize;
	if ( !clipAxis( origin.x, delta.x, mMinX, maxX, &begin, &end ) || !clipAxis( origin.z, delta.z, mMinZ, maxZ, &begin, &end ) ) {
		return;
	}
	int cellX = cellIndexX( origin.x + delta.x * begin );
	int cellZ = cellIndexZ( origin.z + delta.z * begin );
	int lastX = cellIndexX( origin.x + delta.x * end );
	int lastZ = cellIndexZ( origin.z + delta.z * end );
	int stepX;
	int stepZ;
	float nextX;
	float nextZ;
	float stepTimeX;
	float stepTimeZ;
	setupAxis( origin.x, delta.x, mMinX, mCellSize, cellX, &stepX, &nextX, &stepTimeX );
	setupAxis( origin.z, delta.z, mMinZ, mCellSize, cellZ, &stepZ, &nextZ, &stepTimeZ );
	// 最初のますから最後のますまで、境目に着く順に隣へ進む。小数の誤差で最後のますを通り越しても止まるよう、回数にも上限を付ける
	int remaining = std::abs( lastX - cellX ) + std::abs( lastZ - cellZ );
	collectCell( cellX, cellZ, items );
	for ( ; remaining > 0; --remaining ) {
		if ( nextX < nextZ ) {
			cellX += stepX;
			nextX += stepTimeX;
		} else {
			cellZ += stepZ;
			nextZ += stepTimeZ;
		}
		if ( cellX < 0 || cellX >= mCountX || cellZ < 0 || cellZ >= mCountZ ) {
			break;
		}
		collectCell( cellX, cellZ, items );
	}
	// 小数の誤差で角をずれて進んだときも、線分の終わりのますは必ず入れる（入れ済みなら印で飛ばされる）
	collectCell( lastX, lastZ, items );
}

void GroundGrid::collectArea( const Vector3& boundsMin, const Vector3& boundsMax, std::vector< int >* items ) const {
	ASSERT( items && "GroundGrid : items is null." );
	items->clear();
	if ( mCells.empty() ) {
		return;
	}
	beginQuery();
	int beginX = cellIndexX( boundsMin.x );
	int endX = cellIndexX( boundsMax.x );
	int beginZ = cellIndexZ( boundsMin.z );
	int endZ = cellIndexZ( boundsMax.z );
	for ( int z = beginZ; z <= endZ; ++z ) {
		for ( int x = beginX; x <= endX; ++x ) {
			collectCell( x, z, items );
		}
	}
}

void GroundGrid::clearItems() {
	for ( std::vector< int >& cell : mCells ) {
		cell.clear(); // 確保した分は残す（毎ティック入れ直すので、確保し直さない）
	}
}

bool GroundGrid::isReady() const {
	return !mCells.empty();
}

void GroundGrid::beginQuery() const {
	++mQuery;
	if ( mQuery == 0 ) { // 一周したら印を全部消す（0 は「まだ入れていない」の意味なので使わない）
		std::fill( mVisitedQuery.begin(), mVisitedQuery.end(), 0u );
		mQuery = 1;
	}
}

void GroundGrid::collectCell( int cellX, int cellZ, std::vector< int >* items ) const {
	for ( int item : mCells[ static_cast< size_t >( cellZ * mCountX + cellX ) ] ) {
		unsigned int& visited = mVisitedQuery[ static_cast< size_t >( item ) ];
		if ( visited == mQuery ) {
			continue;
		}
		visited = mQuery;
		items->push_back( item );
	}
}

int GroundGrid::cellIndexX( float x ) const {
	int cell = static_cast< int >( std::floor( ( x - mMinX ) / mCellSize ) );
	return std::min( std::max( cell, 0 ), mCountX - 1 );
}

int GroundGrid::cellIndexZ( float z ) const {
	int cell = static_cast< int >( std::floor( ( z - mMinZ ) / mCellSize ) );
	return std::min( std::max( cell, 0 ), mCountZ - 1 );
}
