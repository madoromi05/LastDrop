#include "GameSrc/Player/HoldComponent.h"
#include "GameSrc/Player/HeldItem.h"
#include "GameSrc/Player/HeldMagazine.h"
#include "GameSrc/Player/FirstPersonBodyComponent.h"
#include "GameSrc/Player/PlayerLocomotionComponent.h"
#include "GameSrc/Component/SkinnedModelComponent.h"
#include "GameSrc/Component/LightComponent.h"
#include "GameSrc/Weapon/GunComponent.h"
#include "GameSrc/Weapon/WeaponModelComponent.h"
#include "GameSrc/World/GameObject.h"
#include "GameSrc/Core/GameTime.h"
#include "GameSrc/Interaction/Inventory.h"
#include "GameSrc/Render/SkinnedMesh/SkinnedMeshData.h"

#include "GameLib/GameLib.h"
#include "GameLib/Base/DebugStream.h"
#include "GameLib/Math/Functions.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// 銃を持たせる手（右手の骨）。持ち方（手から見た銃の置き方）は銃の側（WeaponModelComponent::gripInHand）。
	const char* kWeaponBone = "mixamorig:RightHand";
	// 銃を支える手。どのアニメでも、この手の骨が銃の上の決まった点に来るよう右手首を曲げる（FirstPersonBody）。
	const char* kSupportHandBone = "mixamorig:LeftHand";
	// 持ち方を合わせたアニメと時刻。このポーズで左手が銃のどこにあるかを「左手の来るべき点」にする。
	// 数値で持たずにアニメから求めるのは、持ち方を合わせ直したときに一緒に合わせ直す値を増やさないため。
	const char* kGripCalibrationAnimation = "rifle_idle"; // rifle aiming idle
	const float kGripCalibrationTime = 0.f;               // 1フレーム目
	// リロード中に腕だけ差し替えるアニメ（Basic Shooter Pack の reloading。tools/configs/player.json の extra_animations）。
	const char* kReloadAnimation = "rifle_reload";

	const float kNoWeight   = 0.f; // 補正の効き具合
	const float kFullWeight = 1.f;
	// 補正を効かせる／外す長さ（フレーム）。アニメを切り替えるクロスフェードと同じ 0.2 秒
	// （素手⇔銃・歩き⇔走りのポーズの切り替えに、補正が急に入ると腕や手首が跳ねて見えるため）。
	const float kWeightFadeFrames = 0.2f * GameTime::kTicksPerSecond;

	// 補正の効き具合を target へ1フレームぶん近づける。
	float approachWeight( float weight, float target ) {
		float step = kFullWeight / kWeightFadeFrames;
		if ( weight < target ) {
			return Math::min( target, weight + step );
		}
		return Math::max( target, weight - step );
	}

	// 持ち方を合わせたアニメで、支える手の骨が銃のどこにあるか（銃のモデル座標）を out へ書く。
	// そのアニメが無ければ false（両手持ちの補正をしない）。起動時に1回だけ呼ぶ（骨格を1つ作って捨てる）。
	bool findSupportHandOnWeapon( const SkinnedMeshData* data, int hand, int supportHand, const Matrix34& grip, Vector3* out ) {
		Scene::Animation calibration = data->animation( kGripCalibrationAnimation );
		if ( !calibration ) {
			GameLib::cout << "HoldComponent: animation not found: " << kGripCalibrationAnimation << GameLib::endl;
			return false;
		}
		Scene::Tree skeleton = data->createSkeleton();
		skeleton.setAnimation( calibration );
		skeleton.updateAnimation( kGripCalibrationTime );
		std::vector< Matrix34 > pose( data->boneNumber() );
		data->computeBonePose( skeleton, &pose );

		Matrix34 toWeapon; // 骨格の根の空間 → 銃のモデル座標
		toWeapon.setMul( pose[ hand ], grip );
		toWeapon.invert();
		const Matrix34& support = pose[ supportHand ];
		toWeapon.mul( out, Vector3( support.m03, support.m13, support.m23 ) );
		return true;
	}
}

HoldComponent::HoldComponent( const SkinnedMeshData* holderData, const GameObject* held ) :
	mHolderData( holderData ),
	mHeld( held ),
	mGun( nullptr ),
	mLight( nullptr ),
	mWeaponModel( nullptr ),
	mModel( nullptr ),
	mBody( nullptr ),
	mReloadArms( holderData, kReloadAnimation ),
	mHoldWeight( 0.f ),
	mCarryWeight( 0.f ) {
	ASSERT( holderData && "HoldComponent : holderData is null." );
	mHoldWeight = mStance.isDrawn() ? kFullWeight : kNoWeight;
}

// HeldItem・HeldMagazine の定義が見える所で破棄する（ヘッダでは前方宣言だけ）。
HoldComponent::~HoldComponent() {
}

