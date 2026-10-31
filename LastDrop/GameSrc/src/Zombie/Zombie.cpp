#include "GameSrc/Zombie/Zombie.h"
#include "GameSrc/Zombie/ZombieModel.h"
#include "GameSrc/Zombie/ChaseTarget.h"
#include "GameSrc/Zombie/ZombieUncertaintyParams.h"
#include "GameSrc/Zombie/ZombieDamageReaction.h"
#include "GameSrc/Collision/CollisionWorld.h"
#include "GameSrc/Hit/HitWorld.h"
#include "GameSrc/Hit/DamageInfo.h"
#include "GameSrc/Math/AngleMath.h"
#include "GameSrc/Math/FlatMath.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Matrix34.h"
#include "GameLib/Math/Functions.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// アニメを繋ぐ長さ・始める時刻・歩く速さは ZombieAnimationSet。
	// ── 追ってくるとき（SummaryPlan/個別仕様/ZombieChase_Plan.txt の 3 章）──
	// 目の高さ（m。足元から）。ここから自キャラの目が見えているかを調べる。身長 1.82m の目の辺り。
	const float kEyeHeight = 1.6f;
	// ── 攻撃（SummaryPlan/個別仕様/ZombieAttack_Plan.txt の 3 章。間合いは ZombieAttackReach）──
	// 噛むときに首・背骨を突き出す向き（骨格の根の空間で前。押し方は ZombieFlinch::lunge）。
	const Vector3 kBiteLungeDirection( 0.f, 0.f, 1.f );
	// ── 脚を失って体を引きずる（SummaryPlan/個別仕様/ZombieCrawl_Plan.txt。目の高さ・腕の速さは ZombieCrawler）──
	// 噛むときに首・背骨を押す向き（骨格の根の空間で上）。寝ている体は頭が前を向いているので、頭を持ち上げて脚へ噛みつく。
	const Vector3 kCrawlBiteLungeDirection( 0.f, 1.f, 0.f );

	// ── 体力の個体差（SummaryPlan/個別仕様/ZombieUncertainty_Plan.txt の 3 章。幅から選ぶのは ZombieUncertaintyParams）──
	// T ポーズの確認用（歩くアニメが無い）の体の丈夫さ。表のままにして、部位ごとの減り方を確かめられるようにする。
	const float kFixedToughness = 1.f;
}

Zombie::Zombie( const ZombieModel* model, const CollisionWorld* collisionWorld, const NavMesh* navMesh,
const Vector3& position, float angleY, const Animations& animations, int sightPhase, int randomSeed ) :
	mModel( model ),
	mRandom( Random::create( randomSeed ) ),
	mDismemberment( &model->severLayout() ),
	mCrawler( &model->crawlLayout(), &mDismemberment ),
	mFlinch( &model->flinchLayout() ),
	mSway( &model->flinchLayout(), zombieUncertaintyParams().pickSwayPeriod( &mRandom ), mRandom.getFloat() ),
	mMesh( model->data() ),
	mHitboxes( &model->hitboxLayout() ),
	mHealth( animations.move ? zombieUncertaintyParams().pickToughness( &mRandom ) : kFixedToughness ),
	mPosition( position ),
	mAngleY( AngleMath::wrapSigned( angleY ) ),
	mMover( collisionWorld, this ), // this は覚えるだけ（当たり判定を読むのは move のとき＝構築の後）
	mRootSpan( &mMesh, &mMover ),
	mAnimations( model, animations, &mMesh, &mRootSpan ),
	mLocomotion( this, navMesh, &mMover, sightPhase, mRandom.getInt() ), // 丈夫さと同じ種のままだと、選ぶ値の並びが揃うので引き直す
	mState( State::Alive ) {
	mMesh.setPosition( mPosition );
	mMesh.setAngleY( mAngleY );
	mPoseModifiers.add( mCrawler.arms() );
	mPoseModifiers.add( &mFlinch );
	mPoseModifiers.add( &mDismemberment );
	mMesh.setPoseModifier( &mPoseModifiers );
	mMesh.setBodyPoseModifier( &mSway ); // 揺れには部位の箱も付いてくる
	// 気付く前は立っている（立つアニメが無ければ歩くアニメ。どちらも空ならレスト姿勢＝T ポーズ）。
	mAnimations.startStanding();
}

