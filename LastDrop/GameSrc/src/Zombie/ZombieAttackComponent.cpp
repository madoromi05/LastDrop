#include "GameSrc/Zombie/ZombieAttackComponent.h"
#include "GameSrc/Zombie/ZombieStateComponent.h"
#include "GameSrc/Zombie/ZombieChaseComponent.h"
#include "GameSrc/Zombie/ZombieDismemberComponent.h"
#include "GameSrc/Zombie/ZombieRootMotionComponent.h"
#include "GameSrc/Zombie/ChaseTarget.h"
#include "GameSrc/Component/TransformComponent.h"
#include "GameSrc/Component/SkinnedModelComponent.h"
#include "GameSrc/World/GameObject.h"

ZombieAttackComponent::ZombieAttackComponent() :
	mPosition( nullptr ),
	mAngleY( nullptr ),
	mMesh( nullptr ),
	mState( nullptr ),
	mLocomotion( nullptr ),
	mCrawler( nullptr ),
	mRootSpan( nullptr ),
	mAnimations( nullptr ) {
}

void ZombieAttackComponent::start() {
	TransformComponent* transform = requireComponent< TransformComponent >();
	mPosition = &transform->position();
	mAngleY = &transform->angleY();
	mMesh = &requireComponent< SkinnedModelComponent >()->mesh();
	mState = requireComponent< ZombieStateComponent >();
	mLocomotion = &requireComponent< ZombieChaseComponent >()->locomotion();
	mCrawler = &requireComponent< ZombieDismemberComponent >()->crawler();
	ZombieRootMotionComponent* rootMotion = requireComponent< ZombieRootMotionComponent >();
	mRootSpan = &rootMotion->span();
	mAnimations = &rootMotion->animations();
}

bool ZombieAttackComponent::tryStart( const ChaseTarget* target ) {
	if ( !canStart( target ) ) {
		return false;
	}
	mLocomotion->clearPath();
	mAttack.start();
	if ( mState->isCrawling() ) {
		// 攻撃のアニメは流さない（倒れた姿勢のまま、腕を前へ伸ばす）。段階はアニメと同じ時間割の時計で進める（update）。
		return true;
	}
	mAnimations->playAttack( mPosition ); // 終わったら、まず立つ（次の update で追う先を考え直し、離れていれば歩き出す）
	return true;
}

void ZombieAttackComponent::approach( const ChaseTarget* target ) {
	if ( !target ) {
		return;
	}
	mLocomotion->turnToward( mState->bodyCenter(), target->position, mState->posture(), mAngleY );
	// 寄る距離は、踏み込む前の足元（根の動きのアンカー＝bodyCenter）から測る。寄るのもアンカーごと（踏み込みは別に足す）。
	if ( mAttack.phase() == ZombieAttack::Phase::Swing ) {
		float distance = ZombieAttackReach::approachStep( mState->bodyCenter(), target->position );
		if ( distance > 0.f ) {
			mRootSpan->moveFooting( mLocomotion->walkStep( distance, *mAngleY ), mPosition ); // 相手の体・銃の箱に当たったら、そこで止まる
		}
	}
	mMesh->setPosition( *mPosition );
	mMesh->setAngleY( *mAngleY );
}

void ZombieAttackComponent::update( const ChaseTarget* target ) {
	bool wasActive = mAttack.isActive();
	if ( mState->isCrawling() ) {
		mAttack.updateWithoutAnimation( mAnimations->attackPeriod(), isTargetInReach( target ) );
		return; // 腕を伸ばす・戻すのは ZombieCrawler（ZombieCrawlComponent）。アニメは替えない
	}
	mAttack.update( mMesh->animationTime(), mMesh->isAnimationFinished(), isTargetInReach( target ) );
	if ( wasActive && !mAttack.isActive() ) {
		mAnimations->returnToLocomotion( mPosition ); // 攻撃のアニメの最後（元の立ち姿）から、立つアニメへ戻す
	}
}

void ZombieAttackComponent::cancel() {
	mAttack.cancel();
}

bool ZombieAttackComponent::isActive() const {
	return mAttack.isActive();
}

// 攻撃は立っている体と、体を引きずっている体だけがする（倒れ始めたら cancel 済み）。
bool ZombieAttackComponent::isAttacking() const {
	return mState->isAlive() && mAttack.isAttacking();
}

bool ZombieAttackComponent::hitTargetThisTick() const {
	return mState->isAlive() && mAttack.hitThisTick();
}

bool ZombieAttackComponent::canStart( const ChaseTarget* target ) const {
	if ( !mAnimations->hasAttack() || !target || !target->attackable || target->attackTaken || !mAttack.canStart() ) {
		return false;
	}
	if ( mLocomotion->mode() != ZombieChase::Mode::Chase ) {
		return false; // 見えている相手だけを襲う（見失って探している間・うろついている間は襲わない）
	}
	if ( mState->isCrawling() && !mCrawler->canPull() ) {
		return false; // 両腕を失った体は攻撃できない
	}
	return ZombieAttackReach::canStart( origin(), target->position );
}

bool ZombieAttackComponent::isTargetInReach( const ChaseTarget* target ) const {
	if ( !target || !target->attackable ) {
		return false;
	}
	return ZombieAttackReach::canHit( origin(), target->position );
}

ZombieAttackReach::Origin ZombieAttackComponent::origin() const {
	ZombieAttackReach::Origin origin;
	origin.center = mState->bodyCenter();
	origin.crawling = mState->isCrawling();
	// 寝ている体は足元から前へ 1.5m ほど伸びているので、手の届く距離は頭から測る。
	origin.reachFrom = origin.crawling ? mState->headPosition() : origin.center;
	origin.angleY = *mAngleY;
	return origin;
}
