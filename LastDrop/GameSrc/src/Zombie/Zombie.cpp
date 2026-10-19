#include "GameSrc/Zombie/Zombie.h"
#include "GameSrc/Zombie/ZombieModel.h"
#include "GameSrc/Render/SkinnedMesh/SkinnedMeshData.h"
#include "GameSrc/Collision/CollisionWorld.h"
#include "GameSrc/Hit/HitWorld.h"
#include "GameSrc/Hit/DamageInfo.h"
#include "GameSrc/Core/GameTime.h"

#include "GameLib/GameLib.h"
#include "GameLib/Base/DebugStream.h"
#include "GameLib/Math/Matrix34.h"
#include "GameLib/Math/Functions.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// 撃たれた瞬間の姿勢から死亡アニメへ繋ぐ長さ（ティック）。短いほど撃たれた手応えが強いが、0 だと姿勢が飛ぶ。
	const float kDeathFadeTicks = 6.f;
	const float kDeathStartTime = 0.f; // 死亡アニメは頭から再生する
	// 倒れ終わってから沈み始めるまで（秒）。
	const float kCorpseSeconds = 10.f;
	// 沈むのにかける時間（秒）と深さ（m）。体の厚み（倒れた胴の高さ）より深くして、沈み終わりに何も見えないようにする。
	const float kSinkSeconds = 1.5f;
	const float kSinkDepth = 0.5f;

	// 撃たれた瞬間の姿勢からよろけのアニメへ繋ぐ長さ（ティック）。死亡アニメと同じく短くして、手応えを早く出す。
	const float kStaggerFadeTicks = 6.f;
	// よろけのアニメ（gethit）を再生し始める時刻（フレーム）。gethit には撃たれた反応が 2 回続けて入っていて
	// （0〜30 フレームで 1 回目、30 フレームで立ち姿に戻り、30〜59 フレームで 2 回目）、頭から流すと
	// 「反応 → 立ち姿 → もう一度よろける」と 2 回に見える。左脚と両腕が大きく動く 2 回目だけを使う（2026-10-18 ユーザー指定）。
	// 終わりはアニメの最後（59 フレーム）なので、PLAY_ONCE で止まったら歩きへ戻す。
	const float kStaggerStartTime = 30.f;
	// よろけ終わりから歩きへ戻す長さ（ティック）。よろけの最後と歩きの始めの姿勢は大きく違う（最大 46°）ので、少し長めに混ぜる。
	const float kRecoverFadeTicks = 10.f;
	const float kMoveRestartTime = 0.f; // 歩きは頭から再生し直す
	// よろけが終わってから、次のよろけを出さない時間（秒）。頭に当たったときは待たない。
	const float kStaggerCooldownSeconds = 1.f;

	// ── 追ってくるときの歩き方（SummaryPlan/個別仕様/ZombieChase_Plan.txt の 3 章）──
	// 歩く速さ（m/ティック）。ふだんは歩きアニメの足の動きから求めた表（ZombieStride）で進み、これは表が作れなかったときだけ使う。
	// 接地している足先が 1 周で左 0.45m・右 0.57m 後ろへ動くのを Blender で測った平均（1 周 1 秒で約 1.0m）。
	const float kFallbackWalkSpeed = 1.0f / GameTime::kTicksPerSecond;
	// 向きを変える速さ（度/ティック）。1 秒に 180°（急に振り向かない）。
	const float kTurnPerTick = 180.f / GameTime::kTicksPerSecond;
	// 行き先との向きの差がこれより大きい間は、その場で向きを変えるだけで進まない（横歩き・後ろ歩きに見えないように）。
	const float kTurnInPlaceDegree = 60.f;
	// 行き先がこれより近ければ、向きを決めない（真上に居るときに向きが定まらず回り続けないように。m）。
	const float kMinTurnDistance = 0.05f;
	// 目の高さ（m。足元から）。ここから自キャラの目が見えているかを調べる。身長 1.82m の目の辺り。
	const float kEyeHeight = 1.6f;
	// 歩く ⇔ 立つ のアニメを繋ぐ長さ（ティック）。
	const float kLocomotionFadeTicks = 10.f;
	const float kHalfTurn = 180.f;
	const float kFullTurn = 360.f;

	// degree を -180〜180 へ。
	float wrapSignedAngle( float degree ) {
		while ( degree > kHalfTurn ) {
			degree -= kFullTurn;
		}
		while ( degree < -kHalfTurn ) {
			degree += kFullTurn;
		}
		return degree;
	}

	int secondsToTicks( float seconds ) {
		return static_cast< int >( seconds * GameTime::kTicksPerSecond );
	}
	const int kCorpseTicks = secondsToTicks( kCorpseSeconds );
	const int kSinkTicks = secondsToTicks( kSinkSeconds );
	const int kStaggerCooldownTicks = secondsToTicks( kStaggerCooldownSeconds );

	float ticksToSeconds( int ticks ) {
		return static_cast< float >( ticks ) / GameTime::kTicksPerSecond;
	}

	// name のアニメを引く。name が nullptr なら空（使わない）。名前が無ければ、静かに使えなくなるので知らせる。
	Scene::Animation findAnimation( const SkinnedMeshData* data, const char* name ) {
		if ( !name ) {
			return Scene::Animation();
		}
		Scene::Animation animation = data->animation( name );
		if ( !animation ) {
			GameLib::cout << "Zombie: animation not found: " << name << GameLib::endl;
		}
		return animation;
	}
}