void Zombie::update( const ChaseTarget* target, const HitWorld& hitWorld, const ZombieCrowd* crowd ) {
	// 動いた後の位置・向きでアニメと部位の箱を置くので、先に動く。攻撃の間は追う先を考え直さない（相手の方を向いて寄るだけ）。
	// 攻撃に行っている体は仲間を避けない（つかみに行く動きを崩さない。ほかの体が、この体から離れる）。
	mLocomotion.beginTick();
	if ( mState == State::Alive && !mStagger.isActive() && canChase() ) {
		if ( mAttack.isActive() ) {
			approachDuringAttack( target );
		} else {
			chase( target, hitWorld, crowd );
		}
	} else if ( mState == State::Downed ) {
		updateDowned( target, hitWorld );
	}
	if ( isAlive() ) {
		updateAttack( target );
	}
	// 足元を地面（歩道の段差）の高さへ合わせる。歩いていない間（立っている・よろけている）も、歩道の上で沈まないよう毎ティック行う。
	// 倒れた後は動かないので、倒れた場所の高さのまま（沈む演出は見た目の位置だけを下げる）。
	// 根の動きの区間の間は、置き場所を動かした後に合わせる（ZombieRootMotionSpan::apply）。
	if ( mState == State::Alive && !mRootSpan.isActive() ) {
		mPosition.y = mMover.followGround( mPosition );
		mMesh.setPosition( mPosition );
	}
	mSway.update( swayStrength( target ), zombieUncertaintyParams().swayMaxDegree ); // 姿勢を求める前に、このティックの揺れを決める
	mMesh.update();
	mRootSpan.apply( target ? target->body : nullptr, &mPosition );
	if ( mState == State::Alive ) {
		updateStagger();
	} else if ( mState != State::Downed ) { // Downed は倒れるアニメの最後の姿勢のまま（動かすのは腕と置き場所だけ）
		updateAfterDeath();
	}
	mFlinch.update();
	// 体を引きずり始めた後は（死んだ後も）、起こした頭・動かした腕に当たるよう、見えている姿勢に箱を付ける
	// （頭を 0.2m ほど持ち上げるので、アニメどおりの姿勢のままだと頭を狙っても当たらない）。
	mHitboxes.update( mMesh, mCrawler.isStarted() ? HitboxSet::POSE_SHOWN : HitboxSet::POSE_ANIMATED );
}

void Zombie::chase( const ChaseTarget* target, const HitWorld& hitWorld, const ZombieCrowd* crowd ) {
	// 考える（見て・行き先を決め・向きを変える）→ 攻撃を始めるか → 動く、の順（SummaryPlan/個別仕様/ZombieSplit2_Plan.txt の 3 章）。
	Vector3 eye = mPosition;
	eye.y += kEyeHeight;
	float weaveDegree = mSway.weaveDegree( zombieUncertaintyParams().weaveDegree ); // 揺れている間は、倒れている側へ少し流れる
	ZombieLocomotion::Intent intent = mLocomotion.thinkStanding( mPosition, eye, target, hitWorld, crowd, weaveDegree, &mAngleY );
	if ( canStartAttack( target ) ) {
		mLocomotion.clearPath();
		startAttack();
		mMesh.setAngleY( mAngleY );
		return;
	}
	// 向きを変えている最中も、歩きたいなら歩くアニメにする（その場で向きを変える動きに見せる）。
	mAnimations.setWalking( intent.walking );
	if ( intent.walking && intent.facing ) {
		mPosition += mLocomotion.walkStep( mAnimations.walkDistanceThisTick(), mAngleY );
	} else if ( !intent.walking && mLocomotion.mode() == ZombieChase::Mode::Chase ) {
		// 追いついて立ち止まっている。仲間の後ろに重なったまま止まり続けないように
		mLocomotion.standAside( bodyCenter(), crowd, &mPosition );
	}
	mMesh.setPosition( mPosition );
	mMesh.setAngleY( mAngleY );
}

float Zombie::swayStrength( const ChaseTarget* target ) const {
	// 遠くでは揺らさない（遠くから頭を狙う楽しさは残す）。音で探しに来る・うろつく間も揺らさない。
	if ( !target || mState != State::Alive || mStagger.isActive() || mAttack.isActive() || !canChase() ||
		mLocomotion.mode() != ZombieChase::Mode::Chase ) {
		return 0.f;
	}
	const ZombieUncertaintyParams& params = zombieUncertaintyParams();
	return ZombieSway::strengthAt( flatDistanceTo( target->position ), params.swayStartDistance, params.swayFullDistance );
}

