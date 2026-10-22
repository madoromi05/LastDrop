#include "GameSrc/Zombie/ZombiePath.h"
#include "GameSrc/Map/StreetGraph.h"
#include "GameSrc/Core/GameTime.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

namespace{
	// まっすぐ通れるかを調べる間隔（秒）。角を曲がった後、これだけ古い向きで歩くことがある。
	const float kProbeIntervalSeconds = 0.5f;
	const int kProbeIntervalTicks = static_cast< int >( kProbeIntervalSeconds * GameTime::kTicksPerSecond );
	// 通り道の点にこれより近づいたら、次の点へ（m。水平だけ）。
	const float kWaypointReachDistance = 1.f;
	// 行き先が、通り道を探したときからこれより動いたら探し直す（m）。追っている自キャラが歩き回るたびに探さないように。
	const float kReplanDistance = 3.f;
	// 行き先が 1 ティックでこれより動いたら、別の所へ向かい直したとみなす（m）。走る自キャラでも 1 ティックに約 0.05m。
	const float kDestinationJumpDistance = 1.f;
	// この長さ（m）近づくたびに「近づけている」とみなす。壁に沿って滑るだけで寄れていないのを見分ける（うろつきと同じ値）。
	const float kProgressDistance = 0.3f;
	// 近づけないまま、これだけ過ぎたら「着けない」（秒。うろつきと同じ値）。
	const float kStallSeconds = 2.f;
	const int kStallTicks = static_cast< int >( kStallSeconds * GameTime::kTicksPerSecond );

	// a と b の水平の距離（m）。
	float flatDistance( const Vector3& a, const Vector3& b ) {
		Vector3 d;
		d.setSub( b, a );
		d.y = 0.f;
		return d.length();
	}
}

ZombiePath::ZombiePath( int probePhase ) :
	mNext( 0 ),
	mRouteGoal( 0.f, 0.f, 0.f ),
	mLastDestination( 0.f, 0.f, 0.f ),
	mHasLastDestination( false ),
	mProbeTicks( ( probePhase >= 0 ? probePhase : -probePhase ) % kProbeIntervalTicks ),
	mProgressStarted( false ),
	mBestDistance( 0.f ),
	mStallTicks( 0 ) {
}

const Vector3& ZombiePath::steer( const Vector3& self, const Vector3& destination, const StreetGraph& streets,
const HitWorld& hitWorld ) {
	if ( mHasLastDestination && flatDistance( mLastDestination, destination ) > kDestinationJumpDistance ) {
		mProbeTicks = 0; // 新しい行き先へ通れるかを、このティックに調べる（古い通り道は探し直すか捨てる）
		resetProgress();
	}
	mLastDestination = destination;
	mHasLastDestination = true;
	if ( --mProbeTicks <= 0 ) {
		mProbeTicks = kProbeIntervalTicks;
		probe( self, destination, streets, hitWorld );
	}
	if ( !mRoute.empty() ) {
		mRoute.back() = destination; // 最後の点は、動いていく行き先（自キャラ）に付いていく
		int last = static_cast< int >( mRoute.size() ) - 1;
		while ( mNext < last && flatDistance( self, mRoute[ mNext ] ) <= kWaypointReachDistance ) {
			++mNext;
			resetProgress();
		}
	}

	const Vector3& point = currentPoint( destination );
	float distance = flatDistance( self, point );
	if ( !mProgressStarted ) {
		mProgressStarted = true;
		mBestDistance = distance;
		mStallTicks = 0;
	} else if ( distance < mBestDistance - kProgressDistance ) {
		mBestDistance = distance;
		mStallTicks = 0;
	} else {
		++mStallTicks;
	}
	return point;
}

void ZombiePath::probe( const Vector3& self, const Vector3& destination, const StreetGraph& streets, const HitWorld& hitWorld ) {
	if ( streets.isWalkable( self, destination, hitWorld ) ) {
		if ( !mRoute.empty() ) {
			mRoute.clear();
			resetProgress();
		}
		return;
	}
	if ( mRoute.empty() || flatDistance( mRouteGoal, destination ) > kReplanDistance ) {
		// 見つからなければ（通れる交差点が無い等）空のまま＝まっすぐ向かい、壁に沿って滑る。着けなければ isStuck になる。
		streets.findPath( self, destination, hitWorld, &mRoute );
		mNext = 0;
		mRouteGoal = destination;
		resetProgress();
		return;
	}
	// 次の次の点が見えていれば、今の点は飛ばす（角を斜めに曲がる）。
	if ( mNext + 1 < static_cast< int >( mRoute.size() ) && streets.isWalkable( self, mRoute[ mNext + 1 ], hitWorld ) ) {
		++mNext;
		resetProgress();
	}
}

void ZombiePath::clear() {
	mRoute.clear();
	mNext = 0;
	mHasLastDestination = false;
	mProbeTicks = 0; // 次の steer ですぐ調べる
	resetProgress();
}

bool ZombiePath::isStuck() const {
	return mStallTicks >= kStallTicks;
}

void ZombiePath::remainingRoute( std::vector< Vector3 >* out ) const {
	ASSERT( out && "ZombiePath : out is null." );
	out->clear();
	for ( int i = mNext; i < static_cast< int >( mRoute.size() ); ++i ) {
		out->push_back( mRoute[ i ] );
	}
}

const Vector3& ZombiePath::currentPoint( const Vector3& destination ) const {
	return mRoute.empty() ? destination : mRoute[ mNext ];
}

void ZombiePath::resetProgress() {
	mProgressStarted = false;
	mStallTicks = 0;
}
