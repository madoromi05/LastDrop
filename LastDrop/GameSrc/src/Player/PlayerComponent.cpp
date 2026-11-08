#include "GameSrc/Player/PlayerComponent.h"
#include "GameSrc/Player/Body/FirstPersonBodyComponent.h"
#include "GameSrc/Player/PlayerLocomotionComponent.h"
#include "GameSrc/Player/Body/HoldComponent.h"
#include "GameSrc/Player/Body/ArmPoseComponent.h"
#include "GameSrc/Player/PlayerHealthComponent.h"
#include "GameSrc/Player/PlayerGrabComponent.h"
#include "GameSrc/Component/TransformComponent.h"
#include "GameSrc/Component/SkinnedModelComponent.h"
#include "GameSrc/Component/HitboxComponent.h"
#include "GameSrc/Component/CharacterColliderComponent.h"
#include "GameSrc/Component/LightComponent.h"
#include "GameSrc/Weapon/GunComponent.h"
#include "GameSrc/World/GameObject.h"
#include "GameSrc/Math/AngleMath.h"
#include "GameSrc/Core/GameTime.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Functions.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// 骨の名前の頭の共通部分（移動の当たり判定で骨を引くのに使う）。
	const char* kBoneNamePrefix = "mixamorig:";
	const float kFootprintPadding = 0.f; // 移動の当たり判定は体ぴったり（狭い所を通れるように）
	// 目から、移動の当たり判定の前の辺までの余白（m）。これより近くに壁が来ないので、視点が壁を越えて建物の中が見えることが無い。
	// カメラは目の前 5cm（FirstPersonCamera の kNearClip）より近くを描かず、その面の隅は目から約 7.7cm 離れている
	// （縦の視野角 60 度・16:9）。斜めに壁を見ても隅が壁に入らないよう、それより少し大きくする。
	const float kEyeToWallMargin = 0.1f;
	// 手に持った銃を囲む箱から、移動の当たり判定の辺までの余白（m）。銃口を壁に入れないため（銃口の光やフラッシュが壁の向こうへ出ないように）。
	// 銃口の少し前に置くもの（マズルフラッシュの光）まで壁の手前に収まるよう、目の余白と同じだけ取る。
	const float kWeaponToWallMargin = 0.1f;
	const float kNoRecoil = 0.f; // 銃を持っていないときの反動
	const float kNoWeight = 0.f; // 持つ部品が無いときの覗き具合・抱えている割合（素手）
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

void PlayerComponent::setInput( const PlayerCommand& command, float cameraYaw, float cameraPitch, Inventory* inventory ) {
	mCommand = command;
	mCameraYaw = cameraYaw;
	mCameraPitch = cameraPitch;
	mInventory = inventory;
}

UpdatePhase PlayerComponent::updatePhase() const {
	return UpdatePhase::Input;
}

void PlayerComponent::awake() {
	mFootprint = std::make_unique< BodyFootprint >( requireComponent< SkinnedModelComponent >()->data(), kBoneNamePrefix, kFootprintPadding );
}

void PlayerComponent::start() {
	mTransform = requireComponent< TransformComponent >();
	mModel = requireComponent< SkinnedModelComponent >();
	mBody = requireComponent< FirstPersonBodyComponent >();
	mLocomotion = requireComponent< PlayerLocomotionComponent >();
	CharacterColliderComponent* collider = requireComponent< CharacterColliderComponent >();
	collider->setShape( this ); // 移動の当たり判定の形はここが答える
	// collider に自分を覚えさせたので、自分にも外せない印を付ける（requireComponent で引いた側にしか印が付かないため）。
	requireComponent< PlayerComponent >();
	mCollider = collider;
}

