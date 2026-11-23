#include "GameSrc/DebugMode/TownBakeStatus.h"

#ifdef ZF_EDITOR

#include "GameSrc/Map/TownMap.h"
#include "GameSrc/Map/TownFingerprint.h"
#include "GameSrc/Map/TownBakeFile.h"

#include "GameLib/FileIO/Manager.h"

#include <cstdint>
#include <string>

namespace{
	TownBakeStatus::State stateOf( const char* path, std::uint64_t fingerprint ) {	// path のファイルが、指紋 fingerprint の町のファイルとして読めるか
		const bool exists = GameLib::FileIO::Manager().exists( path );
		std::vector< unsigned char > payload;
		return TownBakeStatus::classify( exists, exists && TownBakeFile::read( path, fingerprint, &payload ) );
	}
}

TownBakeStatus::State TownBakeStatus::classify( bool exists, bool matches ) {
	if ( !exists ) {
		return State::Missing;
	}
	return matches ? State::File : State::Stale;
}

const char* TownBakeStatus::stateName( State state ) {
	switch ( state ) {
		case State::Missing:	return "missing";
		case State::Stale:		return "stale";
		case State::File:		return "file";
	}
	return "?";
}

bool TownBakeStatus::bakesSame( const TownMap& loaded, const TownMap& edited, int carSpot, float agentRadius ) {
	return TownFingerprint::navMesh( loaded, carSpot, agentRadius ) == TownFingerprint::navMesh( edited, carSpot, agentRadius ) &&
		TownFingerprint::visibility( loaded ) == TownFingerprint::visibility( edited );
}

TownBakeStatus::TownBakeStatus() :
	mVisibility( State::Missing ) {
}

void TownBakeStatus::update( const TownMap& map, float agentRadius ) {
	const int spotCount = static_cast< int >( map.escapeCarSpots().size() );
	mNavMeshes.clear();
	for ( int spot = 0; spot < spotCount; ++spot ) {
		mNavMeshes.push_back( stateOf( TownBakeFile::navMeshPath( spot ).c_str(), TownFingerprint::navMesh( map, spot, agentRadius ) ) );
	}
	mVisibility = stateOf( TownBakeFile::kVisibilityPath, TownFingerprint::visibility( map ) );
}

const std::vector< TownBakeStatus::State >& TownBakeStatus::navMeshes() const {
	return mNavMeshes;
}

TownBakeStatus::State TownBakeStatus::visibility() const {
	return mVisibility;
}

bool TownBakeStatus::isUpToDate() const {
	if ( mVisibility != State::File ) {	// update の前もここで外れる（表は Missing から始まる）
		return false;
	}
	for ( State state : mNavMeshes ) {
		if ( state != State::File ) {
			return false;
		}
	}
	return true;
}

std::vector< int > TownBakeStatus::spotsToBake( int currentSpot ) const {
	std::vector< int > spots;
	for ( int spot = 0; spot < static_cast< int >( mNavMeshes.size() ); ++spot ) {
		if ( mNavMeshes[ spot ] != State::File ) {
			spots.push_back( spot );
		}
	}
	if ( spots.empty() && mVisibility != State::File ) {
		spots.push_back( currentSpot );	// 表は Bake のたびに書くので、今の出撃のまま 1 回書けば足りる
	}
	return spots;
}

#endif // ZF_EDITOR
