#include "GameSrc/Zombie/ZombieLocomotion.h"
#include "GameSrc/Zombie/ZombieCrowd.h"
#include "GameSrc/Zombie/ZombieCrowdParams.h"
#include "GameSrc/Collision/CharacterMover.h"
#include "GameSrc/Core/GameTime.h"
#include "GameSrc/Math/AngleMath.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Functions.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// 向きを変える速さ（度/ティック）。立っている体は 1 秒に 180°（急に振り向かない）。
	// 這っている体は 1 秒に 60°（立っている時の 1/3。寝た体は大きく回せない）。
	const float kTurnPerTick = 180.f / GameTime::kTicksPerSecond;
	const float kCrawlTurnPerTick = 60.f / GameTime::kTicksPerSecond;
	// 行き先との向きの差がこれより大きい間は、その場で向きを変えるだけで進まない（横歩き・後ろ歩きに見えないように）。
	const float kTurnInPlaceDegree = 60.f;
	// 行き先がこれより近ければ、向きを決めない（真上に居るときに向きが定まらず回り続けないように。m）。
	const float kMinTurnDistance = 0.05f;
}

ZombieLocomotion::ZombieLocomotion( const Zombie* self, const NavMesh* navMesh, const CharacterMover* mover, int sightPhase, int randomSeed ) :
	mSelf( self ),
	mNavMesh( navMesh ),
	mMover( mover ),
	mChase( sightPhase, randomSeed ),
	mPath( sightPhase ),
	mSeparation( 0.f, 0.f, 0.f ) {
	ASSERT( self && "ZombieLocomotion : self is null." );
	ASSERT( navMesh && "ZombieLocomotion : navMesh is null." );
	ASSERT( mover && "ZombieLocomotion : mover is null." );
}

void ZombieLocomotion::look( const Vector3& center, const Vector3& eye, float angleY, const ChaseTarget* target, const HitWorld& hitWorld ) {
	ZombieChase::Mode modeBefore = mChase.mode();
	mChase.update( center, eye, angleY, target, hitWorld );
	if ( mChase.mode() == ZombieChase::Mode::Search && modeBefore != ZombieChase::Mode::Search ) {
		mPath.resetProgress(); // 着けるかは探し始めてから数える（追っている間に寄れなかった分で、すぐあきらめないように）
	}
}

ZombieLocomotion::Intent ZombieLocomotion::thinkStanding( const Vector3& center, const Vector3& eye, const ChaseTarget* target,
const HitWorld& hitWorld, const ZombieCrowd* crowd, float weaveDegree, float* angleY ) {
	ASSERT( angleY && "ZombieLocomotion : angleY is null." );
	look( center, eye, *angleY, target, hitWorld );
	Intent intent;
	intent.walking = mChase.wantsToWalk();
	intent.facing = false;
	intent.wandering = ( mChase.mode() == ZombieChase::Mode::Wander );
	if ( intent.wandering ) {
		mPath.clear(); // うろつく先は近く（ZombieChase が通れる所を選ぶ）なので、通り道は使わない
		if ( mChase.hasDestination() ) {
			const Vector3& destination = mChase.destination();
			intent.facing = turnToward( center, intent.walking ? avoidCrowd( center, destination, crowd ) : destination, Posture::Standing, angleY );
		}
		return intent;
	}
	// 追う・探す間は、まっすぐ通れなければ通り道の次の点を向く（止まる距離・攻撃の間合いは行き先との距離のまま）。
	// 歩いている間は、近くの仲間から離れる向きを混ぜる（前の仲間を避けて横へ回り込む）。
	// 揺れている間は、倒れている側へ少し流れる（追っている間だけ揺れるので、探す間は weaveDegree が 0）。
	const Vector3& point = mPath.steer( center, mChase.destination(), *mNavMesh, intent.walking );
	Vector3 facingPoint = intent.walking ? weave( center, avoidCrowd( center, point, crowd ), weaveDegree ) : point;
	intent.facing = turnToward( center, facingPoint, Posture::Standing, angleY );
	if ( mChase.mode() == ZombieChase::Mode::Search && mPath.isStuck() ) {
		mChase.giveUpSearch(); // 最後に見た場所へ着けない（よけても寄れなかった）。次のティックから立ってうろつく
	}
	return intent;
}

