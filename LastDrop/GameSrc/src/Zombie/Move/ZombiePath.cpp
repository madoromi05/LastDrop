#include "GameSrc/Zombie/Move/ZombiePath.h"
#include "GameSrc/Zombie/Move/ZombieCrowdParams.h"
#include "GameSrc/Map/NavMesh.h"
#include "GameSrc/Map/PathBudget.h"
#include "GameSrc/Core/GameTime.h"
#include "GameSrc/Math/FlatMath.h"
#include "GameSrc/Math/AngleMath.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

namespace{
	// まっすぐ通れるかを調べる間隔（秒）。角を曲がった後、これだけ古い向きで歩くことがある
	const float kProbeIntervalSeconds = 0.5f;
	const int kProbeIntervalTicks = GameTime::secondsToTicks( kProbeIntervalSeconds );
	// 角の点にこれより近づいたら、二等分線を越えていなくても次の点へ（m。水平だけ）
	// 折り返す角は二等分線が決まらないので、近さだけで見る
	const float kCornerReachDistance = 0.3f;
	const float kSideDegree = 90.f;				// 真横（度。向かう点の向きから）。斜め前へよけられないときに使う
	const float kReplanDistance = 3.f;			// 行き先が、通り道を探したときからこれより動いたら探し直す（m）。追っている自キャラが歩き回るたびに探さないように
	const float kDestinationJumpDistance = 1.f;	// 行き先が 1 ティックでこれより動いたら、別の所へ向かい直したとみなす（m）。走る自キャラでも 1 ティックに約 0.05m
	// 自分が向く点の向きへこの長さ（m）進むたびに「進めている」とみなす（うろつきと同じ値）
	// 点との距離ではなく自分の動きで見るのは、逃げる自キャラ（ゾンビより速い）を追う間、まっすぐ歩けていても
	// 距離が縮まらず詰まったとみなされ、横へよけてしまうため。向く点の向きだけを数えるので、壁に沿って滑るだけの動きは数えない
	const float kProgressDistance = 0.3f;
	const float kMinDirectionLength = 0.001f;	// これより短い水平の長さは、向きが決まらないとみなす（m）
}

ZombiePath::ZombiePath( int probePhase, PathBudget* budget ) :
	mBudget( budget ),
	mNext( 0 ),
	mRouteStart( 0.f, 0.f, 0.f ),
	mRouteGoal( 0.f, 0.f, 0.f ),
	mLastDestination( 0.f, 0.f, 0.f ),
	mHasLastDestination( false ),
	mProbeTicks( ( probePhase >= 0 ? probePhase : -probePhase ) % kProbeIntervalTicks ),
	mProgressStarted( false ),
	mProgressOrigin( 0.f, 0.f, 0.f ),
	mStallTicks( 0 ),
	mSidestepPoint( 0.f, 0.f, 0.f ),
	mSidestepTicks( 0 ),
	mSidestepCount( 0 ),
	mSidestepsStarted( 0 ),
	mStuck( false ) {
	ASSERT( budget && "ZombiePath : budget is null." );
}

ZombiePath::~ZombiePath() {
	mBudget->cancel( this ); // 待っている間に消えても、列に残らないように
}

