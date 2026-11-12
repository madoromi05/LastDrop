#include "GameSrc/Zombie/ZombieSpawner.h"
#include "GameSrc/Hit/HitWorld.h"
#include "GameSrc/Collision/CollisionWorld.h"
#include "GameSrc/Collision/BodyFootprint.h"
#include "GameSrc/Core/GameTime.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Functions.h"
#include <utility>

using namespace GameLib::Math;

namespace{
	// 立っているゾンビの目標の数は、作るときに受け取る（日の条件で変わる。DayConditionEffect::zombieCount）
	// 減ったら、これだけ待って 1 体補う（秒）。倒した直後に湧かないように
	const float kRefillSeconds = 10.f;
	const int kRefillTicks = GameTime::secondsToTicks( kRefillSeconds );

	// 自キャラからこれより近い所には湧かせない（m。水平）。目の前に急に出ないように
	const float kMinDistance = 25.f;
	// 画面の内とみなす範囲（度。カメラの水平の向きから左右それぞれ）。画面の横の端（約 46°）より、振り向き始めの分だけ広く取る
	const float kViewHalfAngle = 60.f;
	// 見えるかを調べる、ゾンビの頭の高さ（m。足元から）。頭が見えなければ体も見えないとみなす
	const float kHeadHeight = 1.7f;
	// 補充のとき 1 ティックに調べる候補の数。見つからなければ次のティックに続きを調べる（1 ティックに線を何本も引かない）
	const int kTriesPerTick = 4;
	const float kFullTurn = 360.f;
}

ZombieSpawner::ZombieSpawner( const std::vector< Vector3 >& candidates, const HitWorld* hitWorld,
const CollisionWorld* collisionWorld, const BodyFootprint* footprint, int randomSeed, int targetCount ) :
	mCandidates( candidates ),
	mHitWorld( hitWorld ),
	mCollisionWorld( collisionWorld ),
	mFootprint( footprint ),
	mRandom( Random::create( randomSeed ) ),
	mPlacedInitial( false ),
	mRefillTicks( kRefillTicks ),
	mTargetCount( targetCount ),
	mStandingCount( 0 ) {
	ASSERT( hitWorld && collisionWorld && footprint && "ZombieSpawner : world is null." );
	ASSERT( targetCount > 0 && "ZombieSpawner : targetCount must be positive." );
}

void ZombieSpawner::update( const View& view, std::vector< Order >* out ) {
	ASSERT( out && "ZombieSpawner : out is null." );
	mStandingCount = view.standingZombies;
	if ( mCandidates.empty() ) {
		return;
	}
	if ( !mPlacedInitial ) {
		mPlacedInitial = true;
		planInitial( view, out );
		return;
	}
	planRefill( view, out );
}

void ZombieSpawner::planInitial( const View& view, std::vector< Order >* out ) {
	// 候補を混ぜて（Fisher–Yates）、湧いてよい所から順に目標の数まで取る。足りなければ、残りは補充が埋める
	std::vector< int > order( mCandidates.size() );
	for ( size_t i = 0; i < order.size(); ++i ) {
		order[ i ] = static_cast< int >( i );
	}
	for ( int i = static_cast< int >( order.size() ) - 1; i > 0; --i ) {
		std::swap( order[ i ], order[ mRandom.getInt( i + 1 ) ] );
	}
	int remaining = mTargetCount - view.standingZombies;
	for ( int index : order ) {
		if ( remaining <= 0 ) {
			break;
		}
		const Vector3& position = mCandidates[ index ];
		float yaw = mRandom.getFloat( kFullTurn );
		if ( canSpawnAt( view, position, yaw ) ) {
			out->push_back( { position, yaw } );
			--remaining;
		}
	}
}

void ZombieSpawner::planRefill( const View& view, std::vector< Order >* out ) {
	if ( view.standingZombies >= mTargetCount ) {
		mRefillTicks = kRefillTicks; // 足りている間は数えない（減ってから kRefillSeconds 後に湧く）
		return;
	}
	if ( mRefillTicks > 0 ) {
		--mRefillTicks;
		return;
	}
	if ( !view.hasRoom ) {
		return; // 全体の上限（死体も数える）。死体が沈んで消えるのを待つ
	}
	for ( int i = 0; i < kTriesPerTick; ++i ) {
		const Vector3& position = mCandidates[ mRandom.getInt( static_cast< int >( mCandidates.size() ) ) ];
		float yaw = mRandom.getFloat( kFullTurn );
		if ( canSpawnAt( view, position, yaw ) ) {
			out->push_back( { position, yaw } );
			mRefillTicks = kRefillTicks;
			return;
		}
	}
	// 見つからなかった。残り 0 のまま、次のティックに別の候補を調べる
}

bool ZombieSpawner::canSpawnAt( const View& view, const Vector3& position, float yawDegree ) const {
	Vector3 toPosition;
	toPosition.setSub( position, view.playerEye );
	toPosition.y = 0.f;
	if ( toPosition.squareLength() < kMinDistance * kMinDistance ) {
		return false;
	}
	Vector3 head( position.x, position.y + kHeadHeight, position.z );
	if ( isVisible( view, head ) ) {
		return false;
	}
	// 体の長方形が、建物・車・キャラ・落ちている物にめり込むなら出さない
	Vector3 push;
	return !mCollisionWorld->findPenetration( mFootprint->place( position, yawDegree ), nullptr, &push );
}

bool ZombieSpawner::isVisible( const View& view, const Vector3& head ) const {
	Vector3 toHead;
	toHead.setSub( head, view.playerEye );
	// 画面の外なら見えない（重い線より先に、向きで外す）
	Vector3 flat( toHead.x, 0.f, toHead.z );
	Vector3 forward( GameLib::Math::sin( view.viewYaw ), 0.f, GameLib::Math::cos( view.viewYaw ) );
	if ( forward.dot( flat ) < GameLib::Math::cos( kViewHalfAngle ) * flat.length() ) {
		return false;
	}
	// 画面の内でも、建物・車・地面の陰なら見えない
	return !mHitWorld->isBlocked( view.playerEye, toHead );
}

void ZombieSpawner::setTargetCount( int targetCount ) {
	ASSERT( targetCount > 0 && "ZombieSpawner::setTargetCount : targetCount must be positive." );
	mTargetCount = targetCount;
}

int ZombieSpawner::targetCount() const {
	return mTargetCount;
}

int ZombieSpawner::standingCount() const {
	return mStandingCount;
}

float ZombieSpawner::secondsUntilNext() const {
	return mRefillTicks * GameTime::kTickSeconds;
}