bool Zombie::canStartAttack( const ChaseTarget* target ) const {
	if ( !mAnimations.hasAttack() || !target || !target->grabbable || target->attackTaken || !mAttack.canStart() ) {
		return false;
	}
	if ( mLocomotion.mode() != ZombieChase::Mode::Chase ) {
		return false; // 見えている相手だけを襲う（見失って探している間・うろついている間は襲わない）
	}
	if ( isCrawling() && !mCrawler.canPull() ) {
		return false; // 両腕を失った体はつかめない
	}
	return ZombieAttackReach::canStart( attackOrigin(), target->position );
}

void Zombie::startAttack() {
	mAttack.start();
	if ( isCrawling() ) {
		// 攻撃のアニメは流さない（倒れた姿勢のまま、腕を前へ伸ばす）。段階はアニメと同じ時間割の時計で進める（updateAttack）。
		return;
	}
	mAnimations.playAttack( &mPosition ); // 終わったら、まず立つ（次の update で追う先を考え直し、離れていれば歩き出す）
}

void Zombie::approachDuringAttack( const ChaseTarget* target ) {
	if ( !target ) {
		return;
	}
	mLocomotion.turnToward( bodyCenter(), target->position, posture(), &mAngleY );
	// 寄る距離は、踏み込む前の足元（根の動きのアンカー＝bodyCenter）から測る。寄るのもアンカーごと（踏み込みは別に足す）。
	if ( mAttack.phase() == ZombieAttack::Phase::Reach ) {
		float distance = ZombieAttackReach::approachStep( bodyCenter(), target->position );
		if ( distance > 0.f ) {
			mRootSpan.moveFooting( mLocomotion.walkStep( distance, mAngleY ), &mPosition ); // 相手の体・銃の箱に当たったら、そこで止まる
		}
	}
	mMesh.setPosition( mPosition );
	mMesh.setAngleY( mAngleY );
}

void Zombie::updateAttack( const ChaseTarget* target ) {
	bool wasActive = mAttack.isActive();
	bool crawling = isCrawling();
	if ( crawling ) {
		mAttack.updateWithoutAnimation( mAnimations.attackPeriod(), isTargetInReach( target ) );
	} else {
		mAttack.update( mMesh.animationTime(), mMesh.isAnimationFinished(), isTargetInReach( target ) );
	}
	if ( mAttack.bitThisTick() ) {
		// 噛むアニメが無いので、首と背骨を突き出して顔を近づける。
		// 立っている体は前へ、体を引きずっている体は上へ（頭を持ち上げて脚へ噛みつく）。
		mFlinch.lunge( crawling ? kCrawlBiteLungeDirection : kBiteLungeDirection );
	}
	if ( crawling ) {
		return; // 腕を伸ばす・戻すのは ZombieCrawl（updateDowned）。アニメは替えない
	}
	if ( mAttack.isActive() ) {
		mMesh.setPlaybackRate( mAttack.playbackRate() );
	} else if ( wasActive ) {
		mAnimations.returnToLocomotion( &mPosition ); // 攻撃のアニメの最後（元の立ち姿）から、立つアニメへ戻す
	}
}

bool Zombie::isTargetInReach( const ChaseTarget* target ) const {
	if ( !target || !target->grabbable ) {
		return false;
	}
	return ZombieAttackReach::canGrab( attackOrigin(), target->position );
}

ZombieAttackReach::Origin Zombie::attackOrigin() const {
	ZombieAttackReach::Origin origin;
	origin.center = bodyCenter();
	origin.crawling = isCrawling();
	// 寝ている体は足元から前へ 1.5m ほど伸びているので、手の届く距離は頭から測る。
	origin.reachFrom = origin.crawling ? headPosition() : origin.center;
	origin.angleY = mAngleY;
	return origin;
}

float Zombie::flatDistanceTo( const Vector3& target ) const {
	return FlatMath::distance( bodyCenter(), target );
}

bool Zombie::canChase() const {
	return mAnimations.canWalk();
}

