#include "GameSrc/Audio/GroundMaterial.h"
#include "GameSrc/Audio/IGroundSurface.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

GroundMaterialMap::GroundMaterialMap( const GroundZone* zones, int zoneCount, const IGroundSurface* surface, GroundMaterial fallback ) :
	mZones( zones, zones + zoneCount ),
	mSurface( surface ),
	mFallback( fallback ) {
	ASSERT( mSurface && "GroundMaterialMap : surface is null." );
}

GroundMaterial GroundMaterialMap::materialAt( const Vector3& position ) const {
	for ( const GroundZone& zone : mZones ) {
		if ( position.x >= zone.minX && position.x <= zone.maxX && position.z >= zone.minZ && position.z <= zone.maxZ ) {
			return zone.material;
		}
	}
	GroundMaterial material;
	if ( mSurface->materialBelow( position, &material ) ) {
		return material;
	}
	return mFallback;
}
