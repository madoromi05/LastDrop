#include "GameSrc/Zombie/Component/ZombieStateComponent.h"
#include "GameSrc/Zombie/Animation/ZombieModel.h"
#include "GameSrc/Zombie/Component/ZombieModelComponent.h"
#include "GameSrc/Zombie/Component/ZombieDismemberComponent.h"
#include "GameSrc/Zombie/Component/ZombieRootMotionComponent.h"
#include "GameSrc/Component/TransformComponent.h"
#include "GameSrc/Component/HitboxComponent.h"
#include "GameSrc/Component/CharacterColliderComponent.h"
#include "GameSrc/World/GameObject.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

namespace{
	const float kEyeHeight = 1.6f;	// 目の高さ（m。足元から）。ここから自キャラの目が見えているかを調べる。身長 1.82m の目の辺り
}

ZombieStateComponent::ZombieStateComponent() :
	mState( State::Alive ),
	mModel( nullptr ),
	mPosition( nullptr ),
	mAngleY( nullptr ),
	mHitboxes( nullptr ),
	mCrawler( nullptr ),
	mRootSpan( nullptr ),
	mAnimations( nullptr ) {
}

void ZombieStateComponent::start() {
	mModel = requireComponent< ZombieModelComponent >()->model();
	TransformComponent* transform = requireComponent< TransformComponent >();
	mPosition = &transform->position();
	mAngleY = &transform->angleY();
	mHitboxes = &requireComponent< HitboxComponent >()->boxes();
	mCrawler = &requireComponent< ZombieDismemberComponent >()->crawler();
	ZombieRootMotionComponent* rootMotion = requireComponent< ZombieRootMotionComponent >();
	mRootSpan = &rootMotion->span();
	mAnimations = &rootMotion->animations();
	requireComponent< CharacterColliderComponent >()->setShape( this );	// 移動の当たり判定の形はここが答える
	requireComponent< ZombieStateComponent >();							// collider に自分を覚えさせたので、自分にも外せない印を付ける（requireComponent で引いた側にしか印が付かないため）
}

ZombieStateComponent::State ZombieStateComponent::state() const {
	return mState;
}

void ZombieStateComponent::setState( State state ) {
	mState = state;
}

bool ZombieStateComponent::isAlive() const {
	return mState == State::Alive || mState == State::Downed;
}

bool ZombieStateComponent::isCrawling() const {
	return mState == State::Downed && mCrawler->isStarted();
}

bool ZombieStateComponent::canChase() const {
	return mAnimations->canWalk();
}

ZombieLocomotion::Posture ZombieStateComponent::posture() const {
	return isCrawling() ? ZombieLocomotion::Posture::Crawling : ZombieLocomotion::Posture::Standing;
}

const Vector3& ZombieStateComponent::position() const {
	return *mPosition;
}

Vector3 ZombieStateComponent::bodyCenter() const {
	if ( isCrawling() ) {
		return mCrawler->center();
	}
	return mRootSpan->isActive() ? mRootSpan->anchor() : *mPosition;
}

Vector3 ZombieStateComponent::eyePosition() const {
	Vector3 eye = *mPosition;
	eye.y += kEyeHeight;
	return eye;
}

Vector3 ZombieStateComponent::headPosition() const {
	Vector3 head;
	if ( mHitboxes->partCenter( BodyPart::Head, &head ) ) {
		return head;
	}
	return eyePosition();
}

BoxCollider ZombieStateComponent::collider() const {
	return mModel->footprint().place( bodyCenter(), *mAngleY );	// 体を引きずっている間は、胸の位置に置く（壁に当たるのは胸。頭・足は少しめり込むことがある）
}
