#include "GameSrc/Player/PlayerAimSwaySource.h"
#include "GameSrc/Player/PlayerLocomotionComponent.h"
#include "GameSrc/Player/PlayerHealthComponent.h"
#include "GameSrc/Player/Body/HoldComponent.h"
#include "GameSrc/Player/Body/ArmPoseComponent.h"
#include "GameSrc/World/GameObject.h"

AimSwayBody PlayerAimSwaySource::read( const GameObject& player ) {
	AimSwayBody body; // 部品が無い所は既定の値のまま（素手・止まっている・傷ついていない）
	if ( const HoldComponent* hold = player.getComponent< HoldComponent >() ) {
		body.weaponDrawn = hold->isWeaponInHand();
		body.aimWeight = hold->aimWeight();
	}
	if ( const PlayerLocomotionComponent* locomotion = player.getComponent< PlayerLocomotionComponent >() ) {
		body.walkRate = locomotion->walkRate();
		body.gaitPhase = locomotion->gaitPhase();
		body.gaitCycling = locomotion->gait() != GAIT_STILL;
	}
	if ( const ArmPoseComponent* arms = player.getComponent< ArmPoseComponent >() ) {
		body.carryWeight = arms->carryWeight();
	}
	if ( const PlayerHealthComponent* health = player.getComponent< PlayerHealthComponent >() ) {
		body.danger = health->health().condition() == PlayerCondition::Danger;
	}
	return body;
}
