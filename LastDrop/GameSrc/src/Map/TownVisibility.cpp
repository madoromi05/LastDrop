#include "GameSrc/Map/TownVisibility.h"
#include "GameSrc/Map/Building.h"
#include "GameSrc/Collision/MeshWallBuilder.h"
#include "GameSrc/Math/ScalarMath.h"
#include "GameSrc/Render/StaticMesh/StaticMeshInstance.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Functions.h"
#include "GameLib/Profiling/Profiler.h"

#include <algorithm>
#include <cmath>
#include <cstring>

using namespace GameLib::Math;

namespace{
	const float kCellSize = 4.f;		// ますの 1 辺（m）。道の幅（約 10.6m）の中に 2〜3 ます入る大きさ
	// 建物を見る点の間隔：ますの 1 辺をこの数に分けた格子の点（ますの中に (kViewSteps+1)² 点）。1 なら四隅だけ
	// 2（9 点）にしても Debug の確かめの見落としは 2 → 1 件にしか減らず、作る時間は 0.18 → 0.48 秒に増えるので 1
	const int kViewSteps = 1;
	const float kTargetSpacing = 1.f;	// 建物の辺の上に置く「見られる点」の間隔（m）
	const float kTargetOffset = 0.05f;	// 見られる点を辺から外へずらす幅（m）。点が自分の長方形の上に乗って、線の端が触れた扱いにならないように
	const float kOccluderInset = 0.5f;	// 遮る長方形を、体の高さの壁を囲む箱から縮める幅（m）。入口のへこみ・角の欠けの分（壁の外を通る線を遮らないように）
	const float kCharacterHeight = 2.f;	// ゾンビ（ますどうし）を遮る建物の低さの下限（m）。ゾンビの背（約 1.8m）より高い建物だけが遮る
	// 目がこれより低い建物の上に出たら PVS を使わない分の余裕（m）
	const float kEyeHeightMargin = 0.5f;
	const int kBitsPerWord = 32;
	const float kHalf = 0.5f;
	const signed char kUnknown = -1;	// ますどうしの線を引く点（四隅）の組の答えを、まだ求めていない印
	const int kRectCorners = 4;

	// 表のバイト列の始めに並べる数（std::int32_t）の順番。読むときに、今の建物から求めた数と比べる
	// 数の後に、ますごとの見えうる建物のビット、ますごとの見えうるますのビットが続く
	enum TableHead{
		TABLE_HEAD_COUNT_X,
		TABLE_HEAD_COUNT_Z,
		TABLE_HEAD_BUILDINGS,
		TABLE_HEAD_BUILDING_WORDS,
		TABLE_HEAD_CELL_WORDS,
		TABLE_HEAD_NUMBER,
	};

	int wordsFor( int bitNumber ) {
		return ( bitNumber + kBitsPerWord - 1 ) / kBitsPerWord;
	}