void Zombie::draw() const {
	mMesh.draw();
}

void Zombie::applyDamage( const DamageInfo& damage ) {
	mDismemberment.clearSeveredThisHit();
	// 倒れ始めた後に当たった弾は、死体で止まるだけ（体力も反応も変えない）。
	if ( !isAlive() ) {
		return;
	}
	// 振りほどきには、部位の倍率を掛けた後の量を数える（全体の体力が減った分）。
	float totalBefore = mHealth.total();
	mHealth.takeDamage( damage.part, damage.baseAmount, mRandom.getFloat() ); // 頭は確率で弾ける（クリティカル）
	float dealt = totalBefore - mHealth.total();
	mLocomotion.alert(); // 撃たれたら、見えていなくても気付く
	// のけぞりは骨格の根の空間で曲げるので、弾の向きを置き場所の向きの分だけ戻す。
	mFlinch.push( damage.part, mMesh.toRootDirection( damage.direction ) );
	// 倒れる 1 発で部位も 0 になったら、切れてから倒れる（頭を撃って倒したときは、頭が飛ぶ）。
	severIfBroken( damage.part );
	// どう反応するかを選び（ZombieDamageReaction）、ここで動かす。
	// isAttacking は振りほどきを数える前に読むが、数えて値が変わるのは振りほどかれたとき（よろけを選ばない）だけなので同じ答えになる。
	ZombieDamageReaction::Facts facts;
	facts.canFall = mAnimations.hasDeath();
	facts.isDead = mHealth.isDead();
	facts.isStanding = ( mState == State::Alive );
	facts.hasLostFooting = mDismemberment.hasLostFooting();
	facts.isAttacking = mAttack.isActive();
	facts.canStagger = canStagger( damage.part );
	switch ( ZombieDamageReaction::chooseFall( facts ) ) {
		case ZombieDamageReaction::Fall::Die: startFalling(); return;
		case ZombieDamageReaction::Fall::Down: startDowned(); return;
		case ZombieDamageReaction::Fall::None: break;
	}
	bool shakenOff = mAttack.takeShakeDamage( dealt ); // 倒れる 1 発では数えない
	switch ( ZombieDamageReaction::chooseHit( facts, shakenOff ) ) {
		case ZombieDamageReaction::Hit::ShakeOff: shakeOff(); break;
		case ZombieDamageReaction::Hit::Stagger: startStagger(); break;
		case ZombieDamageReaction::Hit::None: break;
	}
}

void Zombie::shakeOff() {
	// 離すときは、よろけて手を放す（よろけのアニメが無ければ、立つアニメへ戻すだけ）。よろけの間あけは待たない。
	if ( mAnimations.hasStagger() ) {
		startStagger();
	} else {
		mAnimations.returnToLocomotion( &mPosition );
	}
}

void Zombie::severIfBroken( BodyPart part ) {
	if ( !mHealth.isBroken( part ) || !mDismemberment.sever( part ) ) {
		return; // まだ残っている・切れない部位（胴）・もう無い
	}
	for ( BodyPart lostPart : mModel->severLayout().cut( part ).parts ) {
		mHitboxes.removePart( lostPart );
	}
}

void Zombie::startFalling() {
	// 撃たれた瞬間に見えていた姿勢から繋ぐ（歩き・よろけのどの瞬間に倒れても、姿勢が飛ばない）。
	// Downed は同じ死亡アニメで倒れている途中・倒れ終わりなので、繋ぎ直さずそのまま続ける（倒れ終わっていれば次の update で Corpse）。
	// 根の動きも、立っていたときだけ始め直す（Downed は倒れる区間をそのまま続ける。引きずり始めた後は区間が無い）。
	if ( mState == State::Alive ) {
		mAnimations.playDeath( &mPosition );
	}
	mState = State::Falling;
	mAttack.cancel(); // つかんでいたら離す
	mStagger.cancel();
}

void Zombie::startDowned() {
	// 前へ倒れるアニメ（無ければ死亡アニメ）で倒す。倒れ終わったら、その姿勢のまま腕で体を引きずる（ZombieCrawl_Plan.txt の 2 章）。
	mAnimations.playDowned( &mPosition );
	mState = State::Downed;
	mAttack.cancel();
	mStagger.cancel();
}

