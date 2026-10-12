#include "Zombie.h"
#include "ZombieModel.h"
#include "CollisionWorld.h"
#include "HitWorld.h"

using namespace GameLib;
using namespace GameLib::Math;

Zombie::Zombie( const ZombieModel* model, const Vector3& position, float angleY,
const char* animation, float startTime ) :
	mModel( model ),
	mMesh( model->data() ),
	mHitboxes( &model->hitboxLayout() ),
	mPosition( position ),
	mAngleY( angleY ) {
	mMesh.setPosition( mPosition );
	mMesh.setAngleY( mAngleY );
	if ( animation ) {
		mMesh.setAnimation( animation, startTime );
	} else {
		mMesh.setAnimation( Scene::Animation(), startTime ); // 空のアニメ＝レスト姿勢
	}
}

void Zombie::update() {
	mMesh.update();
	mHitboxes.update( mMesh );
}

void Zombie::draw() const {
	mMesh.draw();
}

BoxCollider Zombie::collider() const {
	return mModel->footprint().place( mPosition, mAngleY );
}

const HitboxSet& Zombie::hitboxes() const {
	return mHitboxes;
}

void Zombie::addTo( CollisionWorld* collisionWorld, HitWorld* hitWorld ) {
	collisionWorld->addBody( this );
	hitWorld->addTarget( this );
}

void Zombie::removeFrom( CollisionWorld* collisionWorld, HitWorld* hitWorld ) {
	collisionWorld->removeBody( this );
	hitWorld->removeTarget( this );
}
