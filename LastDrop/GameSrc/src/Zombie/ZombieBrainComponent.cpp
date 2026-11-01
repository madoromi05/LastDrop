#include "GameSrc/Zombie/ZombieBrainComponent.h"
#include "GameSrc/Zombie/ZombieModel.h"
#include "GameSrc/Zombie/ChaseTarget.h"
#include "GameSrc/Zombie/ZombieUncertaintyParams.h"
#include "GameSrc/Zombie/ZombieDamageReaction.h"
#include "GameSrc/Zombie/ZombieHealthComponent.h"
#include "GameSrc/Zombie/ZombieDismemberComponent.h"
#include "GameSrc/Zombie/ZombieAttackComponent.h"
#include "GameSrc/Zombie/ZombieCorpseComponent.h"
#include "GameSrc/Zombie/ZombieChaseComponent.h"
#include "GameSrc/Zombie/ZombieRootMotionComponent.h"
#include "GameSrc/Component/TransformComponent.h"
#include "GameSrc/Component/SkinnedModelComponent.h"
#include "GameSrc/Component/HitboxComponent.h"
#include "GameSrc/Component/CharacterColliderComponent.h"
#include "GameSrc/World/GameObject.h"
#include "GameSrc/Hit/HitWorld.h"
#include "GameSrc/Hit/DamageInfo.h"
#include "GameSrc/Math/FlatMath.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Functions.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// アニメを繋ぐ長さ・始める時刻・歩く速さは ZombieAnimationSet。
	// ── 追ってくるとき（SummaryPlan/個別仕様/ZombieChase_Plan.txt の 3 章）──
	// 目の高さ（m。足元から）。ここから自キャラの目が見えているかを調べる。身長 1.82m の目の辺り。
	const float kEyeHeight = 1.6f;
}

ZombieBrainComponent::ZombieBrainComponent( const ZombieModel* model, TickContext* context, Random random ) :
	mModel( model ),
	mContext( context ),
	mRandom( random ),
	mState( State::Alive ),
	mPosition( nullptr ),
	mAngleY( nullptr ),
	mMesh( nullptr ),
	mHitboxes( nullptr ),
	mCollider( nullptr ),
	mMover( nullptr ),
	mDismemberment( nullptr ),
	mCrawler( nullptr ),
	mFlinch( nullptr ),
	mHealth( nullptr ),
	mStagger( nullptr ),
	mAttack( nullptr ),
	mCorpse( nullptr ),
	mSway( nullptr ),
	mLocomotion( nullptr ),
	mRootSpan( nullptr ),
	mAnimations( nullptr ) {
	ASSERT( model && context && "ZombieBrainComponent : model or context is null." );
}

UpdatePhase ZombieBrainComponent::updatePhase() const {
	return UpdatePhase::Think;
}

void ZombieBrainComponent::start() {
	TransformComponent* transform = requireComponent< TransformComponent >();
	mPosition = &transform->position();
	mAngleY = &transform->angleY();
	mMesh = &requireComponent< SkinnedModelComponent >()->mesh();
	mHitboxes = &requireComponent< HitboxComponent >()->boxes();
	mCollider = requireComponent< CharacterColliderComponent >();
	mCollider->setShape( this ); // 移動の当たり判定の形はここが答える
	mMover = &mCollider->mover();
	ZombieDismemberComponent* dismember = requireComponent< ZombieDismemberComponent >();
	mDismemberment = &dismember->dismemberment();
	mCrawler = &dismember->crawler();
	ZombieHealthComponent* health = requireComponent< ZombieHealthComponent >();
	mHealth = &health->health();
	mFlinch = &health->flinch();
	mStagger = &health->stagger();
	mAttack = &requireComponent< ZombieAttackComponent >()->attack();
	mCorpse = &requireComponent< ZombieCorpseComponent >()->corpse();
	ZombieChaseComponent* chase = requireComponent< ZombieChaseComponent >();
	mSway = &chase->sway();
	mLocomotion = &chase->locomotion();
	ZombieRootMotionComponent* rootMotion = requireComponent< ZombieRootMotionComponent >();
	mRootSpan = &rootMotion->span();
	mAnimations = &rootMotion->animations();

	mMesh->setPosition( *mPosition );
	mMesh->setAngleY( *mAngleY );
	mPoseModifiers.add( mCrawler->arms() );
	mPoseModifiers.add( mFlinch );
	mPoseModifiers.add( mDismemberment );
	mMesh->setPoseModifier( &mPoseModifiers );
	mMesh->setBodyPoseModifier( mSway ); // 揺れには部位の箱も付いてくる
	// 気付く前は立っている（立つアニメが無ければ歩くアニメ。どちらも空ならレスト姿勢＝T ポーズ）。
	mAnimations->startStanding();
}

