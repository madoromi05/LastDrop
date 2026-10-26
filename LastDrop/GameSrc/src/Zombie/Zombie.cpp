#include "GameSrc/Zombie/Zombie.h"
#include "GameSrc/Zombie/ZombieModel.h"
#include "GameSrc/Zombie/ChaseTarget.h"
#include "GameSrc/Zombie/ZombieCrowd.h"
#include "GameSrc/Zombie/ZombieCrowdParams.h"
#include "GameSrc/Zombie/ZombieUncertaintyParams.h"
#include "GameSrc/Render/SkinnedMesh/SkinnedMeshData.h"
#include "GameSrc/Collision/CollisionWorld.h"
#include "GameSrc/Hit/HitWorld.h"
#include "GameSrc/Hit/DamageInfo.h"
#include "GameSrc/Core/GameTime.h"
#include "GameSrc/Math/AngleMath.h"
#include "GameSrc/Math/FlatMath.h"
#include "GameSrc/Math/ScalarMath.h"
#include "GameSrc/Math/PlacementMatrix.h"

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
	// ── 攻撃（SummaryPlan/個別仕様/ZombieAttack_Plan.txt の 3 章）──
	// 攻撃を始める間合い（m。足元どうし、水平だけ）。追いついて止まる距離（ZombieChase の kStopDistance = 1.2m）より遠くする。
	// 自キャラの移動の当たり判定は、銃を持っている間は銃の先まで前へ伸びるので、正面から来たゾンビは 1.2m より手前で
	// 銃の箱にぶつかって止まり、1.2m の内へ入れない（2026-10-21 ユーザー報告「攻撃してこない」）。
	// 止まった後に歩き出す距離（kResumeDistance = 1.5m）より少し遠くして、ぶつかって止まったゾンビも攻撃を始めるようにする。
	const float kAttackStartDistance = 1.6f;
	// 攻撃を始める向きの差（度。左右それぞれ）。相手の方を向き終わってから手を伸ばす。
	const float kAttackStartHalfAngle = 30.f;
	// 手を伸ばし切ったときに、つかめる距離（m）と向きの差（度）。伸ばしている間に下がられたら空振りにする。
	// 距離は始める間合いと同じ（銃の箱にぶつかって寄れなかったときも、手は銃・腕に届いている）。
	const float kGrabReachDistance = kAttackStartDistance;
	const float kGrabHalfAngle = 45.f;
	// 手を伸ばしている間に寄る距離（m）。手は足元から 0.8m 前へ出るので、止まる距離（1.2m）のままだと相手に届かない。
	const float kGrabCloseDistance = 0.9f;
	// 寄る速さ（m/ティック）。0.3m を 0.5 秒で（手を伸ばし切る 0.75 秒より前に寄り終える）。
	const float kReachApproachSpeed = 0.6f / GameTime::kTicksPerSecond;
	const float kAttackStartTime = 0.f;  // 攻撃のアニメは頭から再生する
	const float kAttackFadeTicks = 6.f;  // 立つ・歩くから攻撃のアニメへ繋ぐ長さ（ティック）
	// 噛むときに首・背骨を押す向き（骨格の根の空間で前）と、首を押す回数（のけぞり 1 回 20°なので 2 回で 40°。背骨は 1 回 12°）。
	const Vector3 kBiteLungeDirection( 0.f, 0.f, 1.f );
	const int kBiteHeadPushes = 2;
	// ── 脚を失って体を引きずる（SummaryPlan/個別仕様/ZombieCrawl_Plan.txt）──
	// 前へ倒れるアニメ（death1）が倒れ終わる時刻（フレーム）。ここから後は腰が動かない（Resource/character.txt の腰のキーで確かめた）。
	// これより短いアニメなら、最後まで来たら引きずり始める。
	const float kCrawlStartTime = 160.f;
	// 向きを変える速さ（度/ティック）。1 秒に 60°（立っている時の 1/3。寝た体は大きく回せない）。
	const float kCrawlTurnPerTick = 60.f / GameTime::kTicksPerSecond;
	// 目の高さ（m。胸から）。伏せているので低い。ここから自キャラの目が見えているかを調べる。
	const float kCrawlEyeHeight = 0.4f;
	// 腕の往復の速さ（ZombieCrawl::update の pace）。追っている間は 1、気付いていない・追いついて待っている間はゆっくりもがく。
	const float kCrawlPace = 1.f;
	const float kStrugglePace = 0.3f;
	const float kStillPace = 0.f; // つかみに行っている間・両腕を失った体は、往復を止める
	// 攻撃を始める間合い（m。頭から相手の足元まで、水平だけ）と、手を伸ばし切ったときにつかめる距離。
	// 追いついて止まる距離（胸から 1.2m）で、頭は相手の足元の手前 0.7m 辺りに来るので、それより少し遠くする。
	const float kCrawlAttackStartDistance = 0.9f;
	const float kCrawlGrabReachDistance = 1.0f;
	// 噛むときに首・背骨を押す向き（骨格の根の空間で上）。寝ている体は頭が前を向いているので、頭を持ち上げて脚へ噛みつく。
	const Vector3 kCrawlBiteLungeDirection( 0.f, 1.f, 0.f );

	const int kCorpseTicks = GameTime::secondsToTicks( kCorpseSeconds );
	const int kSinkTicks = GameTime::secondsToTicks( kSinkSeconds );
	const int kStaggerCooldownTicks = GameTime::secondsToTicks( kStaggerCooldownSeconds );

	// ── 体力の個体差（SummaryPlan/個別仕様/ZombieUncertainty_Plan.txt の 3 章）──
	// T ポーズの確認用の体の丈夫さ。表のままにして、部位ごとの減り方を確かめられるようにする。
	const float kFixedToughness = 1.f;
	// 丈夫さ・フラつきの値（F6 で動かせる）。
	ZombieUncertaintyParams gUncertaintyParams;

	// low〜high から一様に選ぶ。F6 で幅を逆にしても（high <= low）、low を使う。
	float pickInRange( float low, float high, Random* random ) {
		if ( high <= low ) {
			return low;
		}
		return random->getFloat( low, high );
	}

	// 体の丈夫さを選ぶ。T ポーズの確認用（歩くアニメが無い）は固定。
	float pickToughness( const Zombie::Animations& animations, Random* random ) {
		if ( !animations.move ) {
			return kFixedToughness;
		}
		return pickInRange( gUncertaintyParams.toughnessMin, gUncertaintyParams.toughnessMax, random );
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

ZombieUncertaintyParams& zombieUncertaintyParams() {
	return gUncertaintyParams;
}

Zombie::Zombie( const ZombieModel* model, const CollisionWorld* collisionWorld, const StreetGraph* streets,
const Vector3& position, float angleY, const Animations& animations, int sightPhase, int randomSeed ) :
	mModel( model ),
	mStreets( streets ),
	mMoveAnimation( findAnimation( model->data(), animations.move ) ),
	mIdleAnimation( findAnimation( model->data(), animations.idle ) ),
	mDeathAnimation( findAnimation( model->data(), animations.death ) ),
	mDownedAnimation( findAnimation( model->data(), animations.downed ) ),
	mStaggerAnimation( findAnimation( model->data(), animations.stagger ) ),
	mAttackAnimation( findAnimation( model->data(), animations.attack ) ),
	mRandom( Random::create( randomSeed ) ),
	mCrawl( &model->crawlLayout() ),
	mFlinch( &model->flinchLayout() ),
	mDismemberment( &model->severLayout() ),
	mSway( &model->flinchLayout(),
		pickInRange( gUncertaintyParams.swayPeriodMin, gUncertaintyParams.swayPeriodMax, &mRandom ), mRandom.getFloat() ),
	mMesh( model->data() ),
	mHitboxes( &model->hitboxLayout() ),
	mHealth( pickToughness( animations, &mRandom ) ),
	mStaggering( false ),
	mStaggerCooldownTicks( 0 ),
	mPosition( position ),
	mAngleY( AngleMath::wrapSigned( angleY ) ),
	mChase( sightPhase, mRandom.getInt() ), // 丈夫さと同じ種のままだと、選ぶ値の並びが揃うので引き直す
	mPath( sightPhase ),
	mMover( collisionWorld, this ), // this は覚えるだけ（当たり判定を読むのは move のとき＝構築の後）
	mSeparation( 0.f, 0.f, 0.f ),
	mWalking( false ),
	mMoveStartTime( animations.moveStartTime ),
	mState( State::Alive ),
	mStateTicks( 0 ),
	mCrawlCenter( position ),
	mCrawlChestOffset( 0.f, 0.f, 0.f ),
	mCrawlYawOffset( 0.f ),
	mAttackClock( 0.f ),
	mAttackRootMotion( model->rootMotion( animations.attack ) ),
	mStaggerRootMotion( model->rootMotion( animations.stagger ) ),
	mDeathRootMotion( model->rootMotion( animations.death ) ),
	mDownedRootMotion( model->rootMotion( animations.downed ? animations.downed : animations.death ) ), // startDowned と同じ選び方
	mRootMotion( nullptr ),
	mRootPass( RootPass::None ),
	mRootAnchor( position ),
	mRootStartOffset( 0.f, 0.f, 0.f ),
	mRootLastTime( 0.f ),
	mMeshOffset( 0.f, 0.f, 0.f ) {
	ASSERT( streets && "Zombie : streets is null." );
	mMesh.setPosition( mPosition );
	mMesh.setAngleY( mAngleY );
	mPoseModifiers.add( &mCrawl );
	mPoseModifiers.add( &mFlinch );
	mPoseModifiers.add( &mDismemberment );
	mMesh.setPoseModifier( &mPoseModifiers );
	mMesh.setBodyPoseModifier( &mSway ); // 揺れには部位の箱も付いてくる
	// 気付く前は立っている（立つアニメが無ければ歩くアニメ。どちらも空ならレスト姿勢＝T ポーズ）。
	mMesh.setAnimation( locomotionAnimation(), mMoveStartTime );
}

void Zombie::update( const ChaseTarget* target, const HitWorld& hitWorld, const ZombieCrowd* crowd ) {
	// 動いた後の位置・向きでアニメと部位の箱を置くので、先に動く。攻撃の間は追う先を考え直さない（相手の方を向いて寄るだけ）。
	// 攻撃に行っている体は仲間を避けない（つかみに行く動きを崩さない。ほかの体が、この体から離れる）。
	mSeparation.set( 0.f, 0.f, 0.f );
	if ( mState == State::Alive && !mStaggering && canChase() ) {
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
	// 根の動きの区間の間は、置き場所を動かした後に合わせる（applyRootMotion）。
	if ( mState == State::Alive && !mRootMotion ) {
		mPosition.y = mMover.followGround( mPosition );
		mMesh.setPosition( mPosition );
	}
	mSway.update( swayStrength( target ), gUncertaintyParams.swayMaxDegree ); // 姿勢を求める前に、このティックの揺れを決める
	mMesh.update();
	applyRootMotion( target );
	if ( mState == State::Alive ) {
		updateStagger();
	} else if ( mState != State::Downed ) { // Downed は倒れるアニメの最後の姿勢のまま（動かすのは腕と置き場所だけ）
		updateAfterDeath();
	}
	mFlinch.update();
	// 体を引きずり始めた後は（死んだ後も）、起こした頭・動かした腕に当たるよう、見えている姿勢に箱を付ける
	// （頭を 0.2m ほど持ち上げるので、アニメどおりの姿勢のままだと頭を狙っても当たらない）。
	mHitboxes.update( mMesh, mCrawl.isStarted() ? HitboxSet::POSE_SHOWN : HitboxSet::POSE_ANIMATED );
}

void Zombie::chase( const ChaseTarget* target, const HitWorld& hitWorld, const ZombieCrowd* crowd ) {
	Vector3 eye = mPosition;
	eye.y += kEyeHeight;
	ZombieChase::Mode modeBefore = mChase.mode();
	mChase.update( mPosition, eye, mAngleY, target, hitWorld );
	if ( mChase.mode() == ZombieChase::Mode::Search && modeBefore != ZombieChase::Mode::Search ) {
		mPath.resetProgress(); // 着けるかは探し始めてから数える（追っている間に寄れなかった分で、すぐあきらめないように）
	}
	bool facing = false;
	bool walking = mChase.wantsToWalk();
	if ( mChase.mode() == ZombieChase::Mode::Wander ) {
		mPath.clear(); // うろつく先は近く（ZombieChase が通れる所を選ぶ）なので、通り道は使わない
		if ( mChase.hasDestination() ) {
			facing = turnToward( walking ? avoidCrowd( mChase.destination(), crowd ) : mChase.destination() );
		}
	} else {
		// 追う・探す間は、まっすぐ通れなければ通り道の次の点を向く（止まる距離・攻撃の間合いは行き先との距離のまま）。
		// 歩いている間は、近くの仲間から離れる向きを混ぜる（前の仲間を避けて横へ回り込む）。
		// 揺れている間は、倒れている側へ少し流れる（追っている間だけ揺れるので、探す間は weave が何もしない）。
		const Vector3& point = mPath.steer( mPosition, mChase.destination(), *mStreets, hitWorld, walking );
		facing = turnToward( walking ? weave( avoidCrowd( point, crowd ) ) : point );
		if ( mChase.mode() == ZombieChase::Mode::Search && mPath.isStuck() ) {
			mChase.giveUpSearch(); // 最後に見た場所へ着けない（よけても寄れなかった）。次のティックから立ってうろつく
		}
	}
	if ( canStartAttack( target ) ) {
		mPath.clear();
		startAttack();
		mMesh.setAngleY( mAngleY );
		return;
	}
	// 向きを変えている最中も、歩きたいなら歩くアニメにする（その場で向きを変える動きに見せる）。
	setWalking( walking );
	if ( walking && facing ) {
		float distance = walkDistanceThisTick();
		Vector3 step( Math::sin( mAngleY ) * distance, 0.f, Math::cos( mAngleY ) * distance );
		// 壁・車・他のキャラに当たったら沿って滑る（向きを変えてめり込んだ分も、ここで外へ出る）。
		mPosition += mMover.move( step );
	} else if ( !walking && mChase.mode() == ZombieChase::Mode::Chase ) {
		standAside( crowd ); // 追いついて立ち止まっている。仲間の後ろに重なったまま止まり続けないように
	}
	mMesh.setPosition( mPosition );
	mMesh.setAngleY( mAngleY );
}

Vector3 Zombie::avoidCrowd( const Vector3& point, const ZombieCrowd* crowd ) {
	if ( !crowd ) {
		return point;
	}
	const ZombieCrowdParams& params = zombieCrowdParams();
	Vector3 center = bodyCenter();
	Vector3 toPoint( point.x - center.x, 0.f, point.z - center.z );
	float distance = toPoint.length();
	if ( distance < kMinTurnDistance ) {
		return point; // 着いている。向きを決めない（turnToward と同じ）
	}
	// 行き先への向き（長さ 1）に、仲間から離れる向き（長さ 0〜1）× 強さを足す。向く点は、その向きの 1m 先。
	mSeparation = crowd->separation( this, center, params.separationRadius );
	mSeparation *= params.separationStrength;
	Vector3 direction;
	direction.setMul( toPoint, 1.f / distance );
	direction += mSeparation;
	if ( direction.length() < kMinTurnDistance ) {
		return point; // 真後ろから押し返されて向きが消えた。行き先へ向く
	}
	direction.normalize();
	Vector3 facingPoint;
	facingPoint.setAdd( center, direction );
	return facingPoint;
}

float Zombie::swayStrength( const ChaseTarget* target ) const {
	// 遠くでは揺らさない（遠くから頭を狙う楽しさは残す）。音で探しに来る・うろつく間も揺らさない。
	if ( !target || mState != State::Alive || mStaggering || mAttack.isActive() || !canChase() ||
		mChase.mode() != ZombieChase::Mode::Chase ) {
		return 0.f;
	}
	const ZombieUncertaintyParams& params = gUncertaintyParams;
	float distance = flatDistanceTo( target->position );
	float range = params.swayStartDistance - params.swayFullDistance;
	if ( range <= 0.f ) {
		return ( distance <= params.swayFullDistance ) ? 1.f : 0.f; // F6 で幅を 0 以下にしたときは、境目で切り替える
	}
	float t = ScalarMath::clamp( ( params.swayStartDistance - distance ) / range, 0.f, 1.f );
	return ScalarMath::smoothStep( t );
}

Vector3 Zombie::weave( const Vector3& point ) const {
	float degree = mSway.lean() * gUncertaintyParams.weaveDegree;
	if ( degree == 0.f ) {
		return point;
	}
	// 体の向きを正に回すと前は +X（骨格の根の空間の横）へ寄る。傾きが正の側＝倒れている側へ向く点を回す。
	Vector3 center = bodyCenter();
	float x = point.x - center.x;
	float z = point.z - center.z;
	float c = Math::cos( degree );
	float s = Math::sin( degree );
	return Vector3( center.x + x * c + z * s, point.y, center.z - x * s + z * c );
}

void Zombie::standAside( const ZombieCrowd* crowd ) {
	if ( !crowd ) {
		return;
	}
	const ZombieCrowdParams& params = zombieCrowdParams();
	Vector3 aside = crowd->separation( this, bodyCenter(), params.standAsideRadius );
	float length = aside.length();
	if ( length <= 0.f ) {
		return;
	}
	// 向きは変えず（相手の方を向いたまま）、決まった速さで横へずれる。壁・ほかの体に当たったら沿って滑る。
	Vector3 step;
	step.setMul( aside, params.standAsideSpeed / GameTime::kTicksPerSecond / length );
	mPosition += mMover.move( step );
	mSeparation = aside;
}

bool Zombie::canStartAttack( const ChaseTarget* target ) const {
	if ( !mAttackAnimation || !target || !target->grabbable || target->attackTaken || !mAttack.canStart() ) {
		return false;
	}
	if ( mChase.mode() != ZombieChase::Mode::Chase ) {
		return false; // 見えている相手だけを襲う（見失って探している間・うろついている間は襲わない）
	}
	if ( isCrawling() && !canCrawl() ) {
		return false; // 両腕を失った体はつかめない
	}
	float startDistance = isCrawling() ? kCrawlAttackStartDistance : kAttackStartDistance;
	return reachDistanceTo( target->position ) <= startDistance &&
		Math::abs( headingDifferenceTo( target->position ) ) <= kAttackStartHalfAngle;
}

void Zombie::startAttack() {
	mAttack.start();
	if ( isCrawling() ) {
		// 攻撃のアニメは流さない（倒れた姿勢のまま、腕を前へ伸ばす）。段階はアニメと同じ時間割の時計で進める。
		mAttackClock = kAttackStartTime;
		return;
	}
	mMesh.crossFadeTo( mAttackAnimation, kAttackStartTime, kAttackFadeTicks, SkinnedMeshInstance::PLAY_ONCE );
	startRootMotion( mAttackRootMotion, kAttackStartTime, RootPass::Target );
	// 攻撃が終わったら、まず立つ（次の update で追う先を考え直し、離れていれば歩き出す）。
	mWalking = false;
}

void Zombie::approachDuringAttack( const ChaseTarget* target ) {
	if ( !target ) {
		return;
	}
	turnToward( target->position );
	// 寄る距離は、踏み込む前の足元（根の動きのアンカー＝bodyCenter）から測る。寄るのもアンカーごと（踏み込みは別に足す）。
	float remaining = flatDistanceTo( target->position ) - kGrabCloseDistance;
	if ( mAttack.phase() == ZombieAttack::Phase::Reach && remaining > 0.f ) {
		float distance = Math::min( kReachApproachSpeed, remaining );
		Vector3 step( Math::sin( mAngleY ) * distance, 0.f, Math::cos( mAngleY ) * distance );
		moveFooting( mMover.move( step ) ); // 相手の体・銃の箱に当たったら、そこで止まる
	}
	mMesh.setPosition( mPosition );
	mMesh.setAngleY( mAngleY );
}

void Zombie::updateAttack( const ChaseTarget* target ) {
	bool wasActive = mAttack.isActive();
	bool crawling = isCrawling();
	if ( crawling ) {
		// 時計はアニメと同じく「見えている時刻」を渡してから進める（つかんでいる間は同じ速さでゆっくり進む）。
		bool finished = mAttack.isActive() && mAttackClock >= mAttackAnimation.period();
		mAttack.update( mAttackClock, finished, isTargetInReach( target ) );
		if ( mAttack.isActive() ) {
			mAttackClock += mAttack.playbackRate();
		}
	} else {
		mAttack.update( mMesh.animationTime(), mMesh.isAnimationFinished(), isTargetInReach( target ) );
	}
	if ( mAttack.bitThisTick() ) {
		// 噛むアニメが無いので、首と背骨を突き出して顔を近づける（のけぞりと同じく、すぐ曲がって 0.25 秒で戻る）。
		// 立っている体は前へ、体を引きずっている体は上へ（頭を持ち上げて脚へ噛みつく）。
		const Vector3& lunge = crawling ? kCrawlBiteLungeDirection : kBiteLungeDirection;
		for ( int i = 0; i < kBiteHeadPushes; ++i ) {
			mFlinch.push( BodyPart::Head, lunge );
		}
		mFlinch.push( BodyPart::Torso, lunge );
	}
	if ( crawling ) {
		return; // 腕を伸ばす・戻すのは ZombieCrawl（updateDowned）。アニメは替えない
	}
	if ( mAttack.isActive() ) {
		mMesh.setPlaybackRate( mAttack.playbackRate() );
	} else if ( wasActive ) {
		// 攻撃のアニメの最後（元の立ち姿）から、立つアニメへ戻す。腰は始めた所へ戻っているので、置き場所も絵の根へ戻す。
		mMesh.crossFadeTo( locomotionAnimation(), kMoveRestartTime, kRecoverFadeTicks );
		endRootMotion();
	}
}

bool Zombie::isTargetInReach( const ChaseTarget* target ) const {
	if ( !target || !target->grabbable ) {
		return false;
	}
	float reach = isCrawling() ? kCrawlGrabReachDistance : kGrabReachDistance;
	return reachDistanceTo( target->position ) <= reach &&
		Math::abs( headingDifferenceTo( target->position ) ) <= kGrabHalfAngle;
}

float Zombie::reachDistanceTo( const Vector3& target ) const {
	// 寝ている体は足元から前へ 1.5m ほど伸びているので、手の届く距離は頭から測る。
	return isCrawling() ? FlatMath::distance( headPosition(), target ) : flatDistanceTo( target );
}

float Zombie::flatDistanceTo( const Vector3& target ) const {
	return FlatMath::distance( bodyCenter(), target );
}

float Zombie::headingDifferenceTo( const Vector3& target ) const {
	Vector3 toTarget;
	toTarget.setSub( target, bodyCenter() );
	if ( toTarget.x == 0.f && toTarget.z == 0.f ) {
		return 0.f; // 真上は向きが無い（向いているとみなす）
	}
	return AngleMath::difference( AngleMath::headingOf( toTarget ), mAngleY );
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
	toDestination.setSub( destination, bodyCenter() );
	toDestination.y = 0.f;
	if ( toDestination.length() < kMinTurnDistance ) {
		return true;
	}
	// 向き 0 が +Z、90 が +X（AngleMath）。
	float difference = AngleMath::difference( AngleMath::headingOf( toDestination ), mAngleY );
	float turnLimit = isCrawling() ? kCrawlTurnPerTick : kTurnPerTick;
	float turn = Math::max( -turnLimit, Math::min( turnLimit, difference ) );
	mAngleY = AngleMath::wrapSigned( mAngleY + turn );
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
	// 振りほどきには、部位の倍率を掛けた後の量を数える（全体の体力が減った分）。
	float totalBefore = mHealth.total();
	mHealth.takeDamage( damage.part, damage.baseAmount, mRandom.getFloat() ); // 頭は確率で弾ける（クリティカル）
	float dealt = totalBefore - mHealth.total();
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
	} else if ( mAttack.takeShakeDamage( dealt ) ) {
		// 体を引きずっている体はよろけない（手を放して腕を戻すだけ。腕を戻すのは ZombieCrawl）。
		if ( mState == State::Alive ) {
			shakeOff();
		}
	} else if ( mState == State::Alive && !mAttack.isActive() && canStagger( damage.part ) ) {
		startStagger(); // 攻撃の途中はよろけない（攻撃のアニメが途切れないように。のけぞりだけ）
	}
}

void Zombie::shakeOff() {
	// 離すときは、よろけて手を放す（よろけのアニメが無ければ、立つアニメへ戻すだけ）。よろけの間あけは待たない。
	if ( mStaggerAnimation ) {
		startStagger();
	} else {
		mMesh.crossFadeTo( locomotionAnimation(), kMoveRestartTime, kRecoverFadeTicks );
		endRootMotion();
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
	// 根の動きも、立っていたときだけ始め直す（Downed は倒れる区間をそのまま続ける。引きずり始めた後は区間が無い）。
	if ( mState == State::Alive ) {
		mMesh.crossFadeTo( mDeathAnimation, kDeathStartTime, kDeathFadeTicks, SkinnedMeshInstance::PLAY_ONCE );
		startRootMotion( mDeathRootMotion, kDeathStartTime, RootPass::AllBodies );
	}
	mState = State::Falling;
	mAttack.cancel(); // つかんでいたら離す
	mStateTicks = 0;
	mStaggering = false;
}

void Zombie::startDowned() {
	// 這うアニメが無いので、前へ倒れるアニメで倒し、最後の姿勢で止めて代わりにする（ZombieDismember_Plan.txt の 5 章）。
	// 倒れ終わったら、その姿勢のまま腕で体を引きずる（ZombieCrawl_Plan.txt の 2 章）。
	Scene::Animation fall = mDownedAnimation ? mDownedAnimation : mDeathAnimation;
	mMesh.crossFadeTo( fall, kDeathStartTime, kDeathFadeTicks, SkinnedMeshInstance::PLAY_ONCE );
	startRootMotion( mDownedRootMotion, kDeathStartTime, RootPass::AllBodies );
	mState = State::Downed;
	mAttack.cancel();
	mStaggering = false;
}

void Zombie::updateDowned( const ChaseTarget* target, const HitWorld& hitWorld ) {
	if ( !mCrawl.isStarted() ) {
		if ( !canStartCrawling() ) {
			return; // まだ倒れている途中（または引きずれない体）。倒れた場所の姿勢のまま
		}
		endRootMotion(); // ここから先は胸の位置で動かす（足元は絵の根から測り直す）
		startCrawling();
	}
	float pace = kStillPace;
	bool moving = false;
	if ( mAttack.isActive() ) {
		if ( target ) {
			turnToward( target->position ); // つかみに行く・つかんでいる間は、相手の方を向くだけ（寄らない）
		}
	} else {
		pace = crawlChase( target, hitWorld, &moving );
	}
	mCrawl.update( pace, mAttack.isAttacking() );
	if ( moving ) {
		// 腕で引いた分だけ進む（引いていない腕・失った腕の分は進まない）。壁・車に当たったら胸の位置で沿って滑る。
		float distance = mCrawl.pullDistance( hasArm( ZombieCrawlLayout::SIDE_LEFT ), hasArm( ZombieCrawlLayout::SIDE_RIGHT ) );
		if ( distance > 0.f ) {
			Vector3 step( Math::sin( mAngleY ) * distance, 0.f, Math::cos( mAngleY ) * distance );
			mCrawlCenter += mMover.move( step );
		}
	}
	placeCrawlingBody();
	// 見えている相手（追っている・つかみに行っている間）へ顔を向ける。置き場所を決めた後の骨格の根の空間で渡す。
	bool looking = target && ( mChase.mode() == ZombieChase::Mode::Chase || mAttack.isActive() );
	Vector3 lookPoint( 0.f, 0.f, 0.f );
	if ( looking ) {
		Matrix34 worldToRoot;
		worldToRoot.setInverse( mMesh.rootMatrix() );
		worldToRoot.mul( &lookPoint, target->eyePosition );
	}
	mCrawl.setLookTarget( looking, lookPoint );
}

bool Zombie::canStartCrawling() const {
	if ( !mDownedAnimation || !canChase() || !mModel->crawlLayout().isValid() ) {
		return false; // 前へ倒れるアニメが無い（仰向けかもしれない）・確認用の体・腕の骨が無い
	}
	return mMesh.isAnimationFinished() || mMesh.animationTime() >= kCrawlStartTime;
}

void Zombie::startCrawling() {
	const ZombieCrawlLayout& layout = mModel->crawlLayout();
	Matrix34 hips;
	Matrix34 chest;
	Matrix34 neck;
	if ( !mMesh.animatedBoneMatrix( layout.hips(), &hips ) || !mMesh.animatedBoneMatrix( layout.chest(), &chest ) ||
		!mMesh.animatedBoneMatrix( layout.neck(), &neck ) ) {
		return; // まだ update していない（倒れ終わってから呼ぶので起きない）
	}
	// 骨の位置を、置き場所と向きを掛ける前（骨格の根の空間）へ戻して測る。
	Matrix34 worldToRoot;
	worldToRoot.setInverse( mMesh.rootMatrix() );
	Vector3 hipsRoot;
	Vector3 chestRoot;
	Vector3 neckRoot;
	worldToRoot.mul( &hipsRoot, PlacementMatrix::translationOf( hips ) );
	worldToRoot.mul( &chestRoot, PlacementMatrix::translationOf( chest ) );
	worldToRoot.mul( &neckRoot, PlacementMatrix::translationOf( neck ) );
	Vector3 bodyDirection;
	bodyDirection.setSub( neckRoot, hipsRoot );
	mCrawlYawOffset = AngleMath::headingOf( bodyDirection );
	mCrawlChestOffset.set( chestRoot.x, 0.f, chestRoot.z );
	// 見た目の向き（今の mAngleY）は変えず、進む向きを体の向きへ。胸の位置も今の見た目から求める（どちらも見た目は飛ばない）。
	Vector3 chestOffset;
	mMesh.rootMatrix().mul33( &chestOffset, mCrawlChestOffset );
	mCrawlCenter.setAdd( mPosition, chestOffset );
	mAngleY = AngleMath::wrapSigned( mAngleY + mCrawlYawOffset );
	mPath.clear();
	mCrawl.start();
}

float Zombie::crawlChase( const ChaseTarget* target, const HitWorld& hitWorld, bool* moving ) {
	*moving = false;
	if ( !canCrawl() ) {
		return kStillPace; // 両腕を失った。倒れたまま動かない
	}
	Vector3 eye = mCrawlCenter;
	eye.y += kCrawlEyeHeight;
	ZombieChase::Mode modeBefore = mChase.mode();
	mChase.update( mCrawlCenter, eye, mAngleY, target, hitWorld );
	if ( mChase.mode() == ZombieChase::Mode::Search && modeBefore != ZombieChase::Mode::Search ) {
		mPath.resetProgress(); // 着けるかは探し始めてから数える（chase と同じ）
	}
	if ( mChase.mode() == ZombieChase::Mode::Wander ) {
		mPath.clear();
		return kStrugglePace; // 気付いていない間はうろつかず、その場でもがくだけ
	}
	bool facing = turnToward( mPath.steer( mCrawlCenter, mChase.destination(), *mStreets, hitWorld, mChase.wantsToWalk() ) );
	if ( mChase.mode() == ZombieChase::Mode::Search && mPath.isStuck() ) {
		mChase.giveUpSearch();
	}
	if ( canStartAttack( target ) ) {
		mPath.clear();
		startAttack();
		return kStillPace;
	}
	if ( !mChase.wantsToWalk() ) {
		return kStrugglePace; // 追いついて待っている
	}
	// 向きを変えている最中も腕は動かす（その場で向きを変える動きに見せる）。進むのは向き終わってから。
	*moving = facing;
	return kCrawlPace;
}

void Zombie::placeCrawlingBody() {
	mMesh.setAngleY( AngleMath::wrapSigned( mAngleY - mCrawlYawOffset ) );
	mCrawlCenter.y = mMover.followGround( mCrawlCenter );
	// 足元は、胸から見た目の向きで戻した所（胸を中心に向きを変えるので、向きを変えると足元が回る）。
	Vector3 chestOffset;
	mMesh.rootMatrix().mul33( &chestOffset, mCrawlChestOffset );
	mPosition.setSub( mCrawlCenter, chestOffset );
	mPosition.y = mCrawlCenter.y;
	mMesh.setPosition( mPosition );
}

bool Zombie::hasArm( ZombieCrawlLayout::Side side ) const {
	// 前腕から先が無ければ引けない（上腕を失ったときは、その先の前腕も無い）。
	BodyPart foreArm = ( side == ZombieCrawlLayout::SIDE_LEFT ) ? BodyPart::LeftForeArm : BodyPart::RightForeArm;
	return !mDismemberment.isLost( foreArm );
}

bool Zombie::canCrawl() const {
	return hasArm( ZombieCrawlLayout::SIDE_LEFT ) || hasArm( ZombieCrawlLayout::SIDE_RIGHT );
}

bool Zombie::isCrawling() const {
	return mState == State::Downed && mCrawl.isStarted();
}

Vector3 Zombie::bodyCenter() const {
	if ( isCrawling() ) {
		return mCrawlCenter;
	}
	return mRootMotion ? mRootAnchor : mPosition;
}

void Zombie::startRootMotion( const ZombieRootMotion* table, float startTime, RootPass pass ) {
	// 前の区間は閉じ、置き場所を今見えている絵の根へ戻してから、そこをアンカーにする。
	endRootMotion();
	if ( !table ) {
		return;
	}
	mRootMotion = table;
	mRootPass = pass;
	mRootAnchor = mPosition;
	mRootStartOffset = table->offsetAt( startTime );
	mRootLastTime = -1.f; // 最初の applyRootMotion で必ず当たり直す（アニメ時刻は 0 以上）
}

void Zombie::applyRootMotion( const ChaseTarget* target ) {
	if ( !mRootMotion ) {
		return;
	}
	float time = mMesh.animationTime();
	if ( time != mRootLastTime ) {
		// 始めた時刻からの根の水平の動きを、今の向きでワールドへ（攻撃中は相手の方へ向きを変えるので、毎回回し直す）。
		Vector3 rootDelta = mRootMotion->offsetAt( time );
		rootDelta -= mRootStartOffset;
		Vector3 wanted;
		mMesh.rootMatrix().mul33( &wanted, rootDelta );
		wanted.y = 0.f;
		// 移動の箱はアンカーにある（bodyCenter）ので、毎回アンカーから当たり直す。1 ティックごとの差を足していくと、
		// 踏み込みを壁で止められた後の戻りで、始めた所より後ろへ下がってしまう。
		Vector3 moved( 0.f, 0.f, 0.f );
		switch ( mRootPass ) {
			case RootPass::Target:    moved = mMover.movePassingThrough( wanted, target ? target->body : nullptr ); break;
			case RootPass::AllBodies: moved = mMover.moveIgnoringBodies( wanted ); break;
			case RootPass::None:      moved = mMover.move( wanted ); break;
		}
		mPosition.x = mRootAnchor.x + moved.x;
		mPosition.z = mRootAnchor.z + moved.z;
		// 絵の根は、置き場所から根の動きの分だけ戻した所。当たらなければアンカーのまま（今までと同じ見た目）で、
		// 当たって進めなかった分だけ手前へ戻る。
		mMeshOffset.set( -wanted.x, 0.f, -wanted.z );
		mRootLastTime = time;
	}
	// 足元の高さは、腰の下（置き場所）の地面に合わせる（歩道の端で倒れても浮かない）。
	mPosition.y = mMover.followGround( mPosition );
	mRootAnchor.y = mPosition.y;
	mMesh.setPosition( meshRoot() );
}

void Zombie::endRootMotion() {
	if ( !mRootMotion ) {
		return;
	}
	Vector3 root = meshRoot();
	mPosition.x = root.x;
	mPosition.z = root.z;
	mMeshOffset.set( 0.f, 0.f, 0.f );
	mRootMotion = nullptr;
	mMesh.setPosition( mPosition );
}

void Zombie::freezeRootMotion() {
	mRootMotion = nullptr;
}

Vector3 Zombie::meshRoot() const {
	Vector3 root;
	root.setAdd( mPosition, mMeshOffset );
	return root;
}

void Zombie::moveFooting( const Vector3& moved ) {
	if ( mRootMotion ) {
		mRootAnchor += moved;
	}
	mPosition += moved;
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
	startRootMotion( mStaggerRootMotion, kStaggerStartTime, RootPass::None );
	mStaggering = true;
}

void Zombie::updateStagger() {
	if ( mStaggering ) {
		if ( mMesh.isAnimationFinished() ) {
			// よろける前に歩いていたか立っていたかに戻す（次の update で追う先を考え直し、要れば切り替える）。
			mMesh.crossFadeTo( locomotionAnimation(), kMoveRestartTime, kRecoverFadeTicks );
			endRootMotion(); // 腰は下がった後に元の所へ戻っている
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
				freezeRootMotion(); // 置き場所は死体の腰の下・絵の根はそのまま（もう動かない）
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
			Vector3 shown = meshRoot();
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

void Zombie::hearNoise( const Vector3& position ) {
	bool listening = ( mState == State::Alive && !mStaggering ) || isCrawling();
	if ( !listening || mAttack.isActive() || !canChase() ) {
		return; // 倒れている途中・死んだ・よろけている・攻撃している体と、追ってこない確認用の体は聞かない
	}
	mChase.hearNoise( position );
}

const ZombiePath& Zombie::path() const {
	return mPath;
}

const Vector3& Zombie::separation() const {
	return mSeparation;
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
		case State::Corpse:  return GameTime::ticksToSeconds( kCorpseTicks - mStateTicks + kSinkTicks );
		case State::Sinking: return GameTime::ticksToSeconds( kSinkTicks - mStateTicks );
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
	for ( int i = 0; i < mHitboxes.boxNumber(); ++i ) {
		Matrix34 box;
		if ( mHitboxes.boxPart( i ) == BodyPart::Head && mHitboxes.boxMatrix( i, &box ) ) {
			return Vector3( box.m03, box.m13, box.m23 ); // -1〜1 の立方体の中心が置かれた所
		}
	}
	Vector3 eye = mPosition;
	eye.y += kEyeHeight;
	return eye;
}

const Vector3& Zombie::position() const {
	return mPosition;
}