void Zombie::updateDowned( const ChaseTarget* target, const HitWorld& hitWorld ) {
	if ( !mCrawler.isStarted() ) {
		if ( !canStartCrawling() ) {
			return; // まだ倒れている途中（または引きずれない体）。倒れた場所の姿勢のまま
		}
		mRootSpan.end( &mPosition ); // ここから先は胸の位置で動かす（足元は絵の根から測り直す）
		startCrawling();
	}
	ZombieCrawler::Effort effort = ZombieCrawler::Effort::Still;
	bool moving = false;
	if ( mAttack.isActive() ) {
		if ( target ) {
			mLocomotion.turnToward( bodyCenter(), target->position, posture(), &mAngleY ); // つかみに行く・つかんでいる間は、相手の方を向くだけ（寄らない）
		}
	} else {
		effort = crawlChase( target, hitWorld, &moving );
	}
	// 腕で引いた分だけ進む。壁・車に当たったら胸の位置で沿って滑る。
	float distance = mCrawler.moveArms( effort, mAttack.isAttacking(), moving );
	if ( distance > 0.f ) {
		mCrawler.moveCenter( mLocomotion.walkStep( distance, mAngleY ) );
	}
	mCrawler.place( mAngleY, mMover, &mMesh, &mPosition ); // 胸の位置と進む向きから、見た目の向き・足元を決め直す
	// 見えている相手（追っている・つかみに行っている間）へ顔を向ける。置き場所を決めた後の骨格の根の空間で渡す。
	bool looking = target && ( mLocomotion.mode() == ZombieChase::Mode::Chase || mAttack.isActive() );
	Vector3 lookPoint( 0.f, 0.f, 0.f );
	if ( looking ) {
		lookPoint = mMesh.toRootPoint( target->eyePosition );
	}
	mCrawler.look( looking, lookPoint );
}

bool Zombie::canStartCrawling() const {
	if ( !mAnimations.hasDowned() || !canChase() || !mModel->crawlLayout().isValid() ) {
		return false; // 前へ倒れるアニメが無い（仰向けかもしれない）・確認用の体・腕の骨が無い
	}
	return mAnimations.hasFinishedDownedFall();
}

void Zombie::startCrawling() {
	if ( !mCrawler.start( mMesh, mPosition, &mAngleY ) ) {
		return; // まだ update していない（倒れ終わってから呼ぶので起きない）
	}
	mLocomotion.clearPath();
}

