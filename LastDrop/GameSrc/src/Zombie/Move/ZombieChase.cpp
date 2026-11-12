#include "GameSrc/Zombie/Move/ZombieChase.h"
#include "GameSrc/Zombie/Move/ChaseTarget.h"
#include "GameSrc/Hit/HitWorld.h"
#include "GameSrc/Core/GameTime.h"
#include "GameSrc/Math/FlatMath.h"

#include "GameLib/Math/Functions.h"

using namespace GameLib::Math;

namespace{
	// 気付く
	// 前（向きから左右 kFieldOfViewHalfDegree の内）に居る自キャラは、これより近ければ気付く（m。目から目まで）
	// 追っている間は、向き問わずこの距離まで見失わない
	const float kFarSightRange = 15.f;
	// 横・後ろに居る自キャラは、これより近いときだけ気付く（m）。後ろから近づけるように前より短くする
	const float kNearSightRange = 10.f;
	// 前とみなす範囲（度。向きから左右それぞれ）。上下は見ない（水平の向きだけ）
	const float kFieldOfViewHalfDegree = 60.f;
	// 見通しの線を引く間隔（秒）。気付くのがこれだけ遅れてもよい長さ
	const float kSightIntervalSeconds = 0.3f;
	const int kSightIntervalTicks = GameTime::secondsToTicks( kSightIntervalSeconds );

	// ── 近づく──
	// 自キャラにこれより近づいたら止まる（m。足元どうし、水平だけ）
	const float kStopDistance = 1.2f;
	// 止まった後、自キャラがこれより離れたら歩き出す（止まる距離より少し長くして、境目で歩く・止まるを繰り返さない）
	const float kResumeDistance = 1.5f;
	// 最後に見た場所・うろつく先にこれより近づいたら着いたとみなす（m）
	const float kArriveDistance = 0.5f;

	// ── うろつく──
	// うろつく先の向きの幅（度。今の向きから左右それぞれ）。前寄りにして、行ったり来たりせず少しずつ町を流れていく
	const float kWanderHalfAngle = 90.f;
	// うろつく先までの距離（m）
	const float kWanderMinDistance = 3.f;
	const float kWanderMaxDistance = 8.f;
	// 着いてから次を選ぶまで立っている長さ（秒。毎回この間でランダム）
	const float kWanderWaitMinSeconds = 1.f;
	const float kWanderWaitMaxSeconds = 3.f;
	// うろつく先を選び直す回数。全部壁に当たったら、もう一度立ってから後ろ向きを基準に選ぶ
	const int kWanderPickTries = 4;
	// うろつく先へ通るかを調べる線の高さ（m。足元から）。歩道の段差（0.15m）には当たらず、車・壁には当たる高さ
	const float kWanderProbeHeight = 0.5f;
	// 線を行き先より先まで伸ばす長さ（m）。行き先が壁にくっついて、体（長方形）がめり込む先を選ばないように
	const float kWanderClearance = 0.5f;
	// この長さ（m）近づくたびに「近づけている」とみなす。壁に沿って滑るだけで行き先へ寄れていないのを見分ける
	const float kWanderProgressDistance = 0.3f;
	// 近づけないまま、これだけ過ぎたら選び直す（秒）
	const float kWanderStallSeconds = 1.f;
	const int kWanderStallTicks = GameTime::secondsToTicks( kWanderStallSeconds );
	const float kHalfTurn = 180.f;

	// ── 銃声を聞く──
	// 聞いてから向かい始めるまで（秒。体ごと・聞くたびにこの間でランダム）。全員が同じ瞬間に振り向かないように
	const float kNoiseDelayMinSeconds = 0.2f;
	const float kNoiseDelayMaxSeconds = 1.0f;

	// 向き angleY（度。0 は +Z、90 は +X）の水平の向き（長さ 1）
	Vector3 forwardOf( float angleY ) {
		return Vector3( GameLib::Math::sin( angleY ), 0.f, GameLib::Math::cos( angleY ) );
	}