Zombie::Zombie( const ZombieModel* model, const CollisionWorld* collisionWorld, const Vector3& position, float angleY,
const Animations& animations, int sightPhase ) :
	mModel( model ),
	mMoveAnimation( findAnimation( model->data(), animations.move ) ),
	mIdleAnimation( findAnimation( model->data(), animations.idle ) ),
	mDeathAnimation( findAnimation( model->data(), animations.death ) ),
	mStaggerAnimation( findAnimation( model->data(), animations.stagger ) ),
	mFlinch( &model->flinchLayout() ),
	mDismemberment( &model->severLayout() ),
	mMesh( model->data() ),
	mHitboxes( &model->hitboxLayout() ),
	mStaggering( false ),
	mStaggerCooldownTicks( 0 ),
	mPosition( position ),
	mAngleY( wrapSignedAngle( angleY ) ),
	mChase( sightPhase ),
	mMover( collisionWorld, this ), // this は覚えるだけ（当たり判定を読むのは move のとき＝構築の後）
	mWalking( false ),
	mMoveStartTime( animations.moveStartTime ),
	mState( State::Alive ),
	mStateTicks( 0 ) {
	mMesh.setPosition( mPosition );
	mMesh.setAngleY( mAngleY );
	mPoseModifiers.add( &mFlinch );
	mPoseModifiers.add( &mDismemberment );
	mMesh.setPoseModifier( &mPoseModifiers );
	// 気付く前は立っている（立つアニメが無ければ歩くアニメ。どちらも空ならレスト姿勢＝T ポーズ）。
	mMesh.setAnimation( locomotionAnimation(), mMoveStartTime );
}

void Zombie::update( const ChaseTarget* target, const HitWorld& hitWorld ) {
	// 動いた後の位置・向きでアニメと部位の箱を置くので、先に動く。
	if ( mState == State::Alive && !mStaggering && canChase() ) {
		chase( target, hitWorld );
	}
	// 足元を地面（歩道の段差）の高さへ合わせる。歩いていない間（立っている・よろけている）も、歩道の上で沈まないよう毎ティック行う。
	// 倒れた後は動かないので、倒れた場所の高さのまま（沈む演出は見た目の位置だけを下げる）。
	if ( mState == State::Alive ) {
		mPosition.y = mMover.followGround( mPosition );
		mMesh.setPosition( mPosition );
	}
	mMesh.update();
	if ( mState == State::Alive ) {
		updateStagger();
	} else if ( mState != State::Downed ) { // Downed は死亡アニメの最後の姿勢で止まっているだけ
		updateAfterDeath();
	}
	mFlinch.update();
	mHitboxes.update( mMesh );
}

void Zombie::chase( const ChaseTarget* target, const HitWorld& hitWorld ) {
	Vector3 eye = mPosition;
	eye.y += kEyeHeight;
	mChase.update( mPosition, eye, target, hitWorld );
	bool facing = mChase.hasDestination() ? turnToward( mChase.destination() ) : false;
	// 向きを変えている最中も、歩きたいなら歩くアニメにする（その場で向きを変える動きに見せる）。
	setWalking( mChase.wantsToWalk() );
	if ( mChase.wantsToWalk() && facing ) {
		float distance = walkDistanceThisTick();
		Vector3 step( Math::sin( mAngleY ) * distance, 0.f, Math::cos( mAngleY ) * distance );
		// 壁・車・他のキャラに当たったら沿って滑る（向きを変えてめり込んだ分も、ここで外へ出る）。
		mPosition += mMover.move( step );
	}
	mMesh.setPosition( mPosition );
	mMesh.setAngleY( mAngleY );
}

