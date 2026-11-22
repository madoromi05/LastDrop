#include "GameSrc/Audio/GroundMaterial.h"
#include "GameSrc/Audio/IGroundSurface.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

GroundMaterialMap::GroundMaterialMap( const IGroundSurface* surface, GroundMaterial fallback ) :
	mSurface( surface ),
	mFallback( fallback ) {
	ASSERT( mSurface && "GroundMaterialMap : surface is null." );
}

GroundMaterial GroundMaterialMap::materialAt( const Vector3& position ) const {
	GroundMaterial material;
	if ( mSurface->materialBelow( position, &material ) ) {
		return material;
	}
	return mFallback;
}