void PlayerComponent::update() {
	ASSERT( mInventory && "PlayerComponent::update : inventory is null (setInput first)." );
	// 持つ・体力・命中の部品は外せる（外すと素手・死なない・撃たれない）ので、使うたびに引く。
	HoldComponent* hold = getComponent< HoldComponent >();
	const PlayerHealthComponent* health = getComponent< PlayerHealthComponent >();
	PlayerCommand command = allowedCommand( mCommand );
	// 一人称では体の向き＝視線の向き。振り向きを遅らせると、視点と腕・銃の向きがずれて見えるため即座に合わせる。
	float& angleY = mTransform->angleY();
	angleY = AngleMath::wrapSigned( mCameraYaw );
	mBody->body().setPitch( mCameraPitch );

	Vector3 direction;
	bool moving = mLocomotion->readMoveDirection( command, mCameraYaw, &direction );
	bool wantsRun = moving && command.run;
	if ( moving ) {
		mLocomotion->setMoveDirection( direction );
	}
	// 捨てた銃を手から外し、素手ならカバンの銃を持つ（このティックの構え・撃つより前に）。
	if ( hold ) {
		hold->syncWithInventory( *mInventory );
	}
	GunComponent* gun = hold ? hold->gun() : nullptr;
	// リロード中は覗かない（覗く入力を渡さない。押したままなら、込め終えた後に押し直したとみなして覗き始める）。
	bool reloading = gun && gun->isReloading();
	if ( hold ) {
		hold->updateStance( command.holsterPressed, command.aimHeld && !reloading, wantsRun );
		hold->updateReload( command.reloadPressed, mInventory );
		// 懐中電灯は銃に付いているので、銃を持っていれば（しまっていても・リロード中でも）付け外しできる。
		if ( LightComponent* light = hold->light() ) {
			light->update( command.flashlightPressed );
		}
	}
	// 覗いている間は走らない（覗く方を優先して歩きに落とす）。覗くのをやめれば、走る入力が続いている限り走りに戻る。
	// リロード中も走らない（弾倉を替えながら歩くだけ。バイオハザードと同じ）。
	reloading = gun && gun->isReloading();
	bool running = wantsRun && !( hold && hold->isAiming() ) && !reloading;
	Animator::State next = PlayerLocomotionComponent::chooseState( moving, running, hold && hold->isDrawn() );
	// 撃てるのは、銃を手に持っていて、走っていない間だけ（走りでは銃口を下げて胸元に抱えている）。
	// リロード中も撃てない（銃を下げて弾倉を替えている）。
	fire( hold && hold->isWeaponInHand() && !running && !reloading, command.fireHeld );
	mBody->body().setRecoil( gun ? gun->recoilKick() : kNoRecoil );
	// 腕の効き具合（両手持ち・胸元への寄せ・リロードの腕）。外せる部品（無ければ腕は体のアニメのまま）。
	if ( ArmPoseComponent* arms = getComponent< ArmPoseComponent >() ) {
		arms->update( next );
	}
	// 入力を離しても最後に進んでいた向きへ減速しながら進み、止まる。
	float speedScale = health ? health->health().walkSpeedScale() : kFullSpeedScale;
	mLocomotion->approachSpeed( PlayerLocomotionComponent::speedOf( next ) * speedScale );
	Vector3 step = mLocomotion->step();
	mLocomotion->changeState( next );
	// 移動より先に姿勢を進める。移動の当たり判定は今の目の位置まで伸ばす（collider）ので、
	// このティックの姿勢（見下ろして前へ出た目）で判定しないと、1ティックだけ目が壁を越える。
	// 姿勢は骨格の根の空間で作るので、置き場所が決まる前に進めてよい。
	SkinnedMeshInstance& mesh = mModel->mesh();
	mesh.setAngleY( angleY );
	mesh.update();
	// 向きは上で変えてある。変えた向き・伸びた当たり判定で壁にめり込んだ分も、ここで外へ出す。
	Vector3& position = mTransform->position();
	position += mCollider->mover().move( step );
	position.y = mCollider->mover().followGround( position ); // 歩道の段差に乗る
	mesh.setPosition( position );
	if ( HitboxComponent* hitbox = getComponent< HitboxComponent >() ) {
		hitbox->boxes().update( mesh );
	}
	if ( hold ) {
		hold->takeOutMagazineIfCued();
	}
}

void PlayerComponent::fire( bool canFire, bool triggerHeld ) {
	HoldComponent* hold = getComponent< HoldComponent >();
	GunComponent* gun = hold ? hold->gun() : nullptr;
	if ( !gun ) {
		return;
	}
	// 撃てない間も銃へ伝える（次の1発までの間隔を、撃てない間に止めないため）。
	gun->pullTrigger( triggerHeld && canFire, triggerHeld );
	gun->updateRecoil();
}

AimSwayBody PlayerComponent::aimSwayBody() const {
	const HoldComponent* hold = getComponent< HoldComponent >();
	const PlayerHealthComponent* health = getComponent< PlayerHealthComponent >();
	AimSwayBody body;
	body.weaponDrawn = hold && hold->isWeaponInHand();
	body.aimWeight = hold ? hold->aimWeight() : kNoWeight;
	body.walkRate = mLocomotion->walkRate();
	body.gaitPhase = mLocomotion->gaitPhase();
	body.gaitCycling = mLocomotion->gait() != GAIT_STILL;
	const ArmPoseComponent* arms = getComponent< ArmPoseComponent >();
	body.carryWeight = arms ? arms->carryWeight() : kNoWeight;
	body.danger = health && health->health().condition() == PlayerCondition::Danger;
	return body;
}

BoxCollider PlayerComponent::collider() const {
	const Vector3& position = mTransform->position();
	float angleY = mTransform->angleY();
	// 目は体（腰・胴・脚）より前にあり、見下ろすとさらに前へ出る。前の辺を、今の目の先（＋余白）まで伸ばす。
	// 目は骨格の根の空間（前方が +Z）で持っているので、z がそのまま足元から前への長さになる。
	float eyeReach = mBody->body().eyePosition().z + kEyeToWallMargin;
	// 持っている銃は体より前へ突き出ているので、銃を囲む箱（＋余白）も長方形に含める。銃口が壁に入らなくなる。
	// 銃の箱は今のポーズ（覗く・抱える・反動）から求めたものなので、構えを変えると長方形の大きさも変わる。
	const HoldComponent* hold = getComponent< HoldComponent >();
	Vector3 low;
	Vector3 high;
	if ( !hold || !hold->isWeaponInHand() || !mBody->body().weaponBounds( &low, &high ) ) {
		return mFootprint->place( position, angleY, eyeReach );
	}
	low.x -= kWeaponToWallMargin;
	low.z -= kWeaponToWallMargin;
	high.x += kWeaponToWallMargin;
	high.z = Math::max( high.z + kWeaponToWallMargin, eyeReach );
	return mFootprint->place( position, angleY, low, high );
}

PlayerCommand PlayerComponent::allowedCommand( const PlayerCommand& command ) const {
	PlayerCommand allowed = command;
	const PlayerHealthComponent* health = getComponent< PlayerHealthComponent >();
	if ( health && health->isDead() ) {
		// 死んだら、その場で止まって何もしない（押した瞬間の入力も捨てる）。
		allowed.forward = 0.f;
		allowed.strafe = 0.f;
		allowed.run = false;
		allowed.holsterPressed = false;
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
		allowed.run = false;
	}
	return allowed;
}
