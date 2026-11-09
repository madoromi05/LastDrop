#include "GameSrc/Zombie/Component/ZombieHealthComponent.h"
#include "GameSrc/Zombie/Animation/ZombieModel.h"
#include "GameSrc/Zombie/Component/ZombieModelComponent.h"
#include "GameSrc/Zombie/Damage/ZombieDamageReaction.h"
#include "GameSrc/Zombie/Component/ZombieStateComponent.h"
#include "GameSrc/Zombie/Component/ZombieAttackComponent.h"
#include "GameSrc/Zombie/Component/ZombieChaseComponent.h"
#include "GameSrc/Zombie/Component/ZombieDismemberComponent.h"
#include "GameSrc/Zombie/Component/ZombieRootMotionComponent.h"
#include "GameSrc/Zombie/Move/ZombieRunParams.h"
#include "GameSrc/Zombie/Move/ZombieLocomotion.h"
#include "GameSrc/Zombie/Animation/ZombieRootMotionSpan.h"
#include "GameSrc/Render/SkinnedMesh/SkinnedMeshInstance.h"
#include "GameSrc/Math/AngleMath.h"
#include "GameSrc/Component/TransformComponent.h"
#include "GameSrc/Component/SkinnedModelComponent.h"
#include "GameSrc/Component/HitboxComponent.h"
#include "GameSrc/Component/CharacterColliderComponent.h"
#include "GameSrc/World/GameObject.h"
#include "GameSrc/Hit/DamageInfo.h"

#include "GameLib/GameLib.h"

ZombieHealthComponent::ZombieHealthComponent( float toughness, GameLib::Math::Random random ) :
	mRandom( random ),
	mHealth( toughness ),
	mModel( nullptr ),
	mPosition( nullptr ),
	mMesh( nullptr ),
	mHitboxes( nullptr ),
	mCollider( nullptr ),
	mDismemberment( nullptr ),
	mState( nullptr ),
	mLocomotion( nullptr ),
	mAnimations( nullptr ),
	mRootSpan( nullptr ) {
}

ZombieHealthComponent::~ZombieHealthComponent() {
}

void ZombieHealthComponent::awake() {
	mFlinch = std::make_unique< ZombieFlinch >( &requireComponent< ZombieModelComponent >()->model()->flinchLayout() );
}

void ZombieHealthComponent::start() {
	mModel = requireComponent< ZombieModelComponent >()->model();
	mPosition = &requireComponent< TransformComponent >()->position();
	mMesh = &requireComponent< SkinnedModelComponent >()->mesh();
	mHitboxes = &requireComponent< HitboxComponent >()->boxes();
	mCollider = requireComponent< CharacterColliderComponent >();
	mDismemberment = &requireComponent< ZombieDismemberComponent >()->dismemberment();
	mState = requireComponent< ZombieStateComponent >();
	mLocomotion = &requireComponent< ZombieChaseComponent >()->locomotion();
	ZombieRootMotionComponent* rootMotion = requireComponent< ZombieRootMotionComponent >();
	mAnimations = &rootMotion->animations();
	mRootSpan = &rootMotion->span();
}

void ZombieHealthComponent::applyDamage( const DamageInfo& damage ) {
	mDismemberment->clearSeveredThisHit();
	// 倒れ始めた後に当たった弾は、死体で止まるだけ（体力も反応も変えない）。
	if ( !mState->isAlive() ) {
		return;
	}
	// 振りほどきには、部位の倍率を掛けた後の量を数える（全体の体力が減った分）。
	float totalBefore = mHealth.total();
	mHealth.takeDamage( damage.part, damage.baseAmount, mRandom.getFloat() ); // 頭は確率で弾ける（クリティカル）
	float dealt = totalBefore - mHealth.total();
	mLocomotion->alert(); // 撃たれたら、見えていなくても気付く
	// 走る体は、脚が傷んだらもう走らない（歩きに落ちる。脚を撃って足止めできる。R4_Plan.txt Q7）。
	if ( mHealth.weakestLegRatio() <= zombieRunParams().legSlowRatio ) {
		mLocomotion->stopRunning();
	}
	// のけぞりは骨格の根の空間で曲げるので、弾の向きを置き場所の向きの分だけ戻す。
	mFlinch->push( damage.part, mMesh->toRootDirection( damage.direction ) );
	// 倒れる 1 発で部位も 0 になったら、切れてから倒れる（頭を撃って倒したときは、頭が飛ぶ）。
	severIfBroken( damage.part );
	// どう反応するかを選び（ZombieDamageReaction）、ここで動かす。
	ZombieDamageReaction::Facts facts;
	facts.canFall = mAnimations->hasDeath();
	facts.isDead = mHealth.isDead();
	facts.isStanding = ( mState->state() == ZombieStateComponent::State::Alive );
	facts.hasLostFooting = mDismemberment->hasLostFooting();
	ZombieAttackComponent* attack = getComponent< ZombieAttackComponent >(); // 外せる部品なので、使うたびに引く
	// isAttacking は振りほどきを数える前に読むが、数えて値が変わるのは振りほどかれたとき（よろけを選ばない）だけなので同じ答えになる。
	facts.isAttacking = attack && attack->isActive();
	facts.canStagger = canStagger( damage.part );
	switch ( ZombieDamageReaction::chooseFall( facts ) ) {
		case ZombieDamageReaction::Fall::Die: startFalling(); return;
		case ZombieDamageReaction::Fall::Down: startDowned(); return;
		case ZombieDamageReaction::Fall::None: break;
	}
	bool shakenOff = attack && attack->attack().takeShakeDamage( dealt ); // 倒れる 1 発では数えない
	switch ( ZombieDamageReaction::chooseHit( facts, shakenOff ) ) {
		case ZombieDamageReaction::Hit::Stagger: startStagger( damage.direction ); break;
		case ZombieDamageReaction::Hit::ShakeOff: shakeOff( damage.direction ); break;
		case ZombieDamageReaction::Hit::None: break;
	}
}

