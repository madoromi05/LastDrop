#include "GameSrc/Map/TownEdits.h"
#include "GameSrc/Raid/DayCondition.h"
#include "GameSrc/Raid/RaidParams.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace{
	const float kKeyTolerance = 0.05f;	// 名札と元の座標を同じ物とみなす距離（m）
	const int kKindNumber = static_cast< int >( TownEditKind::Count );
	const int kMessageSize = 160;

	// 使わなかった行の理由。F6 の欄とログに出すので英字で書く（F6 の字形には日本語が無い）
	const char* const kCannotDelete = "cannot delete this kind";
	const char* const kCannotAdd = "cannot add this kind";
	const char* const kNoMatch = "no object matches the key";
	const char* const kManyMatches = "matches 2 or more objects";
	const char* const kAlreadyDeleted = "the same object is already deleted";
	const char* const kBelowMinimum = "deleting it would go below the minimum count";
	const char* const kMoveDeleted = "cannot move a deleted object";
	const char* const kAlreadyMoved = "the same object is already moved";
	const char* const kNoList = "no list to add to";

	const char* const kKindNames[ kKindNumber ] = {
		"Building", "PerimeterBuilding", "Dumpster", "Crate", "FireHydrant", "StreetLight", "ZombieSpawn", "EscapeCar", "PlayerStart",
	};

	int indexOf( TownEditKind kind ){
		return static_cast< int >( kind );
	}

	bool isNear( const TownEditKey& key, float x, float z ){
		const float dx = x - key.fromX;
		const float dz = z - key.fromZ;
		return dx * dx + dz * dz <= kKeyTolerance * kKeyTolerance;
	}

	bool isSameKey( const TownEditKey& a, const TownEditKey& b ){
		return a.kind == b.kind && isNear( a, b.fromX, b.fromZ );
	}

	std::vector< TownPlacement >* placementList( TownMapLists* lists, TownEditKind kind ){
		switch ( kind ) {
			case TownEditKind::Building:			return lists->buildings;
			case TownEditKind::PerimeterBuilding:	return lists->perimeterBuildings;
			case TownEditKind::Dumpster:			return lists->dumpsters;
			case TownEditKind::Crate:				return lists->crates;
			case TownEditKind::FireHydrant:			return lists->fireHydrants;
			case TownEditKind::StreetLight:			return lists->streetLights;
			case TownEditKind::ZombieSpawn:			return lists->zombieSpawnPoints;
			default:								return nullptr;
		}
	}

	int sizeOf( TownMapLists* lists, TownEditKind kind ){
		if ( kind == TownEditKind::EscapeCar || kind == TownEditKind::PlayerStart ) {
			return lists->escapeCarSpots ? static_cast< int >( lists->escapeCarSpots->size() ) : 0;
		}
		std::vector< TownPlacement >* list = placementList( lists, kind );
		return list ? static_cast< int >( list->size() ) : 0;
	}

	TownPlacement* placementAt( TownMapLists* lists, TownEditKind kind, int index ){
		if ( kind == TownEditKind::EscapeCar ) {
			return &( *lists->escapeCarSpots )[ index ].placement;
		}
		if ( kind == TownEditKind::PlayerStart ) {
			return &( *lists->escapeCarSpots )[ index ].playerStart;
		}
		return &( *placementList( lists, kind ) )[ index ];
	}

	std::vector< int > findMatches( TownMapLists* lists, const TownEditKey& key ){
		std::vector< int > matches;
		const int size = sizeOf( lists, key.kind );
		for ( int i = 0; i < size; ++i ) {
			const TownPlacement* placement = placementAt( lists, key.kind, i );
			if ( isNear( key, placement->x, placement->z ) ) {
				matches.push_back( i );
			}
		}
		return matches;
	}

	std::string message( const char* verb, const TownEditKey& key, const char* reason ){
		char buffer[ kMessageSize ];
		std::snprintf( buffer, sizeof( buffer ), "%s %s (%.2f, %.2f): %s", verb, kKindNames[ indexOf( key.kind ) ], key.fromX, key.fromZ, reason );
		return buffer;
	}

	bool canRemoveFrom( const TownEditCounts& counts, TownEditKind kind, const TownEditLimits& limits ) {	// counts から kind を 1 つ消しても下限を割らないか。counts は種類ごとの今の数で、消した分は引いてある
		switch ( kind ) {
			case TownEditKind::Dumpster:	return counts.lootSpots - 1 >= limits.minLootSpots;
			case TownEditKind::Crate:		return counts.lootSpots - 1 >= limits.minLootSpots && counts.crates - 1 >= limits.minCrates;
			case TownEditKind::ZombieSpawn:	return counts.zombieSpawns - 1 >= limits.minZombieSpawns;
			default:						return true;
		}
	}

	void subtractOne( TownEditCounts* counts, TownEditKind kind ){	// kind を 1 つ消した分を counts から引く。下限の無い種類は数えない
		switch ( kind ) {
			case TownEditKind::Dumpster:	--counts->lootSpots; break;
			case TownEditKind::Crate:		--counts->lootSpots; --counts->crates; break;
			case TownEditKind::ZombieSpawn:	--counts->zombieSpawns; break;
			default: break;
		}
	}
}

