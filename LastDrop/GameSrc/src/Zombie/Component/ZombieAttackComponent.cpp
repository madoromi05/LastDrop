#include "GameSrc/Zombie/Component/ZombieAttackComponent.h"
#include "GameSrc/Zombie/Component/ZombieStateComponent.h"
#include "GameSrc/Zombie/Component/ZombieChaseComponent.h"
#include "GameSrc/Zombie/Component/ZombieDismemberComponent.h"
#include "GameSrc/Zombie/Component/ZombieRootMotionComponent.h"
#include "GameSrc/Zombie/Component/ZombieHealthComponent.h"
#include "GameSrc/Zombie/Move/ChaseTarget.h"
#include "GameSrc/Component/TransformComponent.h"
#include "GameSrc/Component/SkinnedModelComponent.h"
#include "GameSrc/World/GameObject.h"
#include "GameSrc/Zombie/Attack/ZombieAttackParams.h"
#include "GameSrc/Core/GameTime.h"
#include "GameSrc/Math/AngleMath.h"

#include "GameLib/Math/Functions.h"

using namespace GameLib;
using GameLib::Math::Vector3;

namespace{
	const Vector3 kBiteLungeDirection( 0.f, 0.f, 1.f );	// 噛むときに首・背骨を突き出す向き（骨格の根の空間で前。押し方は ZombieFlinch::lunge）。噛むアニメが無いので顔を相手へ近づける
	const int kAlignedTicks = 1;						// 寄せ終えた後の残りティック。1 なら毎ティック、ずれた分をそのまま詰める（根の動きの小さな揺れで手が離れないように）

	ZombieAttackParams gParams;
}

ZombieAttackParams& zombieAttackParams() {
	return gParams;
}

ZombieAttackComponent::ZombieAttackComponent( ZombieAttackStyle style ) :
	mStyle( style ),
	mPosition( nullptr ),
	mAngleY( nullptr ),
	mMesh( nullptr ),
	mState( nullptr ),
	mLocomotion( nullptr ),
	mCrawler( nullptr ),
	mRootSpan( nullptr ),
	mAnimations( nullptr ),
	mAlignTicks( kAlignedTicks ) {
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
	if ( mState->isCrawling() ) {
		// 攻撃のアニメは流さない（倒れた姿勢のまま、腕を前へ伸ばす）。段階は自分の時計で進める（update）
		mAttack.startSwing();
		return true;
	}
	if ( mStyle == ZombieAttackStyle::Swing ) {
		mAttack.startSwing();
	} else {
		mAttack.start();
	}
	mAnimations->playAttack( mPosition ); // 終わったら、まず立つ（次の update で追う先を考え直し、離れていれば歩き出す）
	return true;
}

void ZombieAttackComponent::approach( const ChaseTarget* target ) {
	if ( !target ) {
		return;
	}
	if ( mAttack.phase() == ZombieAttack::Phase::Hold ) {
		alignToHeldTarget( target->position );
		mMesh->setPosition( *mPosition );
		mMesh->setAngleY( *mAngleY );
		return;
	}
	mLocomotion->turnToward( mState->bodyCenter(), target->position, mState->posture(), mAngleY );
	// 寄る距離は、踏み込む前の足元（根の動きのアンカー＝bodyCenter）から測る。寄るのもアンカーごと（踏み込みは別に足す）
	// 腕振りも、当たる時刻までは寄る（手を伸ばす間のつかみと同じ。届く所まで来たら approachStep が 0 を返す）
	ZombieAttack::Phase phase = mAttack.phase();
	if ( phase == ZombieAttack::Phase::Reach || phase == ZombieAttack::Phase::Swing ) {
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
		mAttack.updateWithoutAnimation( isTargetInReach( target ) );
		return; // 腕を伸ばす・戻すのは ZombieCrawler（ZombieCrawlComponent）
	}
	mAttack.update( mMesh->animationTime(), mMesh->isAnimationFinished(), isTargetInReach( target ) );
	if ( mAttack.grabbedThisTick() ) {
		// 次の approach から、決まった距離・正面へ寄せ始める（つかんだ時の間合いは 0.9〜1.6m・左右 45° とばらつくので）
		int alignTicks = GameTime::secondsToTicks( gParams.grabAlignSeconds );
		mAlignTicks = ( alignTicks > kAlignedTicks ) ? alignTicks : kAlignedTicks;
	}
	// 首を突き出すのは噛んだときだけ（腕振りは腕のアニメのまま当てる）
	if ( mAttack.hitThisTick() && mStyle == ZombieAttackStyle::Grab ) {
		// のけぞりの部品は外せるので、使うたびに引く（外されていれば首を突き出さずに噛む）
		if ( ZombieHealthComponent* health = getComponent< ZombieHealthComponent >() ) {
			health->flinch().lunge( kBiteLungeDirection );
		}
	}
	if ( mAttack.isActive() ) {
		mMesh->setPlaybackRate( mAttack.playbackRate() ); // つかんでいる間だけゆっくり（アニメを替えれば速さは 1 へ戻る）
	} else if ( wasActive ) {
		mAnimations->returnToLocomotion( mPosition ); // 攻撃のアニメの最後（元の立ち姿）から、立つアニメへ戻す
	}
}