ZombieCrawler::Effort Zombie::crawlChase( const ChaseTarget* target, const HitWorld& hitWorld, bool* moving ) {
	*moving = false;
	if ( !mCrawler.canPull() ) {
		return ZombieCrawler::Effort::Still; // 両腕を失った。倒れたまま動かない
	}
	ZombieLocomotion::Intent intent = mLocomotion.thinkCrawling( mCrawler.center(), mCrawler.eye(), target, hitWorld, &mAngleY );
	if ( intent.wandering ) {
		return ZombieCrawler::Effort::Struggle; // 気付いていない間はうろつかず、その場でもがくだけ
	}
	if ( canStartAttack( target ) ) {
		mLocomotion.clearPath();
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

ZombieLocomotion::Posture Zombie::posture() const {
	return isCrawling() ? ZombieLocomotion::Posture::Crawling : ZombieLocomotion::Posture::Standing;
}

bool Zombie::isCrawling() const {
	return mState == State::Downed && mCrawler.isStarted();
}

Vector3 Zombie::bodyCenter() const {
	if ( isCrawling() ) {
		return mCrawler.center();
	}
	return mRootSpan.isActive() ? mRootSpan.anchor() : mPosition;
}

const std::vector< BodyPart >& Zombie::severedByLastDamage() const {
	return mDismemberment.severedThisHit();
}

void Zombie::shownWorldPose( std::vector< Matrix34 >* out ) const {
	mMesh.shownWorldPose( out );
}

bool Zombie::canStagger( BodyPart part ) const {
	return mAnimations.hasStagger() && mStagger.canStart( part );
}

void Zombie::startStagger() {
	mAnimations.playStagger( &mPosition );
	mStagger.start();
}

void Zombie::updateStagger() {
	if ( !mStagger.isActive() ) {
		mStagger.tick();
		return;
	}
	if ( mMesh.isAnimationFinished() ) {
		// よろける前に歩いていたか立っていたかに戻す（次の update で追う先を考え直し、要れば切り替える）。
		mAnimations.returnToLocomotion( &mPosition ); // 腰は下がった後に元の所へ戻っている
		mStagger.finish();
	}
}

void Zombie::updateAfterDeath() {
	switch ( mState ) {
		case State::Falling:
			if ( mMesh.isAnimationFinished() ) {
				mState = State::Corpse;
				mCorpse.start();
				mRootSpan.freeze(); // 置き場所は死体の腰の下・絵の根はそのまま（もう動かない）
			}
			break;
		case State::Corpse:
			mCorpse.tick();
			if ( mCorpse.isSinking() ) {
				mState = State::Sinking; // 沈めるのは次のティックから
			}
			break;
		case State::Sinking: {
			// 見た目の位置だけ下げる（部位の箱も見た目に付いて沈む）。沈み終わったら isGone が true になり、持ち主が消す。
			mCorpse.tick();
			Vector3 shown = mRootSpan.meshRoot( mPosition );
			shown.y -= mCorpse.sinkDepth();
			mMesh.setPosition( shown );
			break;
		}
		case State::Alive:
		case State::Downed:
			break;
	}
}

Zombie::State Zombie::state() const {
	return mState;
}

bool Zombie::isAlive() const {
	return mState == State::Alive || mState == State::Downed;
}

void Zombie::hearNoise( const Vector3& position ) {
	bool listening = ( mState == State::Alive && !mStagger.isActive() ) || isCrawling();
	if ( !listening || mAttack.isActive() || !canChase() ) {
		return; // 倒れている途中・死んだ・よろけている・攻撃している体と、追ってこない確認用の体は聞かない
	}
	mLocomotion.hearNoise( position );
}

const ZombiePath& Zombie::path() const {
	return mLocomotion.path();
}

const Vector3& Zombie::separation() const {
	return mLocomotion.separation();
}

ZombieChase::Mode Zombie::chaseMode() const {
	return mLocomotion.mode();
}

bool Zombie::isStaggering() const {
	return mStagger.isActive();
}

const ZombieHealth& Zombie::health() const {
	return mHealth;
}

bool Zombie::blocksMovement() const {
	return mState == State::Alive;
}

bool Zombie::isGone() const {
	return mState == State::Sinking && mCorpse.isGone();
}

float Zombie::secondsUntilGone() const {
	switch ( mState ) {
		case State::Corpse:
		case State::Sinking: return mCorpse.secondsUntilGone();
		case State::Alive:
		case State::Downed:
		case State::Falling: break;
	}
	return 0.f;
}

BoxCollider Zombie::collider() const {
	// 体を引きずっている間は、胸の位置に置く（壁に当たるのは胸。頭・足は少しめり込むことがある）。
	return mModel->footprint().place( bodyCenter(), mAngleY );
}

const HitboxSet& Zombie::hitboxes() const {
	return mHitboxes;
}

void Zombie::addTo( CollisionWorld* collisionWorld, HitWorld* hitWorld ) {
	collisionWorld->addBody( this );
	hitWorld->addTarget( this );
}

void Zombie::removeFrom( CollisionWorld* collisionWorld, HitWorld* hitWorld ) {
	collisionWorld->removeBody( this );
	hitWorld->removeTarget( this );
}

ZombieAttack::Phase Zombie::attackPhase() const {
	return mAttack.phase();
}

// 攻撃は立っている体と、体を引きずっている体だけがする（倒れ始めたら ZombieAttack::cancel 済み）。
bool Zombie::isAttacking() const {
	return isAlive() && mAttack.isAttacking();
}

bool Zombie::isHoldingTarget() const {
	return isAlive() && mAttack.isHolding();
}

bool Zombie::grabbedThisTick() const {
	return isAlive() && mAttack.grabbedThisTick();
}

bool Zombie::bitThisTick() const {
	return isAlive() && mAttack.bitThisTick();
}

Vector3 Zombie::headPosition() const {
	Vector3 head;
	if ( mHitboxes.partCenter( BodyPart::Head, &head ) ) {
		return head;
	}
	Vector3 eye = mPosition;
	eye.y += kEyeHeight;
	return eye;
}

const Vector3& Zombie::position() const {
	return mPosition;
}
