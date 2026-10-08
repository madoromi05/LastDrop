#include "Zombie.h"
#include "CollisionWorld.h"
#include "HitWorld.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// 骨の名前の頭の共通部分（Resource/character.txt の骨は "Zombie_Head" のような名前）。
	const char* kBoneNamePrefix = "Zombie_";
	// 移動の当たり判定を体より一回り大きくする幅（m。四辺それぞれ）。体の見た目どうしがめり込む前に止めるため。
	// 部位の当たり判定（HitboxSet）は変えない（弾は体ぴったりで当てる）。
	const float kFootprintPadding = 0.15f;
}

Zombie::Zombie( const SkinnedMeshData* data, const Vector3& position, float angleY,
const char* animation, float startTime ) :
	mMesh( data ),
	mFootprint( data, kBoneNamePrefix, kFootprintPadding ),
	mHitboxes( data, kBoneNamePrefix ),
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
	return mFootprint.place( mPosition, mAngleY );
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