Vector3 ZombiePath::steer( const Vector3& self, const Vector3& destination, const NavMesh& navMesh,
bool walking ) {
	const ZombieCrowdParams& params = zombieCrowdParams();
	if ( mHasLastDestination && FlatMath::distance( mLastDestination, destination ) > kDestinationJumpDistance ) {
		mProbeTicks = 0; // 新しい行き先へ通れるかを、このティックに調べる（古い通り道は探し直すか捨てる）
		mSidestepTicks = 0;
		resetProgress();
	}
	mLastDestination = destination;
	mHasLastDestination = true;
	// よけている間は、よける点へ着くか決まった時間がたつまで、そちらを向く。終わったらこのティックに通り道を調べ直す
	if ( mSidestepTicks > 0 ) {
		--mSidestepTicks;
		if ( mSidestepTicks > 0 && FlatMath::distance( self, mSidestepPoint ) > params.sidestepReach ) {
			return mSidestepPoint;
		}
		mSidestepTicks = 0;
		mProbeTicks = 0;
		restartProgress();
	}
	if ( --mProbeTicks <= 0 ) {
		mProbeTicks = kProbeIntervalTicks;
		probe( self, destination, navMesh );
	}
	if ( !mRoute.empty() ) {
		mRoute.back() = destination; // 最後の点は、動いていく行き先（自キャラ）に付いていく
		int last = static_cast< int >( mRoute.size() ) - 1;
		while ( mNext < last && hasPassedCorner( self ) ) {
			++mNext;
			resetProgress();
		}
	}

	Vector3 point = aimPoint( self, destination );
	if ( !walking ) {
		mProgressStarted = false; // 立ち止まっている間は数えない（歩き出したら、その時の距離から数え直す）
		return point;
	}
	if ( !mProgressStarted ) {
		mProgressStarted = true;
		mProgressOrigin = self;
		mStallTicks = 0;
	} else {
		// 数え始めた所から動いた分のうち、向く点の向きへ進んだ長さ（水平だけ）。点が近すぎて向きが無ければ進めているとみなす
		Vector3 moved( self.x - mProgressOrigin.x, 0.f, self.z - mProgressOrigin.z );
		Vector3 toPoint( point.x - self.x, 0.f, point.z - self.z );
		float length = toPoint.length();
		if ( length < kMinDirectionLength || moved.dot( toPoint ) >= kProgressDistance * length ) {
			mProgressOrigin = self;
			mStallTicks = 0;
		} else {
			++mStallTicks;
		}
	}
	if ( mStallTicks >= GameTime::secondsToTicks( params.sidestepAfterSeconds ) ) {
		// 一度よけた後にまた詰まったら、着けない所かもしれない印を付ける（Search ならあきらめる。Chase はよけ続ける）
		if ( mSidestepCount > 0 ) {
			mStuck = true;
		}
		startSidestep( self, point, navMesh );
		return mSidestepPoint;
	}
	return point;
}

void ZombiePath::startSidestep( const Vector3& self, const Vector3& point, const NavMesh& navMesh ) {
	const ZombieCrowdParams& params = zombieCrowdParams();
	float heading = AngleMath::headingOf( Vector3( point.x - self.x, 0.f, point.z - self.z ) ); // 向く点が真上なら +Z を前とみなす
	if ( !pickSidestep( self, point, heading, params.sidestepAngle, navMesh ) && !pickSidestep( self, point, heading, kSideDegree, navMesh ) ) {
		mSidestepPoint.setMadd( self, AngleMath::forwardOf( heading ), -params.backstepDistance ); // 左右とも塞がっている。少し下がる
	}
	mSidestepTicks = GameTime::secondsToTicks( params.sidestepSeconds );
	++mSidestepCount;
	++mSidestepsStarted;
	mStallTicks = 0;
}

bool ZombiePath::pickSidestep( const Vector3& self, const Vector3& point, float heading, float degree, const NavMesh& navMesh ) {
	float distance = zombieCrowdParams().sidestepDistance;
	Vector3 pointA;
	pointA.setMadd( self, AngleMath::forwardOf( heading + degree ), distance );
	Vector3 pointB;
	pointB.setMadd( self, AngleMath::forwardOf( heading - degree ), distance );
	bool openA = navMesh.isWalkable( self, pointA );
	bool openB = navMesh.isWalkable( self, pointB );
	if ( openA && openB ) {
		mSidestepPoint = ( FlatMath::distance( pointA, point ) <= FlatMath::distance( pointB, point ) ) ? pointA : pointB;
	} else if ( openA ) {
		mSidestepPoint = pointA;
	} else if ( openB ) {
		mSidestepPoint = pointB;
	} else {
		return false;
	}
	return true;
}

void ZombiePath::probe( const Vector3& self, const Vector3& destination, const NavMesh& navMesh ) {
	if ( navMesh.isWalkable( self, destination ) ) {
		if ( !mRoute.empty() ) {
			mRoute.clear();
			resetProgress();
		}
		return;
	}
	if ( mRoute.empty() || FlatMath::distance( mRouteGoal, destination ) > kReplanDistance ) {
		// 今のティックに探す回数が残っていなければ、前の通り道のまま歩き、次のティックにもう一度調べる（間隔を待たない）
		if ( !mBudget->tryAcquire( this ) ) {
			mProbeTicks = 1;
			return;
		}
		// 見つからなければ（通れる交差点が無い等）空のまま＝まっすぐ向かい、壁に沿って滑る。着けなければ isStuck になる
		navMesh.findPath( self, destination, &mRoute );
		mNext = 0;
		mRouteStart = self;
		mRouteGoal = destination;
		resetProgress();
		return;
	}
	// 次の次の点が見えていれば、今の点は飛ばす（角を斜めに曲がる）
	if ( mNext + 1 < static_cast< int >( mRoute.size() ) && navMesh.isWalkable( self, mRoute[ mNext + 1 ] ) ) {
		++mNext;
		resetProgress();
	}
}