void ZombieBrainComponent::update() {
	// 追う先は全員で共通。攻撃に行っている体が自分のほかに居れば、攻撃を始めない。
	ChaseTarget target;
	const ChaseTarget* chaseTarget = nullptr;
	if ( mContext->target ) {
		target = *mContext->target;
		target.attackTaken = ( mContext->attacker != nullptr && mContext->attacker != gameObject() );
		chaseTarget = &target;
	}
	const HitWorld& hitWorld = *mContext->hitWorld;
	// 動いた後の位置・向きでアニメと部位の箱を置くので、先に動く。攻撃の間は追う先を考え直さない（相手の方を向いて寄るだけ）。
	// 攻撃に行っている体は仲間を避けない（攻撃に行く動きを崩さない。ほかの体が、この体から離れる）。
	mLocomotion->beginTick();
	if ( mState == State::Alive && !mStagger->isActive() && canChase() ) {
		if ( mAttack->isActive() ) {
			approachDuringAttack( chaseTarget );
		} else {
			chase( chaseTarget, hitWorld, mContext->crowd );
		}
	} else if ( mState == State::Downed ) {
		updateDowned( chaseTarget, hitWorld );
	}
	if ( isAlive() ) {
		updateAttack( chaseTarget );
	}
	// 足元を地面（歩道の段差）の高さへ合わせる。歩いていない間（立っている・よろけている）も、歩道の上で沈まないよう毎ティック行う。
	// 倒れた後は動かないので、倒れた場所の高さのまま（沈む演出は見た目の位置だけを下げる）。
	// 根の動きの区間の間は、置き場所を動かした後に合わせる（ZombieRootMotionSpan::apply）。
	if ( mState == State::Alive && !mRootSpan->isActive() ) {
		mPosition->y = mMover->followGround( *mPosition );
		mMesh->setPosition( *mPosition );
	}
	mSway->update( swayStrength( chaseTarget ), zombieUncertaintyParams().swayMaxDegree ); // 姿勢を求める前に、このティックの揺れを決める
	mMesh->update();
	mRootSpan->apply( chaseTarget ? chaseTarget->body : nullptr, mPosition );
	if ( mState == State::Alive ) {
		updateStagger();
	} else if ( mState != State::Downed ) { // Downed は倒れるアニメの最後の姿勢のまま（動かすのは腕と置き場所だけ）
		updateAfterDeath();
	}
	mFlinch->update();
	// 体を引きずり始めた後は（死んだ後も）、起こした頭・動かした腕に当たるよう、見えている姿勢に箱を付ける
	// （頭を 0.2m ほど持ち上げるので、アニメどおりの姿勢のままだと頭を狙っても当たらない）。
	mHitboxes->update( *mMesh, mCrawler->isStarted() ? HitboxSet::POSE_SHOWN : HitboxSet::POSE_ANIMATED );
	// 攻撃を始めた体を、後の体に知らせる（同じティックに 2 体が攻撃を始めないように）。
	if ( !mContext->attacker && isAttacking() ) {
		mContext->attacker = gameObject();
	}
}

