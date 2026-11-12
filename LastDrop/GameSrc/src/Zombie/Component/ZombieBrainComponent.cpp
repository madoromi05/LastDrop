#include "GameSrc/Zombie/Component/ZombieBrainComponent.h"
#include "GameSrc/Zombie/Move/ChaseTarget.h"
#include "GameSrc/Zombie/Component/ZombieStateComponent.h"
#include "GameSrc/Zombie/Component/ZombieChaseComponent.h"
#include "GameSrc/Zombie/Component/ZombieAttackComponent.h"
#include "GameSrc/Zombie/Component/ZombieHealthComponent.h"
#include "GameSrc/Zombie/Damage/ZombieFlinch.h"
#include "GameSrc/Zombie/Component/ZombieCrawlComponent.h"
#include "GameSrc/Zombie/Component/ZombieCorpseComponent.h"
#include "GameSrc/Zombie/Component/ZombieDismemberComponent.h"
#include "GameSrc/Zombie/Component/ZombieRootMotionComponent.h"
#include "GameSrc/Component/TransformComponent.h"
#include "GameSrc/Component/SkinnedModelComponent.h"
#include "GameSrc/Component/HitboxComponent.h"
#include "GameSrc/Component/CharacterColliderComponent.h"
#include "GameSrc/World/GameObjectWorld.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

ZombieBrainComponent::ZombieBrainComponent() :
	mContext( nullptr ),
	mAnimationThrottle( 0 ),
	mPosition( nullptr ),
	mMesh( nullptr ),
	mHitboxes( nullptr ),
	mMover( nullptr ),
	mDismember( nullptr ),
	mLocomotion( nullptr ),
	mRootSpan( nullptr ),
	mState( nullptr ),
	mChase( nullptr ) {
}

UpdatePhase ZombieBrainComponent::updatePhase() const {
	return UpdatePhase::Think;
}

void ZombieBrainComponent::start() {
	mContext = shared< TickContext >();
	TransformComponent* transform = requireComponent< TransformComponent >();
	mPosition = &transform->position();
	mMesh = &requireComponent< SkinnedModelComponent >()->mesh();
	mHitboxes = &requireComponent< HitboxComponent >()->boxes();
	mMover = &requireComponent< CharacterColliderComponent >()->mover();
	mDismember = requireComponent< ZombieDismemberComponent >();
	ZombieRootMotionComponent* rootMotion = requireComponent< ZombieRootMotionComponent >();
	mRootSpan = &rootMotion->span();
	mState = requireComponent< ZombieStateComponent >();
	mChase = requireComponent< ZombieChaseComponent >();
	mLocomotion = &mChase->locomotion();
	mAnimationThrottle = AnimationThrottle( gameObject()->id() ); // 湧いた順に番号が増えるので、体ごとに刻みがずれる

	mMesh->setPosition( *mPosition );
	mMesh->setAngleY( transform->angleY() );
	mMesh->setPoseModifier( &mPoseModifiers ); // 中身は update で毎ティック組み直す
	mMesh->setBodyPoseModifier( &mChase->sway() ); // 揺れには部位の箱も付いてくる
	// 気付く前は立っている（立つアニメが無ければ歩くアニメ。どちらも空ならレスト姿勢＝T ポーズ）
	rootMotion->animations().startStanding();
}

void ZombieBrainComponent::update() {
	// 外せる部品（攻撃・体力・這う・死体）は、使う所ごとに引く（外されていれば、その振る舞いを飛ばす）
	ZombieAttackComponent* attack = getComponent< ZombieAttackComponent >();
	ZombieHealthComponent* health = getComponent< ZombieHealthComponent >();
	ChaseTarget target;
	const ChaseTarget* chaseTarget = targetForThisBody( &target );
	// 状態はここで 1 回だけ読む（段の途中で部品が状態を進めても、このティックは読んだ時の状態で最後まで進める。分ける前と同じ）
	ZombieStateComponent::State state = mState->state();
	// 動いた後の位置・向きでアニメと部位の箱を置くので、先に動く
	bool standing = move( chaseTarget, state, attack, health );
	settleOnGround( state );
	pose( chaseTarget, standing, attack, health );
	afterPose( chaseTarget, state, health );
	placeHitboxes();
	claimAttack( attack );
}

const ChaseTarget* ZombieBrainComponent::targetForThisBody( ChaseTarget* out ) const {
	// 追う先は全員で共通。攻撃に行っている体が自分のほかに居れば、攻撃を始めない
	if ( !mContext->target ) {
		return nullptr;
	}
	*out = *mContext->target;
	out->attackTaken = ( mContext->attacker != nullptr && mContext->attacker != gameObject() );
	return out;
}

bool ZombieBrainComponent::move( const ChaseTarget* target, ZombieStateComponent::State state, ZombieAttackComponent* attack, ZombieHealthComponent* health ) {
	const HitWorld& hitWorld = *mContext->hitWorld;
	// 攻撃の間は追う先を考え直さない（相手の方を向いて寄るだけ）
	// 攻撃に行っている体は仲間を避けない（攻撃に行く動きを崩さない。ほかの体が、この体から離れる）
	mLocomotion->beginTick();
	bool staggering = health && health->isStaggering();
	bool standing = ( state == ZombieStateComponent::State::Alive && !staggering && mState->canChase() );
	if ( standing ) {
		if ( attack && attack->isActive() ) {
			attack->approach( target );
		} else {
			chase( target, hitWorld, mContext->crowd, attack );
		}
	} else if ( state == ZombieStateComponent::State::Downed ) {
		if ( ZombieCrawlComponent* crawl = getComponent< ZombieCrawlComponent >() ) {
			crawl->update( target, hitWorld );
		}
	}
	if ( attack && mState->isAlive() ) {
		attack->update( target );
	}
	return standing;
}