	// toTarget が、向き angleY から左右 kFieldOfViewHalfDegree の内か（水平だけで見る）
	bool isInFieldOfView( float angleY, const Vector3& toTarget ) {
		Vector3 flat( toTarget.x, 0.f, toTarget.z );
		float length = flat.length();
		if ( length <= 0.f ) {
			return true; // 真上・真下は向きが無いので前とみなす
		}
		return forwardOf( angleY ).dot( flat ) >= GameLib::Math::cos( kFieldOfViewHalfDegree ) * length;
	}
}

ZombieChase::ZombieChase( int sightPhase, int randomSeed ) :
	mMode( Mode::Wander ),
	mDestination( 0.f, 0.f, 0.f ),
	mRandom( Random::create( randomSeed ) ),
	mWalking( false ),
	mSeesTarget( false ),
	mAlerted( false ),
	mSightTicks( ( sightPhase >= 0 ? sightPhase : -sightPhase ) % kSightIntervalTicks ),
	mNoisePending( false ),
	mNoisePosition( 0.f, 0.f, 0.f ),
	mNoiseDelayTicks( 0 ),
	mWanderWaitTicks( 0 ),
	mWanderBestDistance( 0.f ),
	mWanderStallTicks( 0 ),
	mWanderTurnBack( false ) {
	startWanderWait(); // 最初は立つ（立つ長さは体ごとにランダムなので、一斉には歩き出さない）
}

void ZombieChase::alert() {
	mAlerted = true;
}

void ZombieChase::giveUpSearch() {
	if ( mMode == Mode::Search ) {
		startWanderWait(); // 着いたときと同じく、立ってからうろつく
	}
}

void ZombieChase::hearNoise( const Vector3& position ) {
	if ( mMode == Mode::Chase ) {
		return; // 見えている相手を追っている間は、銃声で行き先を変えない
	}
	mNoisePosition = position;
	if ( !mNoisePending ) {
		mNoisePending = true;
		float seconds = mRandom.getFloat( kNoiseDelayMinSeconds, kNoiseDelayMaxSeconds );
		mNoiseDelayTicks = GameTime::secondsToTicks( seconds );
	}
}

void ZombieChase::update( const Vector3& self, const Vector3& eye, float angleY, const ChaseTarget* target,
const HitWorld& hitWorld ) {
	updateSight( eye, angleY, target, hitWorld );
	if ( mSeesTarget ) {
		mMode = Mode::Chase;
		mDestination = target->position; // 見えている間は、線を引かないティックも今の位置を追う
		mNoisePending = false;           // 見えているので、聞いた所へは行かない
	} else if ( mMode == Mode::Chase ) {
		mMode = Mode::Search; // 最後に見た場所（mDestination）へ向かう
		mWalking = true;
	} else {
		updateNoise();
	}

	switch ( mMode ) {
		case Mode::Wander:
			updateWander( self, angleY, hitWorld );
			break;
		case Mode::Chase:
			updateChase( self );
			break;
		case Mode::Search:
			updateSearch( self );
			break;
	}
}

void ZombieChase::updateSight( const Vector3& eye, float angleY, const ChaseTarget* target, const HitWorld& hitWorld ) {
	if ( !target ) {
		mSeesTarget = false; // 追う相手が居ない
		mAlerted = false;
		return;
	}
	if ( mAlerted ) {
		mAlerted = false;
		mSeesTarget = true; // 撃たれた所（自キャラの居る所）は分かる。次に線を引いて見えなければ、そこを最後に見た場所にする
		mSightTicks = 0;
	} else if ( --mSightTicks <= 0 ) {
		mSightTicks = kSightIntervalTicks;
		mSeesTarget = canSee( eye, angleY, *target, hitWorld );
	}
}

