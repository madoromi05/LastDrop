#include "PlacementMatrix.h"

using namespace GameLib::Math;

Matrix34 PlacementMatrix::make( const Vector3& position, float angleY ) {
	Matrix34 world;
	world.setTranslation( position );
	world.rotateY( angleY );
	return world;
}

Matrix34 PlacementMatrix::make( const Vector3& position, float angleY, float scale ) {
	Matrix34 world = make( position, angleY );
	world.scale( scale );
	return world;
}
