#include "GameSrc/Scene/Game/GameEditorBridge.h"
#include "GameSrc/Scene/Game/World/GameWorld.h"
#include "GameSrc/Scene/Game/Screen/ModalController.h"
#include "GameSrc/Player/Camera/FirstPersonControl.h"
#include "GameSrc/Player/Camera/FirstPersonCamera.h"
#include "GameSrc/Player/PlayerHealthComponent.h"
#include "GameSrc/Player/Body/HoldComponent.h"
#include "GameSrc/Component/TransformComponent.h"
#include "GameSrc/Weapon/GunComponent.h"
#include "GameSrc/World/GameObject.h"
#include "GameSrc/DebugMode/EditorWindow.h"
#include "GameSrc/Core/Pad.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Functions.h"
#include "GameLib/WindowCreator/WindowCreator.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// ── F9 でゾンビを出す（確認用。 ユーザー指定：8m 前に 1 体）──
	const float kDebugSpawnDistance = 8.f; // 自キャラの足元から、向いている方へ（m）
	const int kDebugSpawnNumber = 1;
	// 横並びの間（m）。体の当たり判定の幅（約 0.8m）より広くして、出た瞬間に重ならないようにする。
	const float kDebugSpawnSpacing = 1.2f;
	const float kFaceBackDegree = 180.f; // 自キャラの方を向かせる
	const float kHalf = 0.5f;
}

GameEditorBridge::GameEditorBridge() {
}

GameEditorBridge::~GameEditorBridge() {
	EditorWindow::setPlayerInvincible( false ); // 自キャラがいなくなるので、ON のまま残さない
	EditorWindow::setObjects( nullptr ); // 世界が消えるので、消えた物を指し続けないように
	EditorWindow::setInventory( nullptr ); // カバンも同じ
	EditorWindow::setRaid( nullptr );
	EditorWindow::setRun( nullptr );
	EditorWindow::setInteraction( nullptr );
}

void GameEditorBridge::markReady() {
	mFireProbe.markReady();
}

void GameEditorBridge::publish( GameWorld* world, Inventory* inventory, RaidSession* session, RunState* run ) {
	// F5 で自キャラの無敵を切り替える（押した瞬間。Debug と Profile ビルドだけで効く）。
	GameObject* player = world->player();
	if ( InputDebugInvincible() && player ) {
		if ( PlayerHealthComponent* health = player->getComponent< PlayerHealthComponent >() ) { // 外せる部品（無ければ元から死なない）
			health->toggleInvincible();
		}
	}
	// F6 のエディタの一番上に出す。
	const PlayerHealthComponent* playerHealth = player ? player->getComponent< PlayerHealthComponent >() : nullptr;
	EditorWindow::setPlayerInvincible( playerHealth && playerHealth->isInvincible() );
	EditorWindow::setObjects( &world->objects() ); // F6 の Objects の欄（世界に置いた物の一覧）
	EditorWindow::setInventory( inventory );       // F6 の Inventory の欄（物を足す・行を増やす）
	EditorWindow::setRaid( session );              // F6 の Raid の欄（残り時間・すぐ脱出）
	EditorWindow::setRun( run );                   // 同じ欄の、日・保存を消す
	EditorWindow::setInteraction( &world->interaction() ); // F6 の Loot の欄（中身を振り直す・全部開けたことにする）
}

void GameEditorBridge::spawnZombiesOnKey( GameWorld* world, const FirstPersonCamera& camera ) {
	if ( !InputDebugSpawnZombie() ) {
		return;
	}
	const GameObject* player = world->player();
	if ( !player ) {
		return;
	}
	// 視点の水平の向きの前と右（PlayerLocomotionComponent::readMoveDirection の cameraForward / cameraRight と同じ式）。間に建物があっても調べない（中に出ることがある）。
	float yaw = camera.yaw();
	Vector3 forward( Math::sin( yaw ), 0.f, Math::cos( yaw ) );
	Vector3 right( -Math::cos( yaw ), 0.f, Math::sin( yaw ) );
	Vector3 center;
	center.setMadd( player->getComponent< TransformComponent >()->position(), forward, kDebugSpawnDistance );
	// 真ん中を中心に、横へ等間隔に並べる（3 体なら -1.2m・0・+1.2m）。
	float firstOffset = -kDebugSpawnSpacing * static_cast< float >( kDebugSpawnNumber - 1 ) * kHalf;
	for ( int i = 0; i < kDebugSpawnNumber; ++i ) {
		Vector3 position;
		position.setMadd( center, right, firstOffset + kDebugSpawnSpacing * static_cast< float >( i ) );
		if ( !world->spawnZombie( position, yaw + kFaceBackDegree ) ) {
			break; // 上限に達した
		}
	}
}

void GameEditorBridge::probeFrame( const GameWorld& world, const FirstPersonControl& control, const ModalController& modals, bool freeCamera ) {
	FireProbe::FrameState frame;
	frame.rawFireHeld = InputFireHeld();
	frame.commandFire = control.command().fireHeld;
	frame.suppressed = control.isFireSuppressed();
	frame.cursorLocked = control.isCursorLocked();
	frame.windowActive = GameLib::WindowCreator().isActive();
	frame.paused = modals.isPause();
	frame.bagOpen = modals.isBag();
	frame.lootOpen = modals.isLoot();
	frame.freeCamera = freeCamera;
	frame.dead = world.isPlayerDead();
	FireProbe::GunState gunState = {};
	const GameObject* player = world.player();
	const HoldComponent* hold = player ? player->getComponent< HoldComponent >() : nullptr;
	const GunComponent* gun = HoldComponent::heldGun( player );
	if ( hold && gun ) {
		gunState.hasGun = true;
		gunState.inHand = hold->isWeaponInHand();
		gunState.aiming = hold->isAiming();
		gunState.reloading = gun->isReloading();
		gunState.running = control.command().run;
		gunState.rounds = gun->magazineRounds();
	}
	mFireProbe.onFrame( frame, gunState );
}

void GameEditorBridge::probeTick( const GameWorld& world ) {
	const GunComponent* gun = HoldComponent::heldGun( world.player() ); // 素手なら nullptr
	mFireProbe.onTick( gun && gun->firedThisTick(), gun && gun->dryFiredThisTick() );
}
