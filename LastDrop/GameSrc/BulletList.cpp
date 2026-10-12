#include "BulletList.h"
#include "StaticMeshData.h"

#include "GameLib/GameLib.h"

#include <algorithm>

using namespace GameLib::Math;

BulletList::BulletList( const StaticMeshData* data ) :
	mData( data ) {
	ASSERT( mData && "BulletList : data is null." );
}

void BulletList::fire( const Vector3& origin, const Vector3& direction ) {
	mBullets.emplace_back( origin, direction );
}

void BulletList::update( const HitWorld& world, const IHittable* shooter ) {
	for ( Bullet& bullet : mBullets ) {
		bullet.update( world, shooter );
	}
	mBullets.erase( std::remove_if( mBullets.begin(), mBullets.end(),
		[]( const Bullet& bullet ) { return bullet.isFinished(); } ), mBullets.end() );
}

void BulletList::draw() const {
	if ( !mData->isReady() ) {
		return;
	}
	for ( const Bullet& bullet : mBullets ) {
		bullet.draw( *mData );
	}
}