ZombieLocomotion::Intent ZombieLocomotion::thinkCrawling( const Vector3& center, const Vector3& eye, const ChaseTarget* target,
const HitWorld& hitWorld, float* angleY ) {
	ASSERT( angleY && "ZombieLocomotion : angleY is null." );
	look( center, eye, *angleY, target, hitWorld );
	Intent intent;
	intent.walking = false;
	intent.facing = false;
	intent.wandering = ( mChase.mode() == ZombieChase::Mode::Wander );
	if ( intent.wandering ) {
		mPath.clear(); // 気付いていない間はうろつかず、その場でもがくだけ
		return intent;
	}
	intent.facing = turnToward( center, mPath.steer( center, mChase.destination(), *mNavMesh, mChase.wantsToWalk() ),
		Posture::Crawling, angleY );
	if ( mChase.mode() == ZombieChase::Mode::Search && mPath.isStuck() ) {
		mChase.giveUpSearch();
	}
	// 歩きたいかは、あきらめた後で読む（今までの這う追い方と同じ順）。
	intent.walking = mChase.wantsToWalk();
	return intent;
}

bool ZombieLocomotion::turnToward( const Vector3& center, const Vector3& destination, Posture posture, float* angleY ) const {
	ASSERT( angleY && "ZombieLocomotion : angleY is null." );
	Vector3 toDestination;
	toDestination.setSub( destination, center );
	toDestination.y = 0.f;
	if ( toDestination.length() < kMinTurnDistance ) {
		return true;
	}
	// 向き 0 が +Z、90 が +X（AngleMath）。
	float difference = AngleMath::difference( AngleMath::headingOf( toDestination ), *angleY );
	float turnLimit = ( posture == Posture::Crawling ) ? kCrawlTurnPerTick : kTurnPerTick;
	float turn = Math::max( -turnLimit, Math::min( turnLimit, difference ) );
	*angleY = AngleMath::wrapSigned( *angleY + turn );
	return Math::abs( difference - turn ) <= kTurnInPlaceDegree;
}

Vector3 ZombieLocomotion::walkStep( float distance, float angleY ) const {
	Vector3 step( Math::sin( angleY ) * distance, 0.f, Math::cos( angleY ) * distance );
	// 壁・車・ほかの体に当たったら沿って滑る（向きを変えてめり込んだ分も、ここで外へ出る）。
	return mMover->move( step );
}

void ZombieLocomotion::standAside( const Vector3& center, const ZombieCrowd* crowd, Vector3* footing ) {
	ASSERT( footing && "ZombieLocomotion : footing is null." );
	if ( !crowd ) {
		return;
	}
	const ZombieCrowdParams& params = zombieCrowdParams();
	Vector3 aside = crowd->separation( mSelf, center, params.standAsideRadius );
	float length = aside.length();
	if ( length <= 0.f ) {
		return;
	}
	// 向きは変えず（相手の方を向いたまま）、決まった速さで横へずれる。壁・ほかの体に当たったら沿って滑る。
	Vector3 step;
	step.setMul( aside, params.standAsideSpeed / GameTime::kTicksPerSecond / length );
	*footing += mMover->move( step );
	mSeparation = aside;
}

Vector3 ZombieLocomotion::avoidCrowd( const Vector3& center, const Vector3& point, const ZombieCrowd* crowd ) {
	if ( !crowd ) {
		return point;
	}
	const ZombieCrowdParams& params = zombieCrowdParams();
	Vector3 toPoint( point.x - center.x, 0.f, point.z - center.z );
	float distance = toPoint.length();
	if ( distance < kMinTurnDistance ) {
		return point; // 着いている。向きを決めない（turnToward と同じ）
	}
	// 行き先への向き（長さ 1）に、仲間から離れる向き（長さ 0〜1）× 強さを足す。向く点は、その向きの 1m 先。
	mSeparation = crowd->separation( mSelf, center, params.separationRadius );
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

Vector3 ZombieLocomotion::weave( const Vector3& center, const Vector3& point, float degree ) const {
	if ( degree == 0.f ) {
		return point;
	}
	// 体の向きを正に回すと前は +X（骨格の根の空間の横）へ寄る。傾きが正の側＝倒れている側へ向く点を回す。
	float x = point.x - center.x;
	float z = point.z - center.z;
	float c = Math::cos( degree );
	float s = Math::sin( degree );
	return Vector3( center.x + x * c + z * s, point.y, center.z - x * s + z * c );
}

void ZombieLocomotion::beginTick() {
	mSeparation.set( 0.f, 0.f, 0.f );
}

void ZombieLocomotion::alert() {
	mChase.alert();
}

void ZombieLocomotion::hearNoise( const Vector3& position ) {
	mChase.hearNoise( position );
}

void ZombieLocomotion::clearPath() {
	mPath.clear();
}

ZombieChase::Mode ZombieLocomotion::mode() const {
	return mChase.mode();
}

const ZombiePath& ZombieLocomotion::path() const {
	return mPath;
}

const Vector3& ZombieLocomotion::separation() const {
	return mSeparation;
}
