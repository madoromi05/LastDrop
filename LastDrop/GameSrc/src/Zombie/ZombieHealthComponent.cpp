#include "GameSrc/Zombie/ZombieHealthComponent.h"
#include "GameSrc/Zombie/ZombieModel.h"
#include "GameSrc/Zombie/ZombieModelComponent.h"
#include "GameSrc/Zombie/ZombieDamageReaction.h"
#include "GameSrc/Zombie/ZombieStateComponent.h"
#include "GameSrc/Zombie/ZombieAttackComponent.h"
#include "GameSrc/Zombie/ZombieChaseComponent.h"
#include "GameSrc/Zombie/ZombieDismemberComponent.h"
#include "GameSrc/Zombie/ZombieRootMotionComponent.h"
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
	mAnimations( nullptr ) {
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
	mAnimations = &requireComponent< ZombieRootMotionComponent >()->animations();
}

void ZombieHealthComponent::applyDamage( const DamageInfo& damage ) {
	mDismemberment->clearSeveredThisHit();
	// 倒れ始めた後に当たった弾は、死体で止まるだけ（体力も反応も変えない）。
	if ( !mState->isAlive() ) {
		return;
	}
	mHealth.takeDamage( damage.part, damage.baseAmount, mRandom.getFloat() ); // 頭は確率で弾ける（クリティカル）
	mLocomotion->alert(); // 撃たれたら、見えていなくても気付く
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
	const ZombieAttackComponent* attack = getComponent< ZombieAttackComponent >(); // 外せる部品なので、使うたびに引く
	facts.isAttacking = attack && attack->isActive();
	facts.canStagger = canStagger( damage.part );
	switch ( ZombieDamageReaction::chooseFall( facts ) ) {
		case ZombieDamageReaction::Fall::Die: startFalling(); return;
		case ZombieDamageReaction::Fall::Down: startDowned(); return;
		case ZombieDamageReaction::Fall::None: break;
	}
	switch ( ZombieDamageReaction::chooseHit( facts ) ) {
		case ZombieDamageReaction::Hit::Stagger: startStagger(); break;
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
		attack->cancel(); // 腕を振っていたらやめる
	}
	mStagger.cancel();
	// 移動の名簿から外す（死体の上を歩ける）。命中の名簿には消えるまで残す（死体も弾を止める）。
	// Downed も外す（立ち姿の長方形が倒れた体と合わないため）。倒れ始めるのは撃たれたとき（名簿をたどっている最中ではない）。
	mCollider->stopBlocking();
}

bool ZombieHealthComponent::canStagger( BodyPart part ) const {
	return mAnimations->hasStagger() && mStagger.canStart( part );
}

void ZombieHealthComponent::startStagger() {
	mAnimations->playStagger( mPosition );
	mStagger.start();
}

void ZombieHealthComponent::updateStagger() {
	if ( !mStagger.isActive() ) {
		mStagger.tick();
		return;
	}
	if ( mMesh->isAnimationFinished() ) {
		// よろける前に歩いていたか立っていたかに戻す（次の update で追う先を考え直し、要れば切り替える）。
		mAnimations->returnToLocomotion( mPosition ); // 腰は下がった後に元の所へ戻っている
		mStagger.finish();
	}
}