bool ZombieChase::canSee( const Vector3& eye, float angleY, const ChaseTarget& target, const HitWorld& hitWorld ) const {
	Vector3 toTarget;
	toTarget.setSub( target.eyePosition, eye );
	// 重い線より先に、距離と向きで外す（後ろの遠くに居る相手には線を引かない）
	bool far = ( mMode == Mode::Chase ) || isInFieldOfView( angleY, toTarget );
	float range = far ? kFarSightRange : kNearSightRange;
	if ( toTarget.squareLength() > range * range ) {
		return false;
	}
	return !hitWorld.isBlocked( eye, toTarget );
}

void ZombieChase::updateNoise() {
	if ( !mNoisePending || --mNoiseDelayTicks > 0 ) {
		return;
	}
	mNoisePending = false;
	mMode = Mode::Search; // 見失ったときと同じく、行き先へ歩き、着いても見えなければ立ってからうろつく
	mDestination = mNoisePosition;
	mWalking = true;
}

void ZombieChase::updateChase( const Vector3& self ) {
	float distance = FlatMath::distance( self, mDestination );
	// 止まっている間は、少し離れるまで歩き出さない
	mWalking = mWalking ? ( distance > kStopDistance ) : ( distance > kResumeDistance );
}

void ZombieChase::updateSearch( const Vector3& self ) {
	if ( FlatMath::distance( self, mDestination ) <= kArriveDistance ) {
		startWanderWait(); // 着いた。見当たらないので、そこで立ってからうろつく
	} else {
		mWalking = true;
	}
}

void ZombieChase::updateWander( const Vector3& self, float angleY, const HitWorld& hitWorld ) {
	if ( !mWalking ) {
		if ( --mWanderWaitTicks > 0 ) {
			return;
		}
		if ( !pickWanderPoint( self, angleY, hitWorld ) ) {
			startWanderWait();
			mWanderTurnBack = true; // 前は塞がっている（角に向いている等）ので、次は後ろ向きを基準に選ぶ
			return;
		}
		mWanderTurnBack = false;
		mWalking = true;
		mWanderBestDistance = FlatMath::distance( self, mDestination );
		mWanderStallTicks = 0;
		return;
	}

	float distance = FlatMath::distance( self, mDestination );
	if ( distance <= kArriveDistance ) {
		startWanderWait();
		return;
	}
	if ( distance < mWanderBestDistance - kWanderProgressDistance ) {
		mWanderBestDistance = distance;
		mWanderStallTicks = 0;
	} else if ( ++mWanderStallTicks >= kWanderStallTicks ) {
		// 壁に沿って滑るだけで寄れていない。立たずに、次のティックで今の向きから選び直す
		mWalking = false;
		mWanderWaitTicks = 0;
	}
}

void ZombieChase::startWanderWait() {
	mMode = Mode::Wander;
	mWalking = false;
	float seconds = mRandom.getFloat( kWanderWaitMinSeconds, kWanderWaitMaxSeconds );
	mWanderWaitTicks = GameTime::secondsToTicks( seconds );
}

bool ZombieChase::pickWanderPoint( const Vector3& self, float angleY, const HitWorld& hitWorld ) {
	float baseAngle = mWanderTurnBack ? angleY + kHalfTurn : angleY;
	Vector3 probeOrigin( self.x, self.y + kWanderProbeHeight, self.z );
	for ( int i = 0; i < kWanderPickTries; ++i ) {
		Vector3 direction = forwardOf( baseAngle + mRandom.getFloat( -kWanderHalfAngle, kWanderHalfAngle ) );
		float distance = mRandom.getFloat( kWanderMinDistance, kWanderMaxDistance );
		Vector3 probe;
		probe.setMul( direction, distance + kWanderClearance );
		if ( hitWorld.isBlocked( probeOrigin, probe ) ) {
			continue;
		}
		Vector3 step;
		step.setMul( direction, distance );
		mDestination.setAdd( self, step );
		return true;
	}
	return false;
}

ZombieChase::Mode ZombieChase::mode() const {
	return mMode;
}

bool ZombieChase::hasDestination() const {
	return mMode != Mode::Wander || mWalking;
}

const Vector3& ZombieChase::destination() const {
	return mDestination;
}

bool ZombieChase::wantsToWalk() const {
	return mWalking;
}
