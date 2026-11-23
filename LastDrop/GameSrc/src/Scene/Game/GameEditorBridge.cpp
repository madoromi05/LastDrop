#include "GameSrc/Scene/Game/GameEditorBridge.h"
#include "GameSrc/Scene/Game/World/GameWorld.h"
#include "GameSrc/Player/Camera/FirstPersonCamera.h"
#include "GameSrc/Player/PlayerHealthComponent.h"
#include "GameSrc/Player/PlayerFlightComponent.h"
#include "GameSrc/Interaction/Item/InventoryComponent.h"
#include "GameSrc/Component/TransformComponent.h"
#include "GameSrc/World/GameObject.h"
#include "GameSrc/DebugMode/EditorWindow.h"
#include "GameSrc/DebugMode/DebugSwitches.h"
#include "GameSrc/Math/AngleMath.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Functions.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// ── F6 の Spawn zombie でゾンビを出す（確認用。8m 前に 1 体） ──
	const float kDebugSpawnDistance = 8.f;	// 自キャラの足元から、向いている方へ（m）
	const int kDebugSpawnNumber = 1;
	const float kDebugSpawnSpacing = 1.2f;	// 横並びの間（m）。体の当たり判定の幅（約 0.8m）より広くして、出た瞬間に重ならないようにする
	const float kFaceBackDegree = 180.f;	// 自キャラの方を向かせる
	const float kHalf = 0.5f;
}

GameEditorBridge::GameEditorBridge() {
}

GameEditorBridge::~GameEditorBridge() {
	EditorWindow::setPlayerInvincible( false ); // 自キャラがいなくなるので、ON のまま残さない
	EditorWindow::setFlightCamera( nullptr ); // 自キャラの視点もシーンと一緒に無くなる
	EditorWindow::setInventory( nullptr ); // カバンが消えるので、消えた物を指し続けないように
	EditorWindow::setRaid( nullptr );
	EditorWindow::setInteraction( nullptr );
	EditorWindow::setWorld( nullptr );
}

void GameEditorBridge::publish( GameWorld* world, RaidSession* session, const FirstPersonCamera& view ) {
	// F6 の Debug タブで頼まれていたら、自キャラの無敵・飛ぶを切り替える（Debug と Profile ビルドだけで効く）
	GameObject* player = world->player();
	DebugSwitches& switches = debugSwitches();
	if ( switches.toggleInvincible && player ) {
		switches.toggleInvincible = false; // 読んだので下ろす（自キャラが居ない間＝ロード中は、置かれるまで残す）
		if ( PlayerHealthComponent* health = player->getComponent< PlayerHealthComponent >() ) { // 外せる部品（無ければ元から死なない）
			health->toggleInvincible();
		}
	}
	if ( switches.toggleFlight && player ) {
		switches.toggleFlight = false; // 無敵と同じく、自キャラが置かれるまで残す
		if ( PlayerFlightComponent* flight = player->getComponent< PlayerFlightComponent >() ) {
			flight->toggle();
		}
	}
	// F6 の Debug タブに今の状態を出す
	const PlayerHealthComponent* playerHealth = player ? player->getComponent< PlayerHealthComponent >() : nullptr;
	const PlayerFlightComponent* playerFlight = player ? player->getComponent< PlayerFlightComponent >() : nullptr;
	EditorWindow::setPlayerInvincible( playerHealth && playerHealth->isInvincible() );
	EditorWindow::setFlightCamera( playerFlight && playerFlight->isFlying() ? &view : nullptr );
	EditorWindow::setInventory( InventoryComponent::inventoryOf( player ) ); // F6 の Inventory の欄（物を足す・行を増やす）。自キャラが居なければ出さない
	EditorWindow::setRaid( session );              // F6 の Raid の欄（経った時間・すぐ脱出）
	EditorWindow::setWorld( world );               // 同じ欄の、今の出撃の条件を切り替える
	EditorWindow::setInteraction( &world->interaction() ); // F6 の Loot の欄（中身を振り直す・全部開けたことにする）
}

void GameEditorBridge::followTownEdits( GameWorld* world, const TownEdits& edits ) {
	mTownModels.follow( edits, world );
}

void GameEditorBridge::spawnRequestedZombies( GameWorld* world, const FirstPersonCamera& camera ) {
	bool& requested = debugSwitches().spawnZombies;
	if ( !requested ) {
		return;
	}
	requested = false; // 読んだので下ろす（自キャラが居なくても、押したことは捨てる）
	const GameObject* player = world->player();
	if ( !player ) {
		return;
	}
	// 視点の水平の向きの前と右。間に建物があっても調べない（中に出ることがある）
	float yaw = camera.yaw();
	Vector3 forward = AngleMath::forwardOf( yaw );
	Vector3 right = AngleMath::rightOf( yaw );
	Vector3 center;
	center.setMadd( player->getComponent< TransformComponent >()->position(), forward, kDebugSpawnDistance );
	// 真ん中を中心に、横へ等間隔に並べる（3 体なら -1.2m・0・+1.2m）
	float firstOffset = -kDebugSpawnSpacing * static_cast< float >( kDebugSpawnNumber - 1 ) * kHalf;
	for ( int i = 0; i < kDebugSpawnNumber; ++i ) {
		Vector3 position;
		position.setMadd( center, right, firstOffset + kDebugSpawnSpacing * static_cast< float >( i ) );
		if ( !world->spawnZombie( position, yaw + kFaceBackDegree ) ) {
			break; // 上限に達した
		}
	}
}
