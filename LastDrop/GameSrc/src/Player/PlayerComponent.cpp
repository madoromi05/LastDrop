#include "GameSrc/Player/PlayerComponent.h"
#include "GameSrc/Player/Body/FirstPersonBodyComponent.h"
#include "GameSrc/Player/PlayerLocomotionComponent.h"
#include "GameSrc/Player/Body/HoldComponent.h"
#include "GameSrc/Player/Body/ArmPoseComponent.h"
#include "GameSrc/Player/PlayerHealthComponent.h"
#include "GameSrc/Player/PlayerGrabComponent.h"
#include "GameSrc/Player/PlayerFlightComponent.h"
#include "GameSrc/Interaction/Item/InventoryComponent.h"
#include "GameSrc/Component/TransformComponent.h"
#include "GameSrc/Component/SkinnedModelComponent.h"
#include "GameSrc/Component/HitboxComponent.h"
#include "GameSrc/Component/CharacterColliderComponent.h"
#include "GameSrc/Component/LightComponent.h"
#include "GameSrc/Weapon/GunComponent.h"
#include "GameSrc/World/GameObject.h"
#include "GameSrc/Math/AngleMath.h"

#include "GameLib/GameLib.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const float kNoRecoil = 0.f; // 銃を持っていないときの反動
	const float kFullSpeedScale = 1.f; // 体力の部品が無いときの歩く速さの倍率（傷ついていない）
}

PlayerComponent::PlayerComponent() :
	mCameraYaw( 0.f ),
	mCameraPitch( 0.f ),
	mInventory( nullptr ),
	mTransform( nullptr ),
	mModel( nullptr ),
	mBody( nullptr ),
	mLocomotion( nullptr ),
	mCollider( nullptr ) {
}

void PlayerComponent::setInput( const PlayerCommand& command, float cameraYaw, float cameraPitch ) {
	mCommand = command;
	mCameraYaw = cameraYaw;
	mCameraPitch = cameraPitch;
}

UpdatePhase PlayerComponent::updatePhase() const {
	return UpdatePhase::Input;
}

void PlayerComponent::start() {
	mInventory = requireComponent< InventoryComponent >();
	mTransform = requireComponent< TransformComponent >();
	mModel = requireComponent< SkinnedModelComponent >();
	mBody = requireComponent< FirstPersonBodyComponent >();
	mLocomotion = requireComponent< PlayerLocomotionComponent >();
	mCollider = requireComponent< CharacterColliderComponent >(); // 移動の当たり判定の形を渡すのは PlayerFootprintComponent
}

void PlayerComponent::update() {
	// 持つ・体力・命中の部品は外せる（外すと素手・死なない・撃たれない）ので、使うたびに引く
	HoldComponent* hold = getComponent< HoldComponent >();
	PlayerCommand command = allowedCommand( mCommand );
	faceCamera();
	const PlayerFlightComponent* flight = getComponent< PlayerFlightComponent >(); // 確認用の外せる部品（Release では付けない）
	bool flying = flight && flight->isFlying();

	// 飛んでいる間は歩かない（脚は止まった姿のまま。Shift は飛ぶ速さにだけ使い、銃を胸元へ抱えない）
	Vector3 direction;
	bool moving = !flying && mLocomotion->readMoveDirection( command, mCameraYaw, &direction );
	bool wantsRun = moving && command.run;
	if ( moving ) {
		mLocomotion->setMoveDirection( direction );
	}
	updateHands( hold, command, wantsRun );
	GunComponent* gun = hold ? hold->gun() : nullptr;	// 手に持つ銃が替わるのは updateHands の中（カバンとの同期）だけなので、ここで読めばこのティックの銃
	// 覗いている間は走らない（覗く方を優先して歩きに落とす）。覗くのをやめれば、走る入力が続いている限り走りに戻る
	// リロード中も走らない（弾倉を替えながら歩くだけ。バイオハザードと同じ）
	bool reloading = gun && gun->isReloading();
	bool running = wantsRun && !( hold && hold->isAiming() ) && !reloading;
	Animator::State next = PlayerLocomotionComponent::chooseState( moving, running, hold && hold->isWeaponInHand() );
	// 撃てるのは、銃を手に持っていて、走っていない間だけ（走りでは銃口を下げて胸元に抱えている）
	// リロード中も撃てない（銃を下げて弾倉を替えている）
	fire( gun, hold && hold->isWeaponInHand() && !running && !reloading, command.fireHeld );
	Vector3 flightStep = flying ? flight->step( command, mCameraYaw, mCameraPitch ) : Vector3( 0.f, 0.f, 0.f );
	moveBody( next, getComponent< PlayerHealthComponent >(), flying ? &flightStep : nullptr );
	if ( hold ) {
		hold->takeOutMagazineIfCued();
	}
}

