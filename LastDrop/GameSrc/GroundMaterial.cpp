#include "GroundMaterial.h"

using namespace GameLib::Math;

namespace{
	// GroundMaterial の順に並べる。
	const char* kMaterialNames[] = {
		"CONCRETE", "METAL", "WOOD", "GRASS", "GRAVEL", "WATER", "GLASS",
	};
	static_assert( sizeof( kMaterialNames ) / sizeof( kMaterialNames[ 0 ] ) == GROUND_NUM, "kMaterialNames must list every GroundMaterial" );
}

const char* groundMaterialName( GroundMaterial material ) {
	return kMaterialNames[ material ];
}

GroundMaterialMap::GroundMaterialMap( const GroundZone* zones, int zoneCount, GroundMaterial fallback ) :
	mZones( zones, zones + zoneCount ),
	mFallback( fallback ) {
}

GroundMaterial GroundMaterialMap::materialAt( const Vector3& position ) const {
	for ( const GroundZone& zone : mZones ) {
		if ( position.x >= zone.minX && position.x <= zone.maxX && position.z >= zone.minZ && position.z <= zone.maxZ ) {
			return zone.material;
		}
	}
	return mFallback;
}
