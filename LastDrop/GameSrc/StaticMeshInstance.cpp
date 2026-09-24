#include "StaticMeshInstance.h"
#include "StaticMeshData.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Matrix34.h"

using namespace GameLib::Math;

StaticMeshInstance::StaticMeshInstance( const StaticMeshData* data ) :
	mData( data ),
	mPosition( 0.f, 0.f, 0.f ),
	mAngleY( 0.f ),
	mScale( 1.f ) {
	ASSERT( mData && "StaticMeshInstance : data is null." );
}

void StaticMeshInstance::setPosition( const Vector3& position ) {
	mPosition = position;
}

void StaticMeshInstance::setAngleY( float degree ) {
	mAngleY = degree;
}

void StaticMeshInstance::setScale( float scale ) {
	mScale = scale;
}

void StaticMeshInstance::draw() const {
	// world = 移動 * 回転 * 拡大（拡大→回転→移動の順に効く）。
	Matrix34 world;
	world.setTranslation( mPosition );
	world.rotateY( mAngleY );
	world.scale( mScale );
	mData->draw( world );
}