void ZombieBrainComponent::chase( const ChaseTarget* target, const HitWorld& hitWorld, const ZombieCrowd* crowd ) {
	// 考える（見て・行き先を決め・向きを変える）→ 攻撃を始めるか → 動く、の順（SummaryPlan/個別仕様/ZombieSplit2_Plan.txt の 3 章）。
	Vector3 eye = *mPosition;
	eye.y += kEyeHeight;
	float weaveDegree = mSway->weaveDegree( zombieUncertaintyParams().weaveDegree ); // 揺れている間は、倒れている側へ少し流れる
	ZombieLocomotion::Intent intent = mLocomotion->thinkStanding( *mPosition, eye, target, hitWorld, crowd, weaveDegree, mAngleY );
	if ( canStartAttack( target ) ) {
		mLocomotion->clearPath();
		startAttack();
		mMesh->setAngleY( *mAngleY );
		return;
	}
	// 向きを変えている最中も、歩きたいなら歩くアニメにする（その場で向きを変える動きに見せる）。
	mAnimations->setWalking( intent.walking );
	if ( intent.walking && intent.facing ) {
		*mPosition += mLocomotion->walkStep( mAnimations->walkDistanceThisTick(), *mAngleY );
	} else if ( !intent.walking && mLocomotion->mode() == ZombieChase::Mode::Chase ) {
		// 追いついて立ち止まっている。仲間の後ろに重なったまま止まり続けないように
		mLocomotion->standAside( bodyCenter(), crowd, mPosition );
	}
	mMesh->setPosition( *mPosition );
	mMesh->setAngleY( *mAngleY );
}

float ZombieBrainComponent::swayStrength( const ChaseTarget* target ) const {
	// 遠くでは揺らさない（遠くから頭を狙う楽しさは残す）。音で探しに来る・うろつく間も揺らさない。
	if ( !target || mState != State::Alive || mStagger->isActive() || mAttack->isActive() || !canChase() ||
		mLocomotion->mode() != ZombieChase::Mode::Chase ) {
		return 0.f;
	}
	const ZombieUncertaintyParams& params = zombieUncertaintyParams();
	return ZombieSway::strengthAt( flatDistanceTo( target->position ), params.swayStartDistance, params.swayFullDistance );
}

bool ZombieBrainComponent::canStartAttack( const ChaseTarget* target ) const {
	if ( !mAnimations->hasAttack() || !target || !target->attackable || target->attackTaken || !mAttack->canStart() ) {
		return false;
	}
	if ( mLocomotion->mode() != ZombieChase::Mode::Chase ) {
		return false; // 見えている相手だけを襲う（見失って探している間・うろついている間は襲わない）
	}
	if ( isCrawling() && !mCrawler->canPull() ) {
		return false; // 両腕を失った体は攻撃できない
	}
	return ZombieAttackReach::canStart( attackOrigin(), target->position );
}

void ZombieBrainComponent::startAttack() {
	mAttack->start();
	if ( isCrawling() ) {
		// 攻撃のアニメは流さない（倒れた姿勢のまま、腕を前へ伸ばす）。段階はアニメと同じ時間割の時計で進める（updateAttack）。
		return;
	}
	mAnimations->playAttack( mPosition ); // 終わったら、まず立つ（次の update で追う先を考え直し、離れていれば歩き出す）
}

void ZombieBrainComponent::approachDuringAttack( const ChaseTarget* target ) {
	if ( !target ) {
		return;
	}
	mLocomotion->turnToward( bodyCenter(), target->position, posture(), mAngleY );
	// 寄る距離は、踏み込む前の足元（根の動きのアンカー＝bodyCenter）から測る。寄るのもアンカーごと（踏み込みは別に足す）。
	if ( mAttack->phase() == ZombieAttack::Phase::Swing ) {
		float distance = ZombieAttackReach::approachStep( bodyCenter(), target->position );
		if ( distance > 0.f ) {
			mRootSpan->moveFooting( mLocomotion->walkStep( distance, *mAngleY ), mPosition ); // 相手の体・銃の箱に当たったら、そこで止まる
		}
	}
	mMesh->setPosition( *mPosition );
	mMesh->setAngleY( *mAngleY );
}

void ZombieBrainComponent::updateAttack( const ChaseTarget* target ) {
	bool wasActive = mAttack->isActive();
	if ( isCrawling() ) {
		mAttack->updateWithoutAnimation( mAnimations->attackPeriod(), isTargetInReach( target ) );
		return; // 腕を伸ばす・戻すのは ZombieCrawl（updateDowned）。アニメは替えない
	}
	mAttack->update( mMesh->animationTime(), mMesh->isAnimationFinished(), isTargetInReach( target ) );
	if ( wasActive && !mAttack->isActive() ) {
		mAnimations->returnToLocomotion( mPosition ); // 攻撃のアニメの最後（元の立ち姿）から、立つアニメへ戻す
	}
}