void ZombieHealthComponent::severIfBroken( BodyPart part ) {
	if ( !mHealth.isBroken( part ) || !mDismemberment->sever( part ) ) {
		return; // まだ残っている・切れない部位（胴）・もう無い
	}
	for ( BodyPart lostPart : mModel->severLayout().cut( part ).parts ) {
		mHitboxes->removePart( lostPart );
	}
}

void ZombieHealthComponent::startFalling() {
	// 撃たれた瞬間に見えていた姿勢から繋ぐ（歩き・よろけのどの瞬間に倒れても、姿勢が飛ばない）。
	// Downed は同じ死亡アニメで倒れている途中・倒れ終わりなので、繋ぎ直さずそのまま続ける（倒れ終わっていれば次の update で Corpse）。
	// 根の動きも、立っていたときだけ始め直す（Downed は倒れる区間をそのまま続ける。引きずり始めた後は区間が無い）。
	if ( mState->state() == ZombieStateComponent::State::Alive ) {
		mAnimations->playDeath( mPosition );
	}
	mState->setState( ZombieStateComponent::State::Falling );
	stopActing();
}

void ZombieHealthComponent::startDowned() {
	// 前へ倒れるアニメ（無ければ死亡アニメ）で倒す。倒れ終わったら、その姿勢のまま腕で体を引きずる（ZombieCrawl_Plan.txt の 2 章）。
	mAnimations->playDowned( mPosition );
	mState->setState( ZombieStateComponent::State::Downed );
	stopActing();
}

void ZombieHealthComponent::stopActing() {
	if ( ZombieAttackComponent* attack = getComponent< ZombieAttackComponent >() ) {
		attack->cancel(); // 攻撃していたらやめる（つかんでいたら離す）
	}
	mStagger.cancel();
	mKnockback.cancel();
	// 移動の名簿から外す（死体の上を歩ける）。命中の名簿には消えるまで残す（死体も弾を止める）。
	// Downed も外す（立ち姿の長方形が倒れた体と合わないため）。倒れ始めるのは撃たれたとき（名簿をたどっている最中ではない）。
	mCollider->stopBlocking();
}

bool ZombieHealthComponent::canStagger( BodyPart part ) const {
	return mAnimations->hasStagger() && mStagger.canStart( part );
}

void ZombieHealthComponent::startStagger( const GameLib::Math::Vector3& push ) {
	mAnimations->playStagger( mPosition );
	mStagger.start();
	mKnockback.start( push );
}

void ZombieHealthComponent::shakeOff( const GameLib::Math::Vector3& push ) {
	// 離すときは、よろけて手を放す（よろけのアニメが無ければ、立つアニメへ戻すだけ）。よろけの間あけは待たない。
	if ( mAnimations->hasStagger() ) {
		startStagger( push );
	} else {
		mAnimations->returnToLocomotion( mPosition );
	}
}

void ZombieHealthComponent::updateStagger() {
	if ( !mStagger.isActive() ) {
		mStagger.tick();
		return;
	}
	updateKnockback();
	if ( mMesh->isAnimationFinished() ) {
		// よろける前に歩いていたか立っていたかに戻す（次の update で追う先を考え直し、要れば切り替える）。
		// gethit の腰は下がった後に元の所へ戻っている（押し下げた分は戻らない）。
		mAnimations->returnToLocomotion( mPosition );
		mStagger.finish();
		mKnockback.cancel(); // 秒を長くして gethit より後まで残っても、歩き出してからは押さない
	}
}

void ZombieHealthComponent::updateKnockback() {
	GameLib::Math::Vector3 push = mKnockback.step();
	float distance = push.length();
	if ( distance <= 0.f ) {
		return;
	}
	// 移動の箱は根の動きのアンカーにあるので、そこから当たりながら押し、アンカーごと動かす（次の apply から、押した所を元に根の動きを足す）。
	// 絵の根も同じだけ動かす（このティックの apply は済んでいるので、ここで置き直す）。
	GameLib::Math::Vector3 moved = mLocomotion->walkStep( distance, AngleMath::headingOf( push ) );
	mRootSpan->moveFooting( moved, mPosition );
	mMesh->setPosition( mRootSpan->meshRoot( *mPosition ) );
}