	void appendBytes( const void* source, size_t size, std::vector< unsigned char >* out ) {	// size バイトを、メモリの並びのまま（x64 のリトルエンディアン）out の後ろへ足す
		const unsigned char* bytes = static_cast< const unsigned char* >( source );
		out->insert( out->end(), bytes, bytes + size );
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

void TownVisibility::reset() {
	mFootprints.clear();
	mCellOpen.clear();
	mBuildingBits.clear();
	mCellBits.clear();
	mCountX = 0;
	mCountZ = 0;
	mBuildingWords = 0;
	mCellWords = 0;
}

void TownVisibility::build( const std::vector< Building >& buildings ) {
	LOAD_SCOPE( "map", "TownVisibility" );
	reset();
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

void TownVisibility::saveTables( std::vector< unsigned char >* out ) const {
	ASSERT( out && "TownVisibility::saveTables : out is null." );
	out->clear();
	if ( !isReady() ) {
		return;
	}
	const std::int32_t head[ TABLE_HEAD_NUMBER ] = { mCountX, mCountZ, static_cast< std::int32_t >( mFootprints.size() ), mBuildingWords, mCellWords };
	appendBytes( head, sizeof( head ), out );
	appendBytes( mBuildingBits.data(), mBuildingBits.size() * sizeof( std::uint32_t ), out );
	appendBytes( mCellBits.data(), mCellBits.size() * sizeof( std::uint32_t ), out );
}

bool TownVisibility::loadTables( const std::vector< Building >& buildings, const std::vector< unsigned char >& data ) {
	LOAD_SCOPE( "map", "TownVisibility::load" );
	reset();
	if ( buildings.empty() ) {
		return false;
	}
	// 建物から求める段だけ行い、表のビットはバイト列から写す。頭の数は、今の建物から求めた数と比べる
	Vector3 townMin;
	Vector3 townMax;
	buildFootprints( buildings, &townMin, &townMax );
	markOpenCells( townMin, townMax );
	const int buildingNumber = static_cast< int >( mFootprints.size() );
	const int buildingWords = wordsFor( buildingNumber );
	const int cellWords = wordsFor( cellCount() );
	const size_t buildingBitsNumber = static_cast< size_t >( cellCount() ) * buildingWords;
	const size_t cellBitsNumber = static_cast< size_t >( cellCount() ) * cellWords;
	std::int32_t head[ TABLE_HEAD_NUMBER ] = {};
	const size_t headBytes = sizeof( head );
	const size_t buildingBytes = buildingBitsNumber * sizeof( std::uint32_t );
	const size_t cellBytes = cellBitsNumber * sizeof( std::uint32_t );
	bool matches = ( data.size() == headBytes + buildingBytes + cellBytes );
	if ( matches ) {
		std::memcpy( head, data.data(), headBytes );
		matches = head[ TABLE_HEAD_COUNT_X ] == mCountX && head[ TABLE_HEAD_COUNT_Z ] == mCountZ &&
			head[ TABLE_HEAD_BUILDINGS ] == buildingNumber &&
			head[ TABLE_HEAD_BUILDING_WORDS ] == buildingWords && head[ TABLE_HEAD_CELL_WORDS ] == cellWords;
	}
	if ( !matches ) {
		reset();
		GameLib::cout << "TownVisibility: the saved table does not match the buildings" << GameLib::endl;
		return false;
	}
	mBuildingWords = buildingWords;
	mCellWords = cellWords;
	mBuildingBits.resize( buildingBitsNumber );
	mCellBits.resize( cellBitsNumber );
	std::memcpy( mBuildingBits.data(), data.data() + headBytes, buildingBytes );
	std::memcpy( mCellBits.data(), data.data() + headBytes + buildingBytes, cellBytes );
	int openNumber = static_cast< int >( std::count( mCellOpen.begin(), mCellOpen.end(), true ) );
	GameLib::cout << "TownVisibility: loaded " << openNumber << " / " << cellCount() << " cells, " << buildingNumber << " buildings" << GameLib::endl;
	return true;
}

void TownVisibility::Footprint::toLocal( float x, float z, float* u, float* v ) const {
	float dx = x - centerX;
	float dz = z - centerZ;
	*u = cosYaw * dx - sinYaw * dz; // toWorld の逆。回す行列は直交なので、逆は転置
	*v = sinYaw * dx + cosYaw * dz;
}

void TownVisibility::Footprint::toWorld( float u, float v, float* x, float* z ) const {
	*x = cosYaw * u + sinYaw * v + centerX; // PlacementMatrix（移動 * Y 軸まわりの回転）が、モデルの点を world へ置くのと同じ式
	*z = -sinYaw * u + cosYaw * v + centerZ;
}

void TownVisibility::Footprint::blockBounds( Vector3* boundsMin, Vector3* boundsMax ) const {
	const float cornerU[ kRectCorners ] = { blockMinU, blockMaxU, blockMaxU, blockMinU };
	const float cornerV[ kRectCorners ] = { blockMinV, blockMinV, blockMaxV, blockMaxV };
	for ( int i = 0; i < kRectCorners; ++i ) {
		float x;
		float z;
		toWorld( cornerU[ i ], cornerV[ i ], &x, &z );
		const Vector3 corner( x, 0.f, z );
		if ( i == 0 ) {
			*boundsMin = corner;
			*boundsMax = corner;
		} else {
			boundsMin->min( corner );
			boundsMax->max( corner );
		}
	}
}

TownVisibility::Footprint TownVisibility::footprintOf( const Building& building ) {
	const Vector3& position = building.position();
	const StaticMeshInstance upright = building.upright(); // その箱が、建物の向きで見た長方形になる
	const WallMesh walls = MeshWallBuilder::make( upright ); // 移動の当たり判定と同じ作り方の、体の高さの壁
	Footprint footprint;
	footprint.centerX = position.x;
	footprint.centerZ = position.z;
	footprint.cosYaw = GameLib::Math::cos( building.angleY() ); // 見た目（Matrix34::rotateY）と同じ、度で受ける cos・sin
	footprint.sinYaw = GameLib::Math::sin( building.angleY() );
	footprint.minU = upright.boundsMin().x;
	footprint.minV = upright.boundsMin().z;
	footprint.maxU = upright.boundsMax().x;
	footprint.maxV = upright.boundsMax().z;
	footprint.blockMinU = walls.boundsMin().x + kOccluderInset;
	footprint.blockMinV = walls.boundsMin().z + kOccluderInset;
	footprint.blockMaxU = walls.boundsMax().x - kOccluderInset;
	footprint.blockMaxV = walls.boundsMax().z - kOccluderInset;
	footprint.height = upright.boundsMax().y;
	return footprint;
}

void TownVisibility::buildFootprints( const std::vector< Building >& buildings, Vector3* townMin, Vector3* townMax ) {
	// ── 建物の足元と、遮る長方形のます ──
	*townMin = buildings.front().mesh().boundsMin(); // 町を囲む範囲は、置いた見た目を囲む箱で広げる（回った建物でも、その足元の長方形を囲む）
	*townMax = buildings.front().mesh().boundsMax();
	mMaxEyeHeight = buildings.front().mesh().boundsMax().y;
	for ( const Building& building : buildings ) {
		const Vector3& boundsMin = building.mesh().boundsMin();
		const Vector3& boundsMax = building.mesh().boundsMax();
		mFootprints.push_back( footprintOf( building ) );
		townMin->x = std::min( townMin->x, boundsMin.x );
		townMin->z = std::min( townMin->z, boundsMin.z );
		townMax->x = std::max( townMax->x, boundsMax.x );
		townMax->z = std::max( townMax->z, boundsMax.z );
		mMaxEyeHeight = std::min( mMaxEyeHeight, boundsMax.y );
	}
	mMaxEyeHeight -= kEyeHeightMargin;
	mOccluders.reset( *townMin, *townMax, kCellSize );
	for ( int i = 0; i < static_cast< int >( mFootprints.size() ); ++i ) {
		Vector3 blockMin;
		Vector3 blockMax;
		mFootprints[ i ].blockBounds( &blockMin, &blockMax ); // 回った長方形を囲む箱で入れる。斜めの建物は隣のますの候補にも入るが、線分は長方形そのもので調べる
		mOccluders.add( i, blockMin, blockMax );
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
				float u;
				float v;
				f.toLocal( centerX, centerZ, &u, &v );
				if ( rectContains( u, v, f.blockMinU, f.blockMinV, f.blockMaxU, f.blockMaxV ) ) {
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
		float au;
		float av;
		float bu;
		float bv;
		f.toLocal( ax, az, &au, &av ); // 線分の両端を建物の向きへ回して、回す前の長方形と比べる（回して動かしても、線分は同じ長さの線分のまま）
		f.toLocal( bx, bz, &bu, &bv );
		float minU = f.blockMinU;
		float minV = f.blockMinV;
		float maxU = f.blockMaxU;
		float maxV = f.blockMaxV;
		// 線の端が長方形の中にある（ますの隅が建物の足元に入った）なら、その建物では遮らない（見える物を消さない側）
		if ( rectContains( au, av, minU, minV, maxU, maxV ) || rectContains( bu, bv, minU, minV, maxU, maxV ) ) {
			continue;
		}
		if ( segmentTouchesRect( au, av, bu, bv, minU, minV, maxU, maxV ) ) {
			return true;
		}
	}
	return false;
}

bool TownVisibility::seesBuilding( float x, float z, int building ) const {
	const Footprint& f = mFootprints[ building ];
	float u;
	float v;
	f.toLocal( x, z, &u, &v ); // 見る点を建物の向きへ回し、回す前の長方形の辺で考える。辺の上の点は world へ戻してから線を引く
	if ( rectContains( u, v, f.minU, f.minV, f.maxU, f.maxV ) ) {
		return true; // 見る点が建物の足元の中（庇の下など）
	}
	// 見る点を向いた辺（最大 2 本）の上に、間隔ごとの点を置いて線を引く
	struct Edge{
		float au, av, bu, bv;
	};
	Edge edges[ 2 ];
	int edgeNumber = 0;
	if ( u < f.minU ) {
		edges[ edgeNumber++ ] = { f.minU - kTargetOffset, f.minV, f.minU - kTargetOffset, f.maxV };
	} else if ( u > f.maxU ) {
		edges[ edgeNumber++ ] = { f.maxU + kTargetOffset, f.minV, f.maxU + kTargetOffset, f.maxV };
	}
	if ( v < f.minV ) {
		edges[ edgeNumber++ ] = { f.minU, f.minV - kTargetOffset, f.maxU, f.minV - kTargetOffset };
	} else if ( v > f.maxV ) {
		edges[ edgeNumber++ ] = { f.minU, f.maxV + kTargetOffset, f.maxU, f.maxV + kTargetOffset };
	}
	for ( int e = 0; e < edgeNumber; ++e ) {
		const Edge& edge = edges[ e ];
		float length = std::sqrt( ( edge.bu - edge.au ) * ( edge.bu - edge.au ) + ( edge.bv - edge.av ) * ( edge.bv - edge.av ) );
		int steps = std::max( 1, static_cast< int >( std::ceil( length / kTargetSpacing ) ) );
		for ( int s = 0; s <= steps; ++s ) {
			float t = static_cast< float >( s ) / steps;
			float tx;
			float tz;
			f.toWorld( ScalarMath::lerp( edge.au, edge.bu, t ), ScalarMath::lerp( edge.av, edge.bv, t ), &tx, &tz );
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
