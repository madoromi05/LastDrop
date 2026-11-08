#include "GameSrc/Player/PlayerBodySounds.h"
#include "GameSrc/Player/PlayerLocomotionComponent.h"
#include "GameSrc/Player/Body/HoldComponent.h"
#include "GameSrc/Component/TransformComponent.h"
#include "GameSrc/Component/LightComponent.h"
#include "GameSrc/Weapon/GunComponent.h"
#include "GameSrc/World/GameObject.h"

using namespace GameLib::Math;

namespace{
	// 道路と歩道の材質は TownGround が、それと違う材質にする場所（水たまりなど。今は無い）は TownMap が決める。
	// これは、地面が答えられない場所（地面を並べる前・地面の外）の材質。
	const GroundMaterial kDefaultGround = GROUND_CONCRETE;
	// 素手のときの銃声（撃たないので鳴らない。FoleyState を埋めるためだけの値）。
	const SoundId kNoShotSound = SOUND_GUNSHOT;
	const float kNoShotVolume = 0.f;
	// 装備の音を鳴らす高さ（足元から。胸のあたり）。足音は足元、装備は体から鳴らして聞こえる高さを分ける。
	const float kBodySoundHeight = 1.3f;
}

PlayerBodySounds::PlayerBodySounds( AudioEngine* audio, const IGroundSurface* ground, const std::vector< GroundZone >& zones ) :
	mGround( zones.data(), static_cast< int >( zones.size() ), ground, kDefaultGround ),
	mFootsteps( audio ),
	mFoley( audio ) {
}

void PlayerBodySounds::update( const GameObject& player ) {
	const PlayerLocomotionComponent* locomotion = player.getComponent< PlayerLocomotionComponent >();
	const HoldComponent* hold = player.getComponent< HoldComponent >(); // 外せる部品（無ければ素手）
	const GunComponent* gun = HoldComponent::heldGun( &player );  // 素手なら nullptr
	const LightComponent* light = hold ? hold->light() : nullptr; // 同上
	const Vector3& feet = player.getComponent< TransformComponent >()->position();
	bool stepped = mFootsteps.update( locomotion->gait(), locomotion->gaitPhase(), feet, mGround );
	PlayerFoley::FoleyState foley;
	foley.bodyPosition.set( feet.x, feet.y + kBodySoundHeight, feet.z );
	foley.gait = locomotion->gait();
	foley.weaponDrawn = hold && hold->isDrawn();
	foley.aiming = hold && hold->isAiming();
	foley.fired = gun && gun->firedThisTick();
	foley.shotSound = gun ? gun->spec().shotSound : kNoShotSound;
	foley.shotVolume = gun ? gun->spec().shotVolume : kNoShotVolume;
	foley.dryFired = gun && gun->dryFiredThisTick();
	WeaponReload::Cue cue = gun ? gun->reloadCueThisTick() : WeaponReload::CUE_NONE;
	foley.magazineOut = ( cue == WeaponReload::CUE_MAG_OUT );
	foley.magazineIn = ( cue == WeaponReload::CUE_MAG_IN );
	foley.boltPulled = ( cue == WeaponReload::CUE_BOLT );
	foley.flashlightToggled = light && light->toggledThisTick();
	mFoley.update( foley, stepped );
}
