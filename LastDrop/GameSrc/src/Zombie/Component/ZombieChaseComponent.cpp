#include "GameSrc/Zombie/Component/ZombieChaseComponent.h"
#include "GameSrc/Zombie/Component/ZombieStateComponent.h"
#include "GameSrc/Zombie/Component/ZombieRootMotionComponent.h"
#include "GameSrc/Zombie/Animation/ZombieModel.h"
#include "GameSrc/Zombie/Component/ZombieModelComponent.h"
#include "GameSrc/Zombie/ZombieUncertaintyParams.h"
#include "GameSrc/Zombie/Move/ChaseTarget.h"
#include "GameSrc/Component/TransformComponent.h"
#include "GameSrc/Component/SkinnedModelComponent.h"
#include "GameSrc/Component/CharacterColliderComponent.h"
#include "GameSrc/World/GameObjectWorld.h"
#include "GameSrc/Map/PathBudget.h"
#include "GameSrc/Math/FlatMath.h"

#include "GameLib/GameLib.h"

ZombieChaseComponent::ZombieChaseComponent( int sightPhase, int wanderSeed, float swayPeriodSeconds, float swayStartPhase, bool runner ) :
	mSightPhase( sightPhase ),
	mWanderSeed( wanderSeed ),
	mSwayPeriodSeconds( swayPeriodSeconds ),
	mSwayStartPhase( swayStartPhase ),
	mRunner( runner ),
	mPosition( nullptr ),
	mAngleY( nullptr ),
	mMesh( nullptr ),
	mState( nullptr ),
	mAnimations( nullptr ) {
}

ZombieChaseComponent::~ZombieChaseComponent() {
}

void ZombieChaseComponent::awake() {
	const ZombieModel* model = requireComponent< ZombieModelComponent >()->model();
	mSway = std::make_unique< ZombieSway >( &model->flinchLayout(), mSwayPeriodSeconds, mSwayStartPhase );
	const CharacterColliderComponent* collider = requireComponent< CharacterColliderComponent >();
	// 仲間の並び（ZombieCrowd）に自分を名乗るのは、物そのもの（ポインタを比べるだけ）。
	mLocomotion = std::make_unique< ZombieLocomotion >( gameObject(), shared< NavMesh >(), shared< PathBudget >(), &collider->mover(),
		mSightPhase, mWanderSeed, mRunner );
}

void ZombieChaseComponent::start() {
	TransformComponent* transform = requireComponent< TransformComponent >();
	mPosition = &transform->position();
	mAngleY = &transform->angleY();
	mMesh = &requireComponent< SkinnedModelComponent >()->mesh();
	mState = requireComponent< ZombieStateComponent >();
	mAnimations = &requireComponent< ZombieRootMotionComponent >()->animations();
}

ZombieLocomotion& ZombieChaseComponent::locomotion() {
	return *mLocomotion;
}

const ZombieLocomotion& ZombieChaseComponent::locomotion() const {
	return *mLocomotion;
}

ZombieSway& ZombieChaseComponent::sway() {
	return *mSway;
}

ZombieLocomotion::Intent ZombieChaseComponent::think( const ChaseTarget* target, const HitWorld& hitWorld, const ZombieCrowd* crowd ) {
	// 考える（見て・行き先を決め・向きを変える）→ 攻撃を始めるか → 動く、の順。
	float weaveDegree = mSway->weaveDegree( zombieUncertaintyParams().weaveDegree ); // 揺れている間は、倒れている側へ少し流れる
	ZombieLocomotion::Intent intent = mLocomotion->thinkStanding( *mPosition, mState->eyePosition(), target, hitWorld, crowd, weaveDegree, mAngleY );
	mMesh->setAngleY( *mAngleY ); // この後、歩かずに攻撃を始めても、向きは見た目に写しておく
	return intent;
}

void ZombieChaseComponent::walk( const ZombieLocomotion::Intent& intent, const ZombieCrowd* crowd ) {
	// 向きを変えている最中も、歩きたいなら歩く（走る）アニメにする（その場で向きを変える動きに見せる）。
	ZombieAnimationSet::Gait gait = ZombieAnimationSet::Gait::Stand;
	if ( intent.walking ) {
		gait = intent.running ? ZombieAnimationSet::Gait::Run : ZombieAnimationSet::Gait::Walk;
	}
	mAnimations->setGait( gait );
	if ( intent.walking && intent.facing ) {
		*mPosition += mLocomotion->walkStep( mAnimations->walkDistanceThisTick(), *mAngleY );
	} else if ( !intent.walking && mLocomotion->mode() == ZombieChase::Mode::Chase ) {
		// 追いついて立ち止まっている。仲間の後ろに重なったまま止まり続けないように
		mLocomotion->standAside( mState->bodyCenter(), crowd, mPosition );
	}
	mMesh->setPosition( *mPosition );
	mMesh->setAngleY( *mAngleY );
}

void ZombieChaseComponent::updateSway( const ChaseTarget* target, bool pursuing ) {
	mSway->update( swayStrength( target, pursuing ), zombieUncertaintyParams().swayMaxDegree );
}

float ZombieChaseComponent::swayStrength( const ChaseTarget* target, bool pursuing ) const {
	// 遠くでは揺らさない（遠くから頭を狙う楽しさは残す）。音で探しに来る・うろつく間も揺らさない。
	if ( !target || !pursuing || mLocomotion->mode() != ZombieChase::Mode::Chase ) {
		return 0.f;
	}
	// 走る体は揺らさない（走りのアニメ自体が大きく動くので、揺れを足すと狙えなくなる。脚が傷んで歩きに落ちたら揺れる）。
	if ( mLocomotion->isRunner() ) {
		return 0.f;
	}
	const ZombieUncertaintyParams& params = zombieUncertaintyParams();
	float distance = FlatMath::distance( mState->bodyCenter(), target->position );
	return ZombieSway::strengthAt( distance, params.swayStartDistance, params.swayFullDistance );
}

ZombieChase::Mode ZombieChaseComponent::mode() const {
	return mLocomotion->mode();
}

const ZombiePath& ZombieChaseComponent::path() const {
	return mLocomotion->path();
}

const GameLib::Math::Vector3& ZombieChaseComponent::separation() const {
	return mLocomotion->separation();
}
