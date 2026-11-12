#include "GameSrc/Zombie/Component/ZombieCrawlComponent.h"
#include "GameSrc/Zombie/Animation/ZombieModel.h"
#include "GameSrc/Zombie/Component/ZombieModelComponent.h"
#include "GameSrc/Zombie/Component/ZombieStateComponent.h"
#include "GameSrc/Zombie/Component/ZombieAttackComponent.h"
#include "GameSrc/Zombie/Component/ZombieChaseComponent.h"
#include "GameSrc/Zombie/Component/ZombieDismemberComponent.h"
#include "GameSrc/Zombie/Component/ZombieRootMotionComponent.h"
#include "GameSrc/Zombie/Move/ChaseTarget.h"
#include "GameSrc/Component/TransformComponent.h"
#include "GameSrc/Component/SkinnedModelComponent.h"
#include "GameSrc/Component/CharacterColliderComponent.h"
#include "GameSrc/World/GameObject.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

ZombieCrawlComponent::ZombieCrawlComponent() :
	mModel( nullptr ),
	mPosition( nullptr ),
	mAngleY( nullptr ),
	mMesh( nullptr ),
	mMover( nullptr ),
	mCrawler( nullptr ),
	mState( nullptr ),
	mLocomotion( nullptr ),
	mRootSpan( nullptr ),
	mAnimations( nullptr ) {
}

void ZombieCrawlComponent::start() {
	mModel = requireComponent< ZombieModelComponent >()->model();
	TransformComponent* transform = requireComponent< TransformComponent >();
	mPosition = &transform->position();
	mAngleY = &transform->angleY();
	mMesh = &requireComponent< SkinnedModelComponent >()->mesh();
	mMover = &requireComponent< CharacterColliderComponent >()->mover();
	mCrawler = &requireComponent< ZombieDismemberComponent >()->crawler();
	mState = requireComponent< ZombieStateComponent >();
	mLocomotion = &requireComponent< ZombieChaseComponent >()->locomotion();
	ZombieRootMotionComponent* rootMotion = requireComponent< ZombieRootMotionComponent >();
	mRootSpan = &rootMotion->span();
	mAnimations = &rootMotion->animations();
}

void ZombieCrawlComponent::update( const ChaseTarget* target, const HitWorld& hitWorld ) {
	if ( !mCrawler->isStarted() ) {
		if ( !canStart() ) {
			return; // まだ倒れている途中（または引きずれない体）。倒れた場所の姿勢のまま
		}
		mRootSpan->end( mPosition ); // ここから先は胸の位置で動かす（足元は絵の根から測り直す）
		startCrawling();
	}
	// 攻撃は外せる部品なので、使うたびに引く（外されていれば、追いついてももがくだけ）
	ZombieAttackComponent* attack = getComponent< ZombieAttackComponent >();
	bool attacking = attack && attack->isActive();
	ZombieCrawler::Effort effort = ZombieCrawler::Effort::Still;
	bool moving = false;
	if ( attacking ) {
		if ( target ) {
			// 腕を振っている間は、相手の方を向くだけ（寄らない）
			mLocomotion->turnToward( mState->bodyCenter(), target->position, mState->posture(), mAngleY );
		}
	} else {
		effort = chase( target, hitWorld, attack, &moving );
	}
	// 腕で引いた分だけ進む。壁・車に当たったら胸の位置で沿って滑る
	float distance = mCrawler->moveArms( effort, attack && attack->attack().isAttacking(), moving );
	if ( distance > 0.f ) {
		mCrawler->moveCenter( mLocomotion->walkStep( distance, *mAngleY ) );
	}
	mCrawler->place( *mAngleY, *mMover, mMesh, mPosition ); // 胸の位置と進む向きから、見た目の向き・足元を決め直す
	// 見えている相手（追っている・攻撃している間）へ顔を向ける。置き場所を決めた後の骨格の根の空間で渡す
	bool looking = target && ( mLocomotion->mode() == ZombieChase::Mode::Chase || ( attack && attack->isActive() ) );
	Vector3 lookPoint( 0.f, 0.f, 0.f );
	if ( looking ) {
		lookPoint = mMesh->toRootPoint( target->eyePosition );
	}
	mCrawler->look( looking, lookPoint );
}

bool ZombieCrawlComponent::canStart() const {
	if ( !mAnimations->hasDowned() || !mState->canChase() || !mModel->crawlLayout().isValid() ) {
		return false; // 前へ倒れるアニメが無い（仰向けかもしれない）・確認用の体・腕の骨が無い
	}
	return mAnimations->hasFinishedDownedFall();
}

void ZombieCrawlComponent::startCrawling() {
	if ( !mCrawler->start( *mMesh, *mPosition, mAngleY ) ) {
		return; // まだ update していない（倒れ終わってから呼ぶので起きない）
	}
	mLocomotion->clearPath();
}

ZombieCrawler::Effort ZombieCrawlComponent::chase( const ChaseTarget* target, const HitWorld& hitWorld, ZombieAttackComponent* attack,
bool* moving ) {
	*moving = false;
	if ( !mCrawler->canPull() ) {
		return ZombieCrawler::Effort::Still; // 両腕を失った。倒れたまま動かない
	}
	ZombieLocomotion::Intent intent = mLocomotion->thinkCrawling( mCrawler->center(), mCrawler->eye(), target, hitWorld, mAngleY );
	if ( intent.wandering ) {
		return ZombieCrawler::Effort::Struggle; // 気付いていない間はうろつかず、その場でもがくだけ
	}
	if ( attack && attack->tryStart( target ) ) {
		return ZombieCrawler::Effort::Still;
	}
	if ( !intent.walking ) {
		return ZombieCrawler::Effort::Struggle; // 追いついて待っている
	}
	// 向きを変えている最中も腕は動かす（その場で向きを変える動きに見せる）。進むのは向き終わってから
	*moving = intent.facing;
	return ZombieCrawler::Effort::Crawl;
}
