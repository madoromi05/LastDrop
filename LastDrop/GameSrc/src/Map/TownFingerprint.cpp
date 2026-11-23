#include "GameSrc/Map/TownFingerprint.h"
#include "GameSrc/Map/TownMap.h"

#include "GameLib/GameLib.h"

#include <cmath>
#include <vector>

namespace{
	const std::uint64_t kFnvOffsetBasis = 14695981039346656037ull;	// FNV-1a 64 ビットの始めの値
	const std::uint64_t kFnvPrime = 1099511628211ull;				// FNV-1a 64 ビットで 1 バイトごとに掛ける数
	const int kBitsPerByte = 8;
	const int kBytesPerValue = 8;									// 1 つの値を、この数のバイトに分けて混ぜる（std::int64_t）
	const std::uint64_t kByteMask = 0xffu;
	const std::int64_t kFormatVersion = 1;							// 混ぜ方か、NavMeshBuilder・TownVisibility の床と表を作る定数を変えたら必ず上げる（古いファイルを読まないように）
	const double kUnitsPerMeter = 100.0;							// 長さは cm に丸める（1cm 未満のずれは同じ町とみなす）
	const double kUnitsPerDegree = 100.0;							// 向きは 0.01 度に丸める

	// 混ぜる並びの印。並びの境目を混ぜて、物が隣の並びへ移っただけで同じ値にならないようにする
	enum class Part : std::int64_t{
		Buildings,
		PerimeterBuildings,
		Dumpsters,
		Crates,
		FireHydrants,
		StreetLights,
		EscapeCar,
		AgentRadius,
	};

	// FNV-1a（64 ビット）。値は下のバイトから順に混ぜるので、同じ値の並びなら、どのビルドでも同じ結果になる
	class Fnv1a{
	public:
		Fnv1a() :
			mHash( kFnvOffsetBasis ) {
		}
		void mix( std::int64_t value ) {
			const std::uint64_t bits = static_cast< std::uint64_t >( value );
			for ( int i = 0; i < kBytesPerValue; ++i ) {
				mHash ^= ( bits >> ( i * kBitsPerByte ) ) & kByteMask;
				mHash *= kFnvPrime;
			}
		}
		void mix( Part part ) {
			mix( static_cast< std::int64_t >( part ) );
		}
		std::uint64_t value() const {
			return mHash;
		}

	private:
		std::uint64_t mHash;
	};

	std::int64_t rounded( float value, double unitsPerOne ) {
		return static_cast< std::int64_t >( std::llround( value * unitsPerOne ) );
	}

	void mixPlacement( const TownPlacement& p, Fnv1a* hash ) {
		hash->mix( rounded( p.x, kUnitsPerMeter ) );
		hash->mix( rounded( p.y, kUnitsPerMeter ) );
		hash->mix( rounded( p.z, kUnitsPerMeter ) );
		hash->mix( rounded( p.yawDegree, kUnitsPerDegree ) );
	}

	void mixList( Part part, const std::vector< TownPlacement >& list, Fnv1a* hash ) {	// 並びの印・数・中身を順に混ぜる（数を混ぜるので、消した・足した物があれば値が変わる）
		hash->mix( part );
		hash->mix( static_cast< std::int64_t >( list.size() ) );
		for ( const TownPlacement& p : list ) {
			mixPlacement( p, hash );
		}
	}

	void mixBuildings( const TownMap& map, Fnv1a* hash ) {	// 建物（ブロックの中・町を囲む）。どちらも WorldBuilder がこの順に並べ、隠れの表の番号もこの順
		mixList( Part::Buildings, map.buildings(), hash );
		mixList( Part::PerimeterBuildings, map.perimeterBuildings(), hash );
	}
}

namespace TownFingerprint{

std::uint64_t navMesh( const TownMap& map, int escapeCarSpot, float agentRadius ) {
	const std::vector< TownEscapeCarSpot >& spots = map.escapeCarSpots();
	ASSERT( escapeCarSpot >= 0 && escapeCarSpot < static_cast< int >( spots.size() ) && "TownFingerprint::navMesh : bad escape car spot." );
	Fnv1a hash;
	hash.mix( kFormatVersion );
	// WorldBuilder が移動の当たり判定へ載せる物。ゾンビの湧く候補・自キャラの始めの位置は載らないので混ぜない
	mixBuildings( map, &hash );
	mixList( Part::Dumpsters, map.dumpsters(), &hash );
	mixList( Part::Crates, map.crates(), &hash );
	mixList( Part::FireHydrants, map.fireHydrants(), &hash );
	mixList( Part::StreetLights, map.streetLights(), &hash );
	hash.mix( Part::EscapeCar );
	mixPlacement( spots[ escapeCarSpot ].placement, &hash );	// 町に置く車はその日の 1 台だけ
	hash.mix( Part::AgentRadius );
	hash.mix( rounded( agentRadius, kUnitsPerMeter ) );
	return hash.value();
}

std::uint64_t visibility( const TownMap& map ) {
	Fnv1a hash;
	hash.mix( kFormatVersion );
	mixBuildings( map, &hash );
	return hash.value();
}

} //namespace TownFingerprint