void ZombieBrainComponent::settleOnGround( ZombieStateComponent::State state ) {
	// 歩いていない間（立っている・よろけている）も、歩道の上で沈まないよう毎ティック行う
	// 倒れた後は動かないので、倒れた場所の高さのまま（沈む演出は見た目の位置だけを下げる）
	// 根の動きの区間の間は、置き場所を動かした後に合わせる（ZombieRootMotionSpan::apply）
	if ( state == ZombieStateComponent::State::Alive && !mRootSpan->isActive() ) {
		mPosition->y = mMover->followGround( *mPosition );
		mMesh->setPosition( *mPosition );
	}
}

void ZombieBrainComponent::pose( const ChaseTarget* target, bool standing, ZombieAttackComponent* attack, ZombieHealthComponent* health ) {
	bool attacking = attack && attack->isActive(); // 攻撃を進めた（move）後に読む
	mChase->updateSway( target, standing && !attacking ); // 姿勢を求める前に、このティックの揺れを決める
	rebuildPoseModifiers( health );
	// 遠い・映らない体は、姿勢を求めるティックを間引く（時刻だけ進める）。部位の箱は placeHitboxes で毎ティック置き直すので、
	// 体ごと動く所（置き場所・向き）は毎ティック正しく、腕や脚の形だけが前に求めた姿勢のまま付いて行く
	AnimationThrottle::Level level = AnimationThrottle::judge( mContext->view, *mHitboxes, *mPosition );
	if ( mAnimationThrottle.shouldPose( level, mustPoseEveryTick( attack, health ) ) ) {
		mMesh->update();
	} else {
		mMesh->advance();
	}
}

void ZombieBrainComponent::afterPose( const ChaseTarget* target, ZombieStateComponent::State state, ZombieHealthComponent* health ) {
	mRootSpan->apply( target ? target->body : nullptr, mPosition );
	if ( state == ZombieStateComponent::State::Alive ) {
		if ( health ) {
			health->updateStagger();
		}
	} else if ( state != ZombieStateComponent::State::Downed ) { // Downed は倒れるアニメの最後の姿勢のまま（動かすのは腕と置き場所だけ）
		if ( ZombieCorpseComponent* corpse = getComponent< ZombieCorpseComponent >() ) {
			corpse->update();
		}
	}
	if ( health ) {
		health->flinch().update();
	}
}

void ZombieBrainComponent::placeHitboxes() {
	// 体を引きずり始めた後は（死んだ後も）、起こした頭・動かした腕に当たるよう、見えている姿勢に箱を付ける
	// （頭を 0.2m ほど持ち上げるので、アニメどおりの姿勢のままだと頭を狙っても当たらない）
	mHitboxes->update( *mMesh, mDismember->crawler().isStarted() ? HitboxSet::POSE_SHOWN : HitboxSet::POSE_ANIMATED );
}

void ZombieBrainComponent::claimAttack( const ZombieAttackComponent* attack ) {
	// 同じティックに 2 体が攻撃を始めないように
	if ( !mContext->attacker && attack && attack->isAttacking() ) {
		mContext->attacker = gameObject();
	}
}

void ZombieBrainComponent::chase( const ChaseTarget* target, const HitWorld& hitWorld, const ZombieCrowd* crowd, ZombieAttackComponent* attack ) {
	// 考える（見て・行き先を決め・向きを変える）→ 攻撃を始めるか → 動く、の順
	ZombieLocomotion::Intent intent = mChase->think( target, hitWorld, crowd );
	if ( attack && attack->tryStart( target ) ) {
		return;
	}
	mChase->walk( intent, crowd );
}

bool ZombieBrainComponent::mustPoseEveryTick( const ZombieAttackComponent* attack, const ZombieHealthComponent* health ) const {
	return ( attack && attack->isActive() ) || mRootSpan->isActive() || mState->isCrawling() ||
		( health && ( health->isStaggering() || health->flinch().isActive() ) );
}

AnimationThrottle::Level ZombieBrainComponent::animationLevel() const {
	return mAnimationThrottle.level();
}

void ZombieBrainComponent::rebuildPoseModifiers( ZombieHealthComponent* health ) {
	mPoseModifiers.clear();
	mPoseModifiers.add( mDismember->crawler().arms() );
	if ( health ) {
		mPoseModifiers.add( &health->flinch() );
	}
	mPoseModifiers.add( &mDismember->dismemberment() );
}

void ZombieBrainComponent::hearNoise( const Vector3& position ) {
	if ( canHear() ) {
		mLocomotion->hearNoise( position );
	}
}

void ZombieBrainComponent::hearFootsteps() {
	if ( canHear() ) {
		mLocomotion->alert(); // 撃たれたときと同じ。次に線を引いて見えなければ、その時の自キャラの所を探しに行く
	}
}

bool ZombieBrainComponent::canHear() const {
	const ZombieAttackComponent* attack = getComponent< ZombieAttackComponent >();
	const ZombieHealthComponent* health = getComponent< ZombieHealthComponent >();
	bool standing = ( mState->state() == ZombieStateComponent::State::Alive && !( health && health->isStaggering() ) );
	return ( standing || mState->isCrawling() ) && !( attack && attack->isActive() ) && mState->canChase();
}
