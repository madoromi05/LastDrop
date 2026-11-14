#include "GameSrc/Zombie/Component/SeveredLimbComponent.h"
#include "GameSrc/Map/TownGround.h"
#include "GameSrc/World/GameObjectWorld.h"

using namespace GameLib::Math;

SeveredLimbComponent::SeveredLimbComponent( const SkinnedMeshData* data, const ZombieSeverLayout::Cut& cut, const std::vector< Matrix34 >& worldPose,
const Vector3& direction, std::mt19937* random ) :
	mLimb( data, cut, worldPose, direction, random ),
	mGround( nullptr ) {
}

void SeveredLimbComponent::draw( const DrawCulling& culling ) const {
	mLimb.draw( culling );
}

UpdatePhase SeveredLimbComponent::updatePhase() const {
	return UpdatePhase::Late; // 出たティック（弾の当たりを解決したティック）のうちに 1 回進む
}

void SeveredLimbComponent::start() {
	mGround = shared< TownGround >();
}

void SeveredLimbComponent::update() {
	mLimb.update( mGround );
	// 沈み終えたら、自分の物を消す印を付ける。消えるのは全員の更新が済んだ後の flush（並びをたどっている最中には消えない）
	if ( mLimb.isGone() ) {
		gameObject()->world()->destroyLater( gameObject() );
	}
}
