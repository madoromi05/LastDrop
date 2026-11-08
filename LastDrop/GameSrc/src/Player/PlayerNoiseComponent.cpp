#include "GameSrc/Player/PlayerNoiseComponent.h"
#include "GameSrc/Player/PlayerLocomotionComponent.h"
#include "GameSrc/Player/Body/HoldComponent.h"
#include "GameSrc/Component/TransformComponent.h"
#include "GameSrc/Weapon/GunComponent.h"
#include "GameSrc/Zombie/Move/ZombieHearing.h"
#include "GameSrc/World/GameObject.h"
#include "GameSrc/World/GameObjectWorld.h"
#include "GameSrc/Core/GameTime.h"

namespace{
	// 足音が届く距離（m。自キャラの足元から水平だけ。壁では切らない。SummaryPlan/個別仕様/ZombieFootstepHearing_Plan.txt）。
	// 走りは横・後ろの目（10m）より遠くまで届き、歩きはそれより短いので、そっと歩けば後ろを抜けられる。
	const float kWalkFootstepHearingRange = 5.f;
	const float kRunFootstepHearingRange = 18.f;
	// 足音を知らせる間隔（秒。ゾンビが目の線を引く間隔と同じ。毎ティック全員を調べないため）。
	const float kFootstepNoiseIntervalSeconds = 0.3f;
	const int kFootstepNoiseIntervalTicks = GameTime::secondsToTicks( kFootstepNoiseIntervalSeconds );

	// 歩き方 gait の足音が届く距離（m。止まっていれば 0）。
	float footstepHearingRange( Gait gait ) {
		switch ( gait ) {
			case GAIT_WALK: return kWalkFootstepHearingRange;
			case GAIT_RUN:  return kRunFootstepHearingRange;
			default:        return 0.f;
		}
	}
}

PlayerNoiseComponent::PlayerNoiseComponent() :
	mTransform( nullptr ),
	mLocomotion( nullptr ),
	mHearing( nullptr ),
	mFootstepTicks( 0 ) {
}

UpdatePhase PlayerNoiseComponent::updatePhase() const {
	return UpdatePhase::Input;
}

void PlayerNoiseComponent::start() {
	mTransform = requireComponent< TransformComponent >();
	mLocomotion = requireComponent< PlayerLocomotionComponent >();
	mHearing = shared< ZombieHearing >();
}

void PlayerNoiseComponent::update() {
	emitGunshot();
	emitFootsteps();
}

void PlayerNoiseComponent::emitGunshot() {
	const GunComponent* gun = HoldComponent::heldGun( gameObject() ); // 素手・持つ部品を外していれば nullptr
	if ( gun && gun->firedThisTick() ) {
		mHearing->hearGunshot( mTransform->position() );
	}
}

void PlayerNoiseComponent::emitFootsteps() {
	if ( --mFootstepTicks > 0 ) {
		return;
	}
	mFootstepTicks = kFootstepNoiseIntervalTicks;
	mHearing->hearFootsteps( mTransform->position(), footstepHearingRange( mLocomotion->gait() ) );
}