void ZombieAttackComponent::cancel() {
	mAttack.cancel();
}

bool ZombieAttackComponent::isActive() const {
	return mAttack.isActive();
}

// 攻撃は立っている体と、体を引きずっている体だけがする（倒れ始めたら cancel 済み）
bool ZombieAttackComponent::isAttacking() const {
	return mState->isAlive() && mAttack.isAttacking();
}

bool ZombieAttackComponent::isHoldingTarget() const {
	return mState->isAlive() && mAttack.isHolding();
}

bool ZombieAttackComponent::grabbedTargetThisTick() const {
	return mState->isAlive() && mAttack.grabbedThisTick();
}

bool ZombieAttackComponent::hitTargetThisTick() const {
	return mState->isAlive() && mAttack.hitThisTick();
}

// 腰（足元）を相手から grabDistance の所へ、向きを相手の正面へ、残りのティックで割った分だけ寄せる
// アンカーごと動かすので、アニメの踏み込み（根の動き）を足した後の腰が、その所に来る
// 相手との間はゾンビが寄って来た所なので、当たり判定はしない（相手はつかまれて動けない）
void ZombieAttackComponent::alignToHeldTarget( const Vector3& target ) {
	float fraction = 1.f / static_cast< float >( mAlignTicks );
	if ( mAlignTicks > kAlignedTicks ) {
		--mAlignTicks;
	}
	// 向きは根の動きのアンカー（bodyCenter）から測る（turnToward と同じ）。腰はアンカーから前へ出ているので、これで腰も相手の正面へ来る
	Vector3 toTarget;
	toTarget.setSub( target, mState->bodyCenter() );
	toTarget.y = 0.f;
	if ( toTarget.squareLength() > 0.f ) {
		float difference = AngleMath::difference( AngleMath::headingOf( toTarget ), *mAngleY );
		*mAngleY = AngleMath::wrapSigned( *mAngleY + difference * fraction );
	}
	// 距離は、相手から腰への向きのまま縮める・離す。重なっていたら、ゾンビの向きの後ろへ離す
	Vector3 fromTarget;
	fromTarget.setSub( *mPosition, target );
	fromTarget.y = 0.f;
	float length = fromTarget.length();
	if ( length <= 0.f ) {
		fromTarget.set( -Math::sin( *mAngleY ), 0.f, -Math::cos( *mAngleY ) );
		length = 1.f;
	}
	Vector3 wanted;
	wanted.setMul( fromTarget, gParams.grabDistance / length );
	wanted += target;
	wanted -= *mPosition;
	wanted.y = 0.f;
	Vector3 step;
	step.setMul( wanted, fraction );
	mRootSpan->moveFooting( step, mPosition );
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
	// 寝ている体は足元から前へ 1.5m ほど伸びているので、手の届く距離は頭から測る
	origin.reachFrom = origin.crawling ? mState->headPosition() : origin.center;
	origin.angleY = *mAngleY;
	return origin;
}
