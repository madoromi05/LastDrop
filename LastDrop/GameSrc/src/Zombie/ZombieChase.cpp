#include "GameSrc/Zombie/ZombieChase.h"
#include "GameSrc/Zombie/ChaseTarget.h"
#include "GameSrc/Hit/HitWorld.h"
#include "GameSrc/Core/GameTime.h"

using namespace GameLib::Math;

namespace{
	// ── 気付く（見積もりの初期値。SummaryPlan/個別仕様/ZombieChase_Plan.txt の 2 章）──
	const float kSightRange = 50.f; // これより遠い自キャラには気付かない（m。目から目まで）
	// 見通しの線を引く間隔（秒）。気付くのがこれだけ遅れてもよい長さ。
	const float kSightIntervalSeconds = 0.2f;
	const int kSightIntervalTicks = static_cast< int >( kSightIntervalSeconds * GameTime::kTicksPerSecond );

	// ── 近づく──
	// 自キャラにこれより近づいたら止まる（m。足元どうし、水平だけ）。
	const float kStopDistance = 1.2f;
	// 止まった後、自キャラがこれより離れたら歩き出す（止まる距離より少し長くして、境目で歩く・止まるを繰り返さない）。
	const float kResumeDistance = 1.5f;
	// 最後に見た場所にこれより近づいたら着いたとみなす（m）。
	const float kArriveDistance = 0.5f;

	// a と b の水平の距離（m）。
	float flatDistance( const Vector3& a, const Vector3& b ) {
		Vector3 d;
		d.setSub( b, a );
		d.y = 0.f;
		return d.length();
	}
}

ZombieChase::ZombieChase( int sightPhase ) :
	mMode( Mode::Idle ),
	mDestination( 0.f, 0.f, 0.f ),
	mWalking( false ),
	mSeesTarget( false ),
	mAlerted( false ),
	mSightTicks( ( sightPhase >= 0 ? sightPhase : -sightPhase ) % kSightIntervalTicks ) {
}

void ZombieChase::alert() {
	mAlerted = true;
}

void ZombieChase::update( const Vector3& self, const Vector3& eye, const ChaseTarget* target, const HitWorld& hitWorld ) {
	if ( !target ) {
		mMode = Mode::Idle; // 追う相手が居ない
		mWalking = false;
		mAlerted = false;
		return;
	}
	if ( mAlerted ) {
		mAlerted = false;
		mSeesTarget = true; // 撃たれた所（自キャラの居る所）は分かる。次に線を引いて見えなければ、そこを最後に見た場所にする
		mSightTicks = 0;
	} else if ( --mSightTicks <= 0 ) {
		mSightTicks = kSightIntervalTicks;
		mSeesTarget = canSee( eye, *target, hitWorld );
	}

	if ( mSeesTarget ) {
		mMode = Mode::Chase;
		mDestination = target->position; // 見えている間は、線を引かないティックも今の位置を追う
	} else if ( mMode == Mode::Chase ) {
		mMode = Mode::Search; // 最後に見た場所（mDestination）へ向かう
		mWalking = true;
	}
	updateWalking( self );
}

bool ZombieChase::canSee( const Vector3& eye, const ChaseTarget& target, const HitWorld& hitWorld ) const {
	Vector3 toTarget;
	toTarget.setSub( target.eyePosition, eye );
	if ( toTarget.length() > kSightRange ) {
		return false;
	}
	return !hitWorld.isBlocked( eye, toTarget );
}

void ZombieChase::updateWalking( const Vector3& self ) {
	float distance = flatDistance( self, mDestination );
	switch ( mMode ) {
		case Mode::Idle:
			mWalking = false;
			break;
		case Mode::Chase:
			// 止まっている間は、少し離れるまで歩き出さない。
			mWalking = mWalking ? ( distance > kStopDistance ) : ( distance > kResumeDistance );
			break;
		case Mode::Search:
			if ( distance <= kArriveDistance ) {
				mMode = Mode::Idle; // 着いた。見当たらないので、そこで立ち止まる
				mWalking = false;
			} else {
				mWalking = true;
			}
			break;
	}
}

ZombieChase::Mode ZombieChase::mode() const {
	return mMode;
}

bool ZombieChase::hasDestination() const {
	return mMode != Mode::Idle;
}

const Vector3& ZombieChase::destination() const {
	return mDestination;
}

bool ZombieChase::wantsToWalk() const {
	return mWalking;
}