void ZombiePath::clear() {
	mBudget->cancel( this );
	mRoute.clear();
	mNext = 0;
	mHasLastDestination = false;
	mProbeTicks = 0; // 次の steer ですぐ調べる
	mSidestepTicks = 0;
	resetProgress();
}

bool ZombiePath::isStuck() const {
	return mStuck;
}

bool ZombiePath::isWaitingForSearch() const {
	return mBudget->isWaiting( this );
}

int ZombiePath::sidestepsStarted() const {
	return mSidestepsStarted;
}

void ZombiePath::remainingRoute( std::vector< Vector3 >* out ) const {
	ASSERT( out && "ZombiePath : out is null." );
	out->clear();
	if ( mSidestepTicks > 0 ) {
		out->push_back( mSidestepPoint );
	}
	for ( int i = mNext; i < static_cast< int >( mRoute.size() ); ++i ) {
		out->push_back( mRoute[ i ] );
	}
}

Vector3 ZombiePath::aimPoint( const Vector3& self, const Vector3& destination ) const {
	if ( mRoute.empty() ) {
		return destination;
	}
	// 自分 → 今の角 → その先の角…と通り道をたどり、決まった長さだけ進んだ所を返す
	// 角が近づくと点が角の向こうへ回り込むので、手前から曲がり始める
	float remaining = zombieCrowdParams().lookaheadDistance;
	Vector3 from = self;
	for ( int i = mNext; i < static_cast< int >( mRoute.size() ); ++i ) {
		const Vector3& to = mRoute[ i ];
		float length = FlatMath::distance( from, to );
		if ( length > remaining ) {
			Vector3 step;
			step.setSub( to, from );
			Vector3 point;
			point.setMadd( from, step, remaining / length );
			return point;
		}
		remaining -= length;
		from = to;
	}
	return mRoute.back(); // 通り道が先読みの長さより短い。行き先そのもの
}

bool ZombiePath::hasPassedCorner( const Vector3& self ) const {
	const Vector3& corner = mRoute[ mNext ];
	if ( FlatMath::distance( self, corner ) <= kCornerReachDistance ) {
		return true;
	}
	// 入ってくる向きと出ていく向き（どちらも長さ 1）の和が、角の二等分線の前の向き。角から見てその向きの側に居れば越えた
	// 近さで決めると、先を向いて角の内側を回る体が角へ近づかないまま、角の手前で次の角を向いて内側へ切り込むため
	const Vector3& previous = ( mNext > 0 ) ? mRoute[ mNext - 1 ] : mRouteStart;
	const Vector3& next = mRoute[ mNext + 1 ];
	Vector3 incoming( corner.x - previous.x, 0.f, corner.z - previous.z );
	Vector3 outgoing( next.x - corner.x, 0.f, next.z - corner.z );
	float incomingLength = incoming.length();
	float outgoingLength = outgoing.length();
	if ( incomingLength < kMinDirectionLength || outgoingLength < kMinDirectionLength ) {
		return false;
	}
	Vector3 bisector;
	bisector.setMul( incoming, 1.f / incomingLength );
	Vector3 outgoingDirection;
	outgoingDirection.setMul( outgoing, 1.f / outgoingLength );
	bisector += outgoingDirection;
	if ( bisector.length() < kMinDirectionLength ) {
		return false;
	}
	Vector3 fromCorner( self.x - corner.x, 0.f, self.z - corner.z );
	return fromCorner.dot( bisector ) >= 0.f;
}

void ZombiePath::resetProgress() {
	restartProgress();
	mSidestepCount = 0;
	mStuck = false;
}

void ZombiePath::restartProgress() {
	mProgressStarted = false;
	mStallTicks = 0;
}
