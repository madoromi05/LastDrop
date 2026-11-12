#include "GameSrc/Zombie/Move/ZombiePath.h"
#include "GameSrc/Zombie/Move/ZombieCrowdParams.h"
#include "GameSrc/Map/NavMesh.h"
#include "GameSrc/Map/PathBudget.h"
#include "GameSrc/Core/GameTime.h"
#include "GameSrc/Math/FlatMath.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

namespace{
	// まっすぐ通れるかを調べる間隔（秒）。角を曲がった後、これだけ古い向きで歩くことがある
	const float kProbeIntervalSeconds = 0.5f;
	const int kProbeIntervalTicks = GameTime::secondsToTicks( kProbeIntervalSeconds );
	// 通り道の点にこれより近づいたら、次の点へ（m。水平だけ）
	const float kWaypointReachDistance = 1.f;
	// 行き先が、通り道を探したときからこれより動いたら探し直す（m）。追っている自キャラが歩き回るたびに探さないように
	const float kReplanDistance = 3.f;
	// 行き先が 1 ティックでこれより動いたら、別の所へ向かい直したとみなす（m）。走る自キャラでも 1 ティックに約 0.05m
	const float kDestinationJumpDistance = 1.f;
	// 自分が向く点の向きへこの長さ（m）進むたびに「進めている」とみなす（うろつきと同じ値）
	// 点との距離ではなく自分の動きで見るのは、逃げる自キャラ（ゾンビより速い）を追う間、まっすぐ歩けていても
	// 距離が縮まらず詰まったとみなされ、横へよけてしまうため。向く点の向きだけを数えるので、壁に沿って滑るだけの動きは数えない
	const float kProgressDistance = 0.3f;
	// 向く点への向きが決まらないほど近いとき（m）。よける向きは前の向きを使えないので、+Z を前とみなす
	const float kMinDirectionLength = 0.001f;
}

ZombiePath::ZombiePath( int probePhase, PathBudget* budget ) :
	mBudget( budget ),
	mNext( 0 ),
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
	mStuck( false ) {
	ASSERT( budget && "ZombiePath : budget is null." );
}

ZombiePath::~ZombiePath() {
	mBudget->cancel( this ); // 待っている間に消えても、列に残らないように
}

const Vector3& ZombiePath::steer( const Vector3& self, const Vector3& destination, const NavMesh& navMesh,
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
		while ( mNext < last && FlatMath::distance( self, mRoute[ mNext ] ) <= kWaypointReachDistance ) {
			++mNext;
			resetProgress();
		}
	}

	const Vector3& point = currentPoint( destination );
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
	Vector3 forward( point.x - self.x, 0.f, point.z - self.z );
	float length = forward.length();
	if ( length < kMinDirectionLength ) {
		forward.set( 0.f, 0.f, 1.f );
	} else {
		forward *= 1.f / length;
	}
	// 前への向きを左右へ 90° 回した 2 つ（水平）。どちらが左かは問わない（通れる方・近い方で選ぶ）
	Vector3 sideA( forward.z, 0.f, -forward.x );
	Vector3 sideB( -forward.z, 0.f, forward.x );
	Vector3 pointA;
	pointA.setMadd( self, sideA, params.sidestepDistance );
	Vector3 pointB;
	pointB.setMadd( self, sideB, params.sidestepDistance );
	bool openA = navMesh.isWalkable( self, pointA );
	bool openB = navMesh.isWalkable( self, pointB );
	if ( openA && openB ) {
		mSidestepPoint = ( FlatMath::distance( pointA, point ) <= FlatMath::distance( pointB, point ) ) ? pointA : pointB;
	} else if ( openA ) {
		mSidestepPoint = pointA;
	} else if ( openB ) {
		mSidestepPoint = pointB;
	} else {
		mSidestepPoint.setMadd( self, forward, -params.backstepDistance ); // 左右とも塞がっている。少し下がる
	}
	mSidestepTicks = GameTime::secondsToTicks( params.sidestepSeconds );
	++mSidestepCount;
	mStallTicks = 0;
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

const Vector3& ZombiePath::currentPoint( const Vector3& destination ) const {
	return mRoute.empty() ? destination : mRoute[ mNext ];
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
