#include "GameSrc/DebugMode/TownEditOriginals.h"
#include "GameSrc/Map/TownMap.h"

#include <vector>

namespace{
	const TownMap& originalTown() {
		static const TownMap town( nullptr, TownEditLimits{} );	// 手直しが無いので下限は使わない
		return town;
	}

	const std::vector< TownPlacement >* originalList( TownEditKind kind ) {	// 種類ごとの元の並び。脱出の車と自キャラの始めの位置は escapeCarSpots の組なので nullptr
		const TownMap& town = originalTown();
		switch ( kind ) {
			case TownEditKind::Building:			return &town.buildings();
			case TownEditKind::PerimeterBuilding:	return &town.perimeterBuildings();
			case TownEditKind::Dumpster:			return &town.dumpsters();
			case TownEditKind::Crate:				return &town.crates();
			case TownEditKind::FireHydrant:			return &town.fireHydrants();
			case TownEditKind::StreetLight:			return &town.streetLights();
			case TownEditKind::ZombieSpawn:			return &town.zombieSpawnPoints();
			default:								return nullptr;
		}
	}
}

int TownEditOriginals::count( TownEditKind kind ) {
	const std::vector< TownPlacement >* list = originalList( kind );
	return static_cast< int >( list ? list->size() : originalTown().escapeCarSpots().size() );
}

const TownPlacement& TownEditOriginals::at( TownEditKind kind, int index ) {
	if ( const std::vector< TownPlacement >* list = originalList( kind ) ) {
		return ( *list )[ index ];
	}
	const TownEscapeCarSpot& spot = originalTown().escapeCarSpots()[ index ];
	return ( kind == TownEditKind::EscapeCar ) ? spot.placement : spot.playerStart;
}

TownEditKey TownEditOriginals::keyOf( TownEditKind kind, int index ) {
	const TownPlacement& original = at( kind, index );
	return { kind, original.x, original.z };
}

TownEditCounts TownEditOriginals::counts() {
	const TownMap& town = originalTown();
	const int crates = static_cast< int >( town.crates().size() );
	return { static_cast< int >( town.dumpsters().size() ) + crates, crates, static_cast< int >( town.zombieSpawnPoints().size() ) };
}

const TownPlacement* TownEditOriginals::placedAt( const TownEdits& edits, TownEditKind kind, int index ) {
	const TownEditKey key = keyOf( kind, index );
	if ( edits.isRemoved( key ) ) {
		return nullptr;
	}
	const TownPlacement* moved = edits.movedTo( key );
	return moved ? moved : &at( kind, index );
}