float Zombie::walkDistanceThisTick() const {
	const ZombieStride& stride = mModel->stride();
	if ( !stride.isValid() ) {
		return kFallbackWalkSpeed;
	}
	// 今見えている歩きのフレームから、次の update で見せるフレームまでに、地面についた足が後ろへ動く長さだけ進む
	// （足が地面を滑らない。歩き方がびっこを引いているので、速さは1歩ごとに波打つ）。
	return stride.distanceAt( mMesh.animationPhase() * mMoveAnimation.period() );
}

bool Zombie::turnToward( const Vector3& destination ) {
	Vector3 toDestination;
	toDestination.setSub( destination, mPosition );
	toDestination.y = 0.f;
	if ( toDestination.length() < kMinTurnDistance ) {
		return true;
	}
	// 向き 0 が +Z、90 が +X（PlayerCharacter と同じ）。
	float difference = wrapSignedAngle( Math::atan2( toDestination.x, toDestination.z ) - mAngleY );
	float turn = Math::max( -kTurnPerTick, Math::min( kTurnPerTick, difference ) );
	mAngleY = wrapSignedAngle( mAngleY + turn );
	return Math::abs( difference - turn ) <= kTurnInPlaceDegree;
}

void Zombie::setWalking( bool walking ) {
	if ( walking == mWalking ) {
		return;
	}
	mWalking = walking;
	// 立つアニメが無ければ、どちらも歩くアニメなので繋ぎ直さない。
	if ( mIdleAnimation ) {
		mMesh.crossFadeTo( locomotionAnimation(), kMoveRestartTime, kLocomotionFadeTicks );
	}
}

Scene::Animation Zombie::locomotionAnimation() const {
	return ( mWalking || !mIdleAnimation ) ? mMoveAnimation : mIdleAnimation;
}

bool Zombie::canChase() const {
	return static_cast< bool >( mMoveAnimation );
}

void Zombie::draw() const {
	mMesh.draw();
}

void Zombie::applyDamage( const DamageInfo& damage ) {
	mSeveredByLastDamage.clear();
	// 倒れ始めた後に当たった弾は、死体で止まるだけ（体力も反応も変えない）。
	if ( !isAlive() ) {
		return;
	}
	mHealth.takeDamage( damage.part, damage.baseAmount );
	mChase.alert(); // 撃たれたら、見えていなくても気付く
	// のけぞりは骨格の根の空間で曲げるので、弾の向きを置き場所の向きの分だけ戻す。
	Matrix34 worldToRoot;
	worldToRoot.setInverse( mMesh.rootMatrix() );
	Vector3 rootDirection;
	worldToRoot.mul33( &rootDirection, damage.direction );
	mFlinch.push( damage.part, rootDirection );
	// 倒れる 1 発で部位も 0 になったら、切れてから倒れる（頭を撃って倒したときは、頭が飛ぶ）。
	severIfBroken( damage.part );
	if ( mDeathAnimation && mHealth.isDead() ) {
		startFalling();
	} else if ( mDeathAnimation && mState == State::Alive && mDismemberment.hasLostFooting() ) {
		startDowned();
	} else if ( mState == State::Alive && canStagger( damage.part ) ) {
		startStagger();
	}
}

void Zombie::severIfBroken( BodyPart part ) {
	if ( !mHealth.isBroken( part ) || !mDismemberment.sever( part ) ) {
		return; // まだ残っている・切れない部位（胴）・もう無い
	}
	for ( BodyPart lostPart : mModel->severLayout().cut( part ).parts ) {
		mHitboxes.removePart( lostPart );
	}
	mSeveredByLastDamage.push_back( part );
}