void PlayerComponent::faceCamera() {
	// 一人称では体の向き＝視線の向き。振り向きを遅らせると、視点と腕・銃の向きがずれて見えるため即座に合わせる
	mTransform->angleY() = AngleMath::wrapSigned( mCameraYaw );
	mBody->body().setPitch( mCameraPitch );
}

void PlayerComponent::updateHands( HoldComponent* hold, const PlayerCommand& command, bool wantsRun ) {
	if ( !hold ) {
		return;
	}
	Inventory& inventory = mInventory->inventory();
	// 捨てた銃を手から外し、素手ならカバンの銃を持つ（このティックの構え・撃つより前に）
	hold->syncWithInventory( inventory );
	GunComponent* gun = hold->gun();
	// リロード中は覗かない（覗く入力を渡さない。押したままなら、込め終えた後に押し直したとみなして覗き始める）
	bool reloading = gun && gun->isReloading();
	hold->updateStance( command.aimHeld && !reloading, wantsRun );
	hold->updateReload( command.reloadPressed, &inventory );
	// 懐中電灯は銃に付いているので、銃を持っていれば（リロード中でも）付け外しできる
	if ( LightComponent* light = hold->light() ) {
		light->update( command.flashlightPressed );
	}
}

void PlayerComponent::fire( GunComponent* gun, bool canFire, bool triggerHeld ) {
	if ( gun ) {
		// 撃てない間も銃へ伝える（次の1発までの間隔を、撃てない間に止めないため）
		gun->pullTrigger( triggerHeld && canFire, triggerHeld );
		gun->updateRecoil();
	}
	mBody->body().setRecoil( gun ? gun->recoilKick() : kNoRecoil );
}

void PlayerComponent::moveBody( Animator::State next, const PlayerHealthComponent* health, const Vector3* flightStep ) {
	// 腕の効き具合（両手持ち・胸元への寄せ・リロードの腕）。外せる部品（無ければ腕は体のアニメのまま）
	if ( ArmPoseComponent* arms = getComponent< ArmPoseComponent >() ) {
		arms->update( next );
	}
	// 入力を離しても最後に進んでいた向きへ減速しながら進み、止まる
	float speedScale = health ? health->health().walkSpeedScale() : kFullSpeedScale;
	mLocomotion->approachSpeed( PlayerLocomotionComponent::speedOf( next ) * speedScale );
	Vector3 step = mLocomotion->step();
	mLocomotion->changeState( next );
	// 移動より先に姿勢を進める。移動の当たり判定は今の目の位置まで伸ばす（PlayerFootprintComponent::collider）ので、
	// このティックの姿勢（見下ろして前へ出た目）で判定しないと、1ティックだけ目が壁を越える
	// 姿勢は骨格の根の空間で作るので、置き場所が決まる前に進めてよい
	SkinnedMeshInstance& mesh = mModel->mesh();
	mesh.setAngleY( mTransform->angleY() );
	mesh.update();
	// 向きは faceCamera で変えてある。変えた向き・伸びた当たり判定で壁にめり込んだ分も、ここで外へ出す
	Vector3& position = mTransform->position();
	if ( flightStep ) {
		position += *flightStep; // 飛んでいる間は壁も地面もすり抜ける
	} else {
		position += mCollider->mover().move( step );
		position.y = mCollider->mover().followGround( position ); // 歩道の段差に乗る
	}
	mesh.setPosition( position );
	if ( HitboxComponent* hitbox = getComponent< HitboxComponent >() ) {
		hitbox->boxes().update( mesh );
	}
}

PlayerCommand PlayerComponent::allowedCommand( const PlayerCommand& command ) const {
	PlayerCommand allowed = command;
	const PlayerHealthComponent* health = getComponent< PlayerHealthComponent >();
	if ( health && health->isDead() ) {
		// 死んだら、その場で止まって何もしない（押した瞬間の入力も捨てる）
		allowed.forward = 0.f;
		allowed.strafe = 0.f;
		allowed.rise = 0.f;
		allowed.run = false;
		allowed.aimHeld = false;
		allowed.fireHeld = false;
		allowed.reloadPressed = false;
		allowed.flashlightPressed = false;
		return allowed;
	}
	const PlayerGrabComponent* grab = getComponent< PlayerGrabComponent >(); // 外せる部品（無ければ、つかまれない）
	if ( grab && grab->isHeld() ) {
		allowed.forward = 0.f; // つかまれている間は動けない（撃って振りほどく）
		allowed.strafe = 0.f;
		allowed.rise = 0.f;
		allowed.run = false;
	}
	return allowed;
}