const char* townEditKindName( TownEditKind kind ){
	return kKindNames[ indexOf( kind ) ];
}

bool townEditKindFromName( const char* name, TownEditKind* out ){
	for ( int i = 0; i < kKindNumber; ++i ) {
		if ( std::strcmp( name, kKindNames[ i ] ) == 0 ) {
			*out = static_cast< TownEditKind >( i );
			return true;
		}
	}
	return false;
}

bool townEditKindCanAddOrDelete( TownEditKind kind ){
	return kind != TownEditKind::EscapeCar && kind != TownEditKind::PlayerStart;
}

void TownEdits::move( const TownEditKey& key, const TownPlacement& to ){
	mMoves.push_back( { key, to } );
	++mRevision;
}

void TownEdits::remove( const TownEditKey& key ){
	mRemovals.push_back( key );
	++mRevision;
}

int TownEdits::add( TownEditKind kind, const TownPlacement& at ){
	mAdditions.push_back( { kind, at, mNextAdditionId++ } );
	++mRevision;
	return static_cast< int >( mAdditions.size() ) - 1;
}

void TownEdits::removeAdded( int index ){
	mAdditions.erase( mAdditions.begin() + index );
	++mRevision;
}

void TownEdits::moveAdded( int index, const TownPlacement& to ){
	mAdditions[ index ].at = to;
	++mRevision;
}

void TownEdits::revert( const TownEditKey& key ){
	for ( size_t i = mMoves.size(); i > 0; --i ) {
		if ( isSameKey( mMoves[ i - 1 ].key, key ) ) {
			mMoves.erase( mMoves.begin() + ( i - 1 ) );
		}
	}
	for ( size_t i = mRemovals.size(); i > 0; --i ) {
		if ( isSameKey( mRemovals[ i - 1 ], key ) ) {
			mRemovals.erase( mRemovals.begin() + ( i - 1 ) );
		}
	}
	++mRevision;
}

void TownEdits::clear(){
	mMoves.clear();
	mRemovals.clear();
	mAdditions.clear();
	++mRevision;
}

bool TownEdits::isEmpty() const {
	return mMoves.empty() && mRemovals.empty() && mAdditions.empty();
}

unsigned TownEdits::revision() const {
	return mRevision;
}

const TownPlacement* TownEdits::movedTo( const TownEditKey& key ) const {
	for ( const Move& m : mMoves ) {
		if ( isSameKey( m.key, key ) ) {
			return &m.to;
		}
	}
	return nullptr;
}

bool TownEdits::isRemoved( const TownEditKey& key ) const {
	for ( const TownEditKey& removal : mRemovals ) {
		if ( isSameKey( removal, key ) ) {
			return true;
		}
	}
	return false;
}

const std::vector< TownEdits::Move >& TownEdits::moves() const {
	return mMoves;
}

const std::vector< TownEditKey >& TownEdits::removals() const {
	return mRemovals;
}

const std::vector< TownEdits::Addition >& TownEdits::additions() const {
	return mAdditions;
}

bool TownEdits::canRemoveOneMore( TownEditKind kind, const TownEditCounts& original, const TownEditLimits& limits ) const {
	TownEditCounts counts = original;
	for ( const TownEditKey& removal : mRemovals ) {
		subtractOne( &counts, removal.kind );
	}
	return canRemoveFrom( counts, kind, limits );
}