bool ZombieBrainComponent::isTargetInReach( const ChaseTarget* target ) const {
	if ( !target || !target->attackable ) {
		return false;
	}
	return ZombieAttackReach::canHit( attackOrigin(), target->position );
}

ZombieAttackReach::Origin ZombieBrainComponent::attackOrigin() const {
	ZombieAttackReach::Origin origin;
	origin.center = bodyCenter();
	origin.crawling = isCrawling();
	// 寝ている体は足元から前へ 1.5m ほど伸びているので、手の届く距離は頭から測る。
	origin.reachFrom = origin.crawling ? headPosition() : origin.center;
	origin.angleY = *mAngleY;
	return origin;
}

float ZombieBrainComponent::flatDistanceTo( const Vector3& target ) const {
	return FlatMath::distance( bodyCenter(), target );
}

bool ZombieBrainComponent::canChase() const {
	return mAnimations->canWalk();
}

void ZombieBrainComponent::applyDamage( const DamageInfo& damage ) {
	mDismemberment->clearSeveredThisHit();
	// 倒れ始めた後に当たった弾は、死体で止まるだけ（体力も反応も変えない）。
	if ( !isAlive() ) {
		return;
	}
	mHealth->takeDamage( damage.part, damage.baseAmount, mRandom.getFloat() ); // 頭は確率で弾ける（クリティカル）
	mLocomotion->alert(); // 撃たれたら、見えていなくても気付く
	// のけぞりは骨格の根の空間で曲げるので、弾の向きを置き場所の向きの分だけ戻す。
	mFlinch->push( damage.part, mMesh->toRootDirection( damage.direction ) );
	// 倒れる 1 発で部位も 0 になったら、切れてから倒れる（頭を撃って倒したときは、頭が飛ぶ）。
	severIfBroken( damage.part );
	// どう反応するかを選び（ZombieDamageReaction）、ここで動かす。
	ZombieDamageReaction::Facts facts;
	facts.canFall = mAnimations->hasDeath();
	facts.isDead = mHealth->isDead();
	facts.isStanding = ( mState == State::Alive );
	facts.hasLostFooting = mDismemberment->hasLostFooting();
	facts.isAttacking = mAttack->isActive();
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

void ZombieBrainComponent::severIfBroken( BodyPart part ) {
	if ( !mHealth->isBroken( part ) || !mDismemberment->sever( part ) ) {
		return; // まだ残っている・切れない部位（胴）・もう無い
	}
	for ( BodyPart lostPart : mModel->severLayout().cut( part ).parts ) {
		mHitboxes->removePart( lostPart );
	}
}

void ZombieBrainComponent::startFalling() {
	// 撃たれた瞬間に見えていた姿勢から繋ぐ（歩き・よろけのどの瞬間に倒れても、姿勢が飛ばない）。
	// Downed は同じ死亡アニメで倒れている途中・倒れ終わりなので、繋ぎ直さずそのまま続ける（倒れ終わっていれば次の update で Corpse）。
	// 根の動きも、立っていたときだけ始め直す（Downed は倒れる区間をそのまま続ける。引きずり始めた後は区間が無い）。
	if ( mState == State::Alive ) {
		mAnimations->playDeath( mPosition );
	}
	mState = State::Falling;
	mAttack->cancel(); // 腕を振っていたらやめる
	mStagger->cancel();
	stopBlocking();
}

void ZombieBrainComponent::startDowned() {
	// 前へ倒れるアニメ（無ければ死亡アニメ）で倒す。倒れ終わったら、その姿勢のまま腕で体を引きずる（ZombieCrawl_Plan.txt の 2 章）。
	mAnimations->playDowned( mPosition );
	mState = State::Downed;
	mAttack->cancel();
	mStagger->cancel();
	stopBlocking();
}

void ZombieBrainComponent::stopBlocking() {
	// 移動の名簿から外す（死体の上を歩ける）。命中の名簿には消えるまで残す（死体も弾を止める）。
	// Downed も外す（立ち姿の長方形が倒れた体と合わないため）。倒れ始めるのは撃たれたとき（名簿をたどっている最中ではない）。
	mCollider->stopBlocking();
}

void ZombieBrainComponent::updateDowned( const ChaseTarget* target, const HitWorld& hitWorld ) {
	if ( !mCrawler->isStarted() ) {
		if ( !canStartCrawling() ) {
			return; // まだ倒れている途中（または引きずれない体）。倒れた場所の姿勢のまま
		}
		mRootSpan->end( mPosition ); // ここから先は胸の位置で動かす（足元は絵の根から測り直す）
		startCrawling();
	}
	ZombieCrawler::Effort effort = ZombieCrawler::Effort::Still;
	bool moving = false;
	if ( mAttack->isActive() ) {
		if ( target ) {
			mLocomotion->turnToward( bodyCenter(), target->position, posture(), mAngleY ); // 腕を振っている間は、相手の方を向くだけ（寄らない）
		}
	} else {
		effort = crawlChase( target, hitWorld, &moving );
	}
	// 腕で引いた分だけ進む。壁・車に当たったら胸の位置で沿って滑る。
	float distance = mCrawler->moveArms( effort, mAttack->isAttacking(), moving );
	if ( distance > 0.f ) {
		mCrawler->moveCenter( mLocomotion->walkStep( distance, *mAngleY ) );
	}
	mCrawler->place( *mAngleY, *mMover, mMesh, mPosition ); // 胸の位置と進む向きから、見た目の向き・足元を決め直す
	// 見えている相手（追っている・攻撃している間）へ顔を向ける。置き場所を決めた後の骨格の根の空間で渡す。
	bool looking = target && ( mLocomotion->mode() == ZombieChase::Mode::Chase || mAttack->isActive() );
	Vector3 lookPoint( 0.f, 0.f, 0.f );
	if ( looking ) {
		lookPoint = mMesh->toRootPoint( target->eyePosition );
	}
	mCrawler->look( looking, lookPoint );
}

bool ZombieBrainComponent::canStartCrawling() const {
	if ( !mAnimations->hasDowned() || !canChase() || !mModel->crawlLayout().isValid() ) {
		return false; // 前へ倒れるアニメが無い（仰向けかもしれない）・確認用の体・腕の骨が無い
	}
	return mAnimations->hasFinishedDownedFall();
}

void ZombieBrainComponent::startCrawling() {
	if ( !mCrawler->start( *mMesh, *mPosition, mAngleY ) ) {
		return; // まだ update していない（倒れ終わってから呼ぶので起きない）
	}
	mLocomotion->clearPath();
}

ZombieCrawler::Effort ZombieBrainComponent::crawlChase( const ChaseTarget* target, const HitWorld& hitWorld, bool* moving ) {
	*moving = false;
	if ( !mCrawler->canPull() ) {
		return ZombieCrawler::Effort::Still; // 両腕を失った。倒れたまま動かない
	}
	ZombieLocomotion::Intent intent = mLocomotion->thinkCrawling( mCrawler->center(), mCrawler->eye(), target, hitWorld, mAngleY );
	if ( intent.wandering ) {
		return ZombieCrawler::Effort::Struggle; // 気付いていない間はうろつかず、その場でもがくだけ
	}
	if ( canStartAttack( target ) ) {
		mLocomotion->clearPath();
		startAttack();
		return ZombieCrawler::Effort::Still;
	}
	if ( !intent.walking ) {
		return ZombieCrawler::Effort::Struggle; // 追いついて待っている
	}
	// 向きを変えている最中も腕は動かす（その場で向きを変える動きに見せる）。進むのは向き終わってから。
	*moving = intent.facing;
	return ZombieCrawler::Effort::Crawl;
}

ZombieLocomotion::Posture ZombieBrainComponent::posture() const {
	return isCrawling() ? ZombieLocomotion::Posture::Crawling : ZombieLocomotion::Posture::Standing;
}

bool ZombieBrainComponent::isCrawling() const {
	return mState == State::Downed && mCrawler->isStarted();
}

Vector3 ZombieBrainComponent::bodyCenter() const {
	if ( isCrawling() ) {
		return mCrawler->center();
	}
	return mRootSpan->isActive() ? mRootSpan->anchor() : *mPosition;
}

const std::vector< BodyPart >& ZombieBrainComponent::severedByLastDamage() const {
	return mDismemberment->severedThisHit();
}

void ZombieBrainComponent::shownWorldPose( std::vector< Matrix34 >* out ) const {
	mMesh->shownWorldPose( out );
}

bool ZombieBrainComponent::canStagger( BodyPart part ) const {
	return mAnimations->hasStagger() && mStagger->canStart( part );
}

void ZombieBrainComponent::startStagger() {
	mAnimations->playStagger( mPosition );
	mStagger->start();
}

void ZombieBrainComponent::updateStagger() {
	if ( !mStagger->isActive() ) {
		mStagger->tick();
		return;
	}
	if ( mMesh->isAnimationFinished() ) {
		// よろける前に歩いていたか立っていたかに戻す（次の update で追う先を考え直し、要れば切り替える）。
		mAnimations->returnToLocomotion( mPosition ); // 腰は下がった後に元の所へ戻っている
		mStagger->finish();
	}
}

void ZombieBrainComponent::updateAfterDeath() {
	switch ( mState ) {
		case State::Falling:
			if ( mMesh->isAnimationFinished() ) {
				mState = State::Corpse;
				mCorpse->start();
				mRootSpan->freeze(); // 置き場所は死体の腰の下・絵の根はそのまま（もう動かない）
			}
			break;
		case State::Corpse:
			mCorpse->tick();
			if ( mCorpse->isSinking() ) {
				mState = State::Sinking; // 沈めるのは次のティックから
			}
			break;
		case State::Sinking: {
			// 見た目の位置だけ下げる（部位の箱も見た目に付いて沈む）。沈み終わったら isGone が true になり、持ち主が消す。
			mCorpse->tick();
			Vector3 shown = mRootSpan->meshRoot( *mPosition );
			shown.y -= mCorpse->sinkDepth();
			mMesh->setPosition( shown );
			break;
		}
		case State::Alive:
		case State::Downed:
			break;
	}
}

ZombieBrainComponent::State ZombieBrainComponent::state() const {
	return mState;
}

bool ZombieBrainComponent::isAlive() const {
	return mState == State::Alive || mState == State::Downed;
}

void ZombieBrainComponent::hearNoise( const Vector3& position ) {
	bool listening = ( mState == State::Alive && !mStagger->isActive() ) || isCrawling();
	if ( !listening || mAttack->isActive() || !canChase() ) {
		return; // 倒れている途中・死んだ・よろけている・攻撃している体と、追ってこない確認用の体は聞かない
	}
	mLocomotion->hearNoise( position );
}

bool ZombieBrainComponent::isGone() const {
	return mState == State::Sinking && mCorpse->isGone();
}

float ZombieBrainComponent::secondsUntilGone() const {
	switch ( mState ) {
		case State::Corpse:
		case State::Sinking: return mCorpse->secondsUntilGone();
		case State::Alive:
		case State::Downed:
		case State::Falling: break;
	}
	return 0.f;
}

BoxCollider ZombieBrainComponent::collider() const {
	// 体を引きずっている間は、胸の位置に置く（壁に当たるのは胸。頭・足は少しめり込むことがある）。
	return mModel->footprint().place( bodyCenter(), *mAngleY );
}

// 攻撃は立っている体と、体を引きずっている体だけがする（倒れ始めたら ZombieAttack::cancel 済み）。
bool ZombieBrainComponent::isAttacking() const {
	return isAlive() && mAttack->isAttacking();
}

bool ZombieBrainComponent::hitTargetThisTick() const {
	return isAlive() && mAttack->hitThisTick();
}

Vector3 ZombieBrainComponent::headPosition() const {
	Vector3 head;
	if ( mHitboxes->partCenter( BodyPart::Head, &head ) ) {
		return head;
	}
	Vector3 eye = *mPosition;
	eye.y += kEyeHeight;
	return eye;
}

const Vector3& ZombieBrainComponent::position() const {
	return *mPosition;
}