void HoldComponent::start() {
	mModel = requireComponent< SkinnedModelComponent >();
	mBody = requireComponent< FirstPersonBodyComponent >();
	GameObject* held = mHeld.get();
	if ( !held ) {
		return; // 素手
	}
	const WeaponModelComponent* model = held->getComponent< WeaponModelComponent >();
	int hand = mHolderData->findBone( kWeaponBone );
	if ( !model || hand < 0 ) {
		// 骨名違いでも落ちずに素手になるだけなので、ここで知らせる。
		GameLib::cout << "HoldComponent: weapon bone not found: " << kWeaponBone << GameLib::endl;
		return;
	}
	mWeaponModel = model;
	mGun = held->getComponent< GunComponent >();
	mLight = held->getComponent< LightComponent >();
	Matrix34 grip = model->gripInHand();
	mHeldItem = std::make_unique< HeldItem >( model->weapon(), hand, grip );
	// 支える手が見つからなければ、右手だけで持つ（左手が銃から離れることがあるだけで落ちない）。
	int supportHand = mHolderData->findBone( kSupportHandBone );
	Vector3 supportOnWeapon( 0.f, 0.f, 0.f );
	if ( supportHand < 0 ) {
		GameLib::cout << "HoldComponent: support hand bone not found: " << kSupportHandBone << GameLib::endl;
	} else if ( !findSupportHandOnWeapon( mHolderData, hand, supportHand, grip, &supportOnWeapon ) ) {
		supportHand = -1;
	}
	Vector3 weaponMin;
	Vector3 weaponMax;
	model->bounds( &weaponMin, &weaponMax );
	mBody->body().setWeapon( hand, grip, model->sightEye(), supportHand, supportOnWeapon, weaponMin, weaponMax );
	// 弾倉は支える手（左手）で抜いて挿す。支える手が無ければ、弾倉は描かない。
	if ( model->magazine() && supportHand >= 0 ) {
		mMagazine = std::make_unique< HeldMagazine >( model->magazine(), supportHand );
	}
}

GunComponent* HoldComponent::gun() const {
	return mGun;
}

LightComponent* HoldComponent::light() const {
	return mGun ? mLight : nullptr;
}

GameObject* HoldComponent::heldObject() const {
	return mHeld.get();
}

void HoldComponent::updateStance( bool holsterPressed, bool aimHeld, bool runHeld ) {
	mStance.update( holsterPressed, aimHeld, runHeld );
}

bool HoldComponent::isDrawn() const {
	return mStance.isDrawn();
}

bool HoldComponent::isAiming() const {
	return mStance.isAiming();
}

float HoldComponent::aimWeight() const {
	return mStance.aimWeight();
}

bool HoldComponent::isWeaponInHand() const {
	return mGun && mStance.isDrawn();
}

void HoldComponent::updateReload( bool reloadPressed, Inventory* inventory ) {
	if ( !mGun ) {
		return;
	}
	if ( !mStance.isDrawn() ) {
		// しまったらやめる（替えている途中の銃は手から消えるので、弾は込めない）。
		mGun->cancelReload();
	} else if ( reloadPressed && mGun->canStartReload( *inventory ) ) {
		mGun->startReload();
		mStance.stopAiming(); // 覗いていたら銃を下ろしてから替える
	}
	mGun->updateReload( inventory );
}

void HoldComponent::updateArms( Animator::State next ) {
	FirstPersonBody& body = mBody->body();
	// リロード中は腕をアニメへ差し替える。アニメでは左手が銃を離れるので、その間は両手持ちの補正（右手首を左手へ向ける）を弱める。
	float armWeight = 0.f;
	if ( mGun && mGun->isReloading() && mReloadArms.sample( mGun->reload().animationFrame(), &mReloadPose ) ) {
		armWeight = mGun->reload().armWeight();
	}
	body.setArmOverride( armWeight > 0.f ? &mReloadPose : nullptr, armWeight );
	mHoldWeight = approachWeight( mHoldWeight, mStance.isDrawn() ? kFullWeight : kNoWeight );
	mCarryWeight = approachWeight( mCarryWeight, PlayerLocomotionComponent::isCarryState( next ) ? kFullWeight : kNoWeight );
	body.setHoldWeight( mHoldWeight * ( kFullWeight - armWeight ) );
	body.setCarryWeight( mCarryWeight );
	body.setAimWeight( mStance.aimWeight() );
}

void HoldComponent::takeOutMagazineIfCued() {
	if ( !mGun || mGun->reloadCueThisTick() != WeaponReload::CUE_MAG_OUT ) {
		return;
	}
	// 弾倉を抜いた瞬間の左手から見た場所を覚える（それからは左手に付いて動く）。
	Matrix34 gun;
	if ( mMagazine && weaponPlacement( &gun ) ) {
		mMagazine->takeOut( mModel->mesh(), gun );
	}
}

float HoldComponent::carryWeight() const {
	return mCarryWeight;
}

bool HoldComponent::muzzle( Vector3* position, Vector3* direction ) const {
	if ( !isWeaponInHand() ) {
		return false;
	}
	Matrix34 gun;
	if ( !weaponPlacement( &gun ) ) {
		return false; // まだポーズが無い
	}
	mWeaponModel->muzzle( gun, position, direction );
	return true;
}

void HoldComponent::draw() const {
	if ( !mHeldItem || !mStance.isDrawn() ) {
		return;
	}
	mHeldItem->draw( mModel->mesh() );
	Matrix34 gun;
	if ( mMagazine && mGun && weaponPlacement( &gun ) ) {
		mMagazine->draw( mModel->mesh(), gun, mGun->reload().isMagazineOut(), mGun->reload().magazineSeatWeight() );
	}
}

bool HoldComponent::weaponPlacement( Matrix34* out ) const {
	return mHeldItem && mHeldItem->placement( mModel->mesh(), out );
}
