#include "Zombie.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// 骨の名前の頭の共通部分（Resource/character.txt の骨は "Zombie_Head" のような名前）。
	const char* kBoneNamePrefix = "Zombie_";
}

Zombie::Zombie( const SkinnedMeshData* data, const Vector3& position, float angleY,
const char* animation, float startTime ) :
	mMesh( data ),
	mFootprint( data, kBoneNamePrefix ),
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