void Zombie::startFalling() {
	// 撃たれた瞬間に見えていた姿勢から繋ぐ（歩き・よろけのどの瞬間に倒れても、姿勢が飛ばない）。
	// Downed は同じ死亡アニメで倒れている途中・倒れ終わりなので、繋ぎ直さずそのまま続ける（倒れ終わっていれば次の update で Corpse）。
	if ( mState == State::Alive ) {
		mMesh.crossFadeTo( mDeathAnimation, kDeathStartTime, kDeathFadeTicks, SkinnedMeshInstance::PLAY_ONCE );
	}
	mState = State::Falling;
	mStateTicks = 0;
	mStaggering = false;
}

void Zombie::startDowned() {
	// 這うアニメが無いので、死亡アニメで倒し、最後の姿勢で止めて代わりにする（ZombieDismember_Plan.txt の 5 章）。
	mMesh.crossFadeTo( mDeathAnimation, kDeathStartTime, kDeathFadeTicks, SkinnedMeshInstance::PLAY_ONCE );
	mState = State::Downed;
	mStaggering = false;
}

const std::vector< BodyPart >& Zombie::severedByLastDamage() const {
	return mSeveredByLastDamage;
}

void Zombie::shownWorldPose( std::vector< Matrix34 >* out ) const {
	ASSERT( out && "Zombie : out is null." );
	int boneNumber = mModel->data()->boneNumber();
	out->resize( boneNumber );
	for ( int b = 0; b < boneNumber; ++b ) {
		if ( !mMesh.boneMatrix( b, &( *out )[ b ] ) ) {
			( *out )[ b ] = mMesh.rootMatrix(); // まだ update していない（置いた直後）。根の位置に置く
		}
	}
}

bool Zombie::canStagger( BodyPart part ) const {
	if ( !mStaggerAnimation || mStaggering ) {
		return false; // よろけの最中は出し直さない（のけぞりだけ足す）
	}
	return mStaggerCooldownTicks <= 0 || part == BodyPart::Head;
}

void Zombie::startStagger() {
	mMesh.crossFadeTo( mStaggerAnimation, kStaggerStartTime, kStaggerFadeTicks, SkinnedMeshInstance::PLAY_ONCE );
	mStaggering = true;
}

void Zombie::updateStagger() {
	if ( mStaggering ) {
		if ( mMesh.isAnimationFinished() ) {
			// よろける前に歩いていたか立っていたかに戻す（次の update で追う先を考え直し、要れば切り替える）。
			mMesh.crossFadeTo( locomotionAnimation(), kMoveRestartTime, kRecoverFadeTicks );
			mStaggering = false;
			mStaggerCooldownTicks = kStaggerCooldownTicks;
		}
	} else if ( mStaggerCooldownTicks > 0 ) {
		--mStaggerCooldownTicks;
	}
}

void Zombie::updateAfterDeath() {
	++mStateTicks;
	switch ( mState ) {
		case State::Falling:
			if ( mMesh.isAnimationFinished() ) {
				mState = State::Corpse;
				mStateTicks = 0;
			}
			break;
		case State::Corpse:
			if ( mStateTicks >= kCorpseTicks ) {
				mState = State::Sinking;
				mStateTicks = 0;
			}
			break;
		case State::Sinking: {
			// 見た目の位置だけ下げる（部位の箱も見た目に付いて沈む）。沈み終わったら isGone が true になり、持ち主が消す。
			float progress = static_cast< float >( mStateTicks ) / static_cast< float >( kSinkTicks );
			if ( progress > 1.f ) {
				progress = 1.f;
			}
			Vector3 shown = mPosition;
			shown.y -= kSinkDepth * progress;
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

ZombieChase::Mode Zombie::chaseMode() const {
	return mChase.mode();
}

bool Zombie::isStaggering() const {
	return mStaggering;
}

const ZombieHealth& Zombie::health() const {
	return mHealth;
}

bool Zombie::blocksMovement() const {
	return mState == State::Alive;
}

bool Zombie::isGone() const {
	return mState == State::Sinking && mStateTicks >= kSinkTicks;
}

float Zombie::secondsUntilGone() const {
	switch ( mState ) {
		case State::Corpse:  return ticksToSeconds( kCorpseTicks - mStateTicks + kSinkTicks );
		case State::Sinking: return ticksToSeconds( kSinkTicks - mStateTicks );
		case State::Alive:
		case State::Downed:
		case State::Falling: break;
	}
	return 0.f;
}

BoxCollider Zombie::collider() const {
	return mModel->footprint().place( mPosition, mAngleY );
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
