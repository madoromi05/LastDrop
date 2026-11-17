#include "GameSrc/Scene/Game/PlayerFeedback.h"
#include "GameSrc/Scene/Game/World/GameWorld.h"
#include "GameSrc/Scene/Game/World/TownEnvironment.h"
#include "GameSrc/Scene/Game/Hud/HudController.h"
#include "GameSrc/Player/Camera/FirstPersonControl.h"
#include "GameSrc/Player/PlayerAimSwaySource.h"
#include "GameSrc/Player/PlayerHealthComponent.h"
#include "GameSrc/Player/Body/FirstPersonBodyComponent.h"
#include "GameSrc/Player/Body/HoldComponent.h"
#include "GameSrc/Weapon/GunComponent.h"
#include "GameSrc/World/GameObject.h"

#include "GameLib/GameLib.h"
#include "GameLib/Profiling/Profiler.h"

namespace{
	const float kNoShotRecoil = 0.f;    // 撃っていないティックの反動の大きさ（FirstPersonControl::updateViewEffects）
	const float kFullHealthRatio = 1.f; // 自キャラがいないときは赤くしない
}

PlayerFeedback::PlayerFeedback( AudioEngine* audio, const TownEnvironment* town ) :
	mBodySounds( audio, &town->ground(), town->map().groundZones() ),
	mCombatSounds( audio ) {
}

void PlayerFeedback::reactToCombat( const GameWorld& world, FirstPersonControl* control ) {
	ASSERT( control && "PlayerFeedback::reactToCombat : control is null." );
	{
		PROFILE_SCOPE( "Game.hitFeedback" );
		mCombatSounds.playHits( world.combat() ); // ゾンビ・物に当たった音（このティックに当たった分だけ。次の update で入れ替わるので、ここで読む）
		shakeOnHeavyHits( world, control ); // 揺れも同じ出来事を読む（次の updateViewEffects から効く）
	}
	{
		PROFILE_SCOPE( "Game.damage" );
		reactToAttacks( world, control );
	}
}

void PlayerFeedback::shakeOnHeavyHits( const GameWorld& world, FirstPersonControl* control ) const {
	using Weight = CombatResolver::ZombieHit::Weight;
	for ( const CombatResolver::ZombieHit& hit : world.combat().zombieHitsThisTick() ) {
		if ( hit.weight == Weight::Severed || hit.weight == Weight::HeadBurst ) {
			control->shakeOnHeavyHit( hit.weight == Weight::HeadBurst );
		}
	}
}

void PlayerFeedback::reactToAttacks( const GameWorld& world, FirstPersonControl* control ) {
	bool hit = false;
	for ( const CombatResolver::ZombieAttackEvent& event : world.combat().zombieAttacksThisTick() ) {
		switch ( event.kind ) {
			case CombatResolver::ZombieAttackEvent::Kind::Grab:
				control->lookToward( event.point ); // つかんだゾンビの顔を見る（その後はマウスで自由に回せる）
				control->shakeOnGrab();
				break;
			case CombatResolver::ZombieAttackEvent::Kind::Hit:
				control->flinch(); // 狙いが跳ね、絵が大きく揺れる
				hit = true;
				break;
		}
	}
	mCombatSounds.playAttacks( world.combat() );
	control->updateLookPull(); // 揺れ・反動をカメラへ渡す updateViewEffects より前に
	const GameObject* player = world.player();
	const PlayerHealthComponent* healthComponent = player ? player->getComponent< PlayerHealthComponent >() : nullptr; // 外せる部品
	const PlayerHealth* health = healthComponent ? &healthComponent->health() : nullptr;
	float healthRatio = health ? health->current() / health->maxHealth() : kFullHealthRatio;
	mDamageView.update( hit, healthRatio, world.isPlayerDead() );
}

void PlayerFeedback::syncView( const GameWorld& world, FirstPersonControl* control, HudController* hud ) {
	ASSERT( control && hud && "PlayerFeedback::syncView : an argument is null." );
	const GameObject* player = world.player();
	const GunComponent* gun = HoldComponent::heldGun( player ); // 素手なら nullptr
	// 撃ったティックの弾は跳ねる前の向きへ飛び、跳ねた視点は次のティックから効く（1発目は狙った所へ飛ぶ）
	// 攻撃された怯み・揺れ（reactToAttacks で受けた分）もここでカメラへ渡す
	// 照準のぶれも同じく次のティックから効く（自キャラがいなければ既定の状態＝ぶれない）
	// 反動の大きさは銃ごと（拳銃はライフルの半分。WeaponCatalog）
	float shotRecoil = ( gun && gun->firedThisTick() ) ? gun->spec().recoilScale : kNoShotRecoil;
	// 右下の弾数は、銃を覗いている間とリロード中だけ出す（持っているだけでは出さない）
	const HoldComponent* hold = player ? player->getComponent< HoldComponent >() : nullptr;
	hud->updateAmmoVisibility( hold && gun && ( hold->isAiming() || gun->isReloading() ) );
	control->updateViewEffects( shotRecoil, player ? PlayerAimSwaySource::read( *player ) : AimSwayBody() );
	if ( player ) {
		// 銃の見た目へ、弾の向き（weaponDirection）と同じ角度のぶれを渡す
		player->getComponent< FirstPersonBodyComponent >()->setWeaponSway( control->camera().weaponSwayYaw(), control->camera().weaponSwayPitch() );
		mBodySounds.update( *player );
	}
}

const PlayerDamageView& PlayerFeedback::damageView() const {
	return mDamageView;
}
