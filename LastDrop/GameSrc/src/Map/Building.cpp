#include "GameSrc/Map/Building.h"
#include "GameSrc/Collision/MeshWallBuilder.h"
#include "GameSrc/Collision/CollisionWorld.h"
#include "GameSrc/Render/Culling/DrawCulling.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

Building::Building( const StaticMeshLods& shapes, const Vector3& position, float angleY, GroundMaterial material ) :
	mMesh( shapes.full(), position, angleY ),
	mShapes( shapes ),
	mMaterial( material ),
	mHidden( false ) {
}

void Building::addCollidersTo( CollisionWorld* world ) const {
	ASSERT( world && "Building : world is null." );
	world->addWallMesh( MeshWallBuilder::make( mMesh ) );
}

void Building::moveTo( const Vector3& position, float angleY ) {
	mMesh.setPosition( position );
	mMesh.setAngleY( angleY );
}

void Building::setHidden( bool hidden ) {
	mHidden = hidden;
}

void Building::draw( const DrawCulling& culling ) const {
	if ( shows( culling ) ) {
		mMesh.draw( shapeFor( culling ) );
	}
}

bool Building::shows( const DrawCulling& culling ) const {
	return !mHidden && culling.shows( mMesh.boundsMin(), mMesh.boundsMax(), DrawCulling::Target::Large );
}

const StaticMeshInstance& Building::mesh() const {
	return mMesh;
}

const StaticMeshData* Building::shapeFor( const DrawCulling& culling ) const {
	return culling.chooseLod( mShapes, &mLod, mMesh.boundsMin(), mMesh.boundsMax() );
}

const Vector3& Building::position() const {
	return mMesh.position();
}

float Building::angleY() const {
	return mMesh.angleY(); // 見た目が持つ向きをそのまま返す（二重に持つと、見た目と足元の向きがずれうるため）
}

StaticMeshInstance Building::upright() const {
	StaticMeshInstance shape( mMesh.data() );						// 原点・向き 0 に置いた同じ形
	shape.setPosition( Vector3( 0.f, mMesh.position().y, 0.f ) );	// 高さだけ建物に合わせる。体の高さの壁（MeshWallBuilder）は world の高さで切るため
	return shape;
}

bool Building::raycast( const Vector3& origin, const Vector3& delta, float* time, Vector3* normal ) const {
	return !mHidden && mMesh.raycast( origin, delta, time, normal );
}

GroundMaterial Building::surfaceMaterial( const Vector3& ) const {
	return mMaterial;
}