std::vector< std::string > TownEdits::apply( TownMapLists* lists, const TownEditLimits& limits ) const {
	std::vector< std::string > warnings;

	// 当てる前の並びで、どの物へ当たるかを全部決める（動かした後の座標を名札で探さないため）
	std::vector< std::vector< bool > > removed( kKindNumber );
	std::vector< std::vector< bool > > moved( kKindNumber );
	for ( int k = 0; k < kKindNumber; ++k ) {
		removed[ k ].assign( sizeOf( lists, static_cast< TownEditKind >( k ) ), false );
		moved[ k ].assign( removed[ k ].size(), false );
	}
	TownEditCounts counter = {
		sizeOf( lists, TownEditKind::Dumpster ) + sizeOf( lists, TownEditKind::Crate ),
		sizeOf( lists, TownEditKind::Crate ),
		sizeOf( lists, TownEditKind::ZombieSpawn ),
	};

	for ( const TownEditKey& key : mRemovals ) {
		if ( !townEditKindCanAddOrDelete( key.kind ) ) {
			warnings.push_back( message( "Delete", key, kCannotDelete ) );
			continue;
		}
		const std::vector< int > matches = findMatches( lists, key );
		if ( matches.empty() ) {
			warnings.push_back( message( "Delete", key, kNoMatch ) );
			continue;
		}
		if ( matches.size() > 1 ) {
			warnings.push_back( message( "Delete", key, kManyMatches ) );
			continue;
		}
		const int index = matches[ 0 ];
		if ( removed[ indexOf( key.kind ) ][ index ] ) {
			warnings.push_back( message( "Delete", key, kAlreadyDeleted ) );
			continue;
		}
		if ( !canRemoveFrom( counter, key.kind, limits ) ) {
			warnings.push_back( message( "Delete", key, kBelowMinimum ) );
			continue;
		}
		subtractOne( &counter, key.kind );
		removed[ indexOf( key.kind ) ][ index ] = true;
	}

	struct Resolved{
		TownEditKind kind;
		int index;
		TownPlacement to;
	};
	std::vector< Resolved > resolved;
	for ( const Move& m : mMoves ) {
		const std::vector< int > matches = findMatches( lists, m.key );
		if ( matches.empty() ) {
			warnings.push_back( message( "Move", m.key, kNoMatch ) );
			continue;
		}
		if ( matches.size() > 1 ) {
			warnings.push_back( message( "Move", m.key, kManyMatches ) );
			continue;
		}
		const int index = matches[ 0 ];
		if ( removed[ indexOf( m.key.kind ) ][ index ] ) {
			warnings.push_back( message( "Move", m.key, kMoveDeleted ) );
			continue;
		}
		if ( moved[ indexOf( m.key.kind ) ][ index ] ) {
			warnings.push_back( message( "Move", m.key, kAlreadyMoved ) );
			continue;
		}
		moved[ indexOf( m.key.kind ) ][ index ] = true;
		resolved.push_back( { m.key.kind, index, m.to } );
	}

	for ( const Resolved& r : resolved ) {
		*placementAt( lists, r.kind, r.index ) = r.to;
	}
	for ( int k = 0; k < kKindNumber; ++k ) {
		std::vector< TownPlacement >* list = placementList( lists, static_cast< TownEditKind >( k ) );
		if ( !list ) {
			continue;
		}
		for ( size_t i = removed[ k ].size(); i > 0; --i ) {
			if ( removed[ k ][ i - 1 ] ) {
				list->erase( list->begin() + ( i - 1 ) );
			}
		}
	}

	for ( const Addition& a : mAdditions ) {
		const TownEditKey key = { a.kind, a.at.x, a.at.z };
		std::vector< TownPlacement >* list = placementList( lists, a.kind );
		if ( !townEditKindCanAddOrDelete( a.kind ) ) {
			warnings.push_back( message( "Add", key, kCannotAdd ) );
		} else if ( !list ) {
			warnings.push_back( message( "Add", key, kNoList ) );
		} else {
			list->push_back( a.at );
		}
	}
	return warnings;
}

TownEditLimits townEditLimits(){
	int mostZombies = 0;
	for ( int c = 0; c < static_cast< int >( DayCondition::Count ); ++c ) {
		const int count = dayConditionEffect( static_cast< DayCondition >( c ) ).zombieCount;
		if ( count > mostZombies ) {
			mostZombies = count;
		}
	}
	const RaidParams& raid = raidParams();
	return { raid.fuelCanCount, raid.rifleCount, mostZombies };	// ガソリンはゴミ箱か木箱のどれか・ライフルは木箱に入れるので、漁れる場所と木箱の下限になる
}
