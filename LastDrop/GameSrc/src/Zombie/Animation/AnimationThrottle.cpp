#include "GameSrc/Zombie/Animation/AnimationThrottle.h"
#include "GameSrc/Zombie/Animation/AnimationThrottleParams.h"
#include "GameSrc/Render/Culling/DrawCulling.h"
#include "GameSrc/Hit/HitboxSet.h"
#include "GameSrc/Map/TownVisibility.h"

#include "GameLib/GameLib.h"
#include "GameLib/Profiling/Profiler.h"

using namespace GameLib::Math;

namespace{
	// 間引く段の値（F6 で動かせる。毎ティック読むので、変えたらすぐ効く）。
	AnimationThrottleParams gThrottleParams;

	const unsigned kMinInterval = 1; // 毎ティック

	// 小数で持つ間隔（ティック）を、四捨五入して 1 以上の整数にする。
	unsigned intervalOf( float ticks ) {
		const float kRound = 0.5f;
		int rounded = static_cast< int >( ticks + kRound );
		return ( rounded < static_cast< int >( kMinInterval ) ) ? kMinInterval : static_cast< unsigned >( rounded );
	}
}

AnimationThrottleParams& animationThrottleParams() {
	return gThrottleParams;
}

AnimationThrottle::AnimationThrottle( unsigned phase ) :
	mPhase( phase ),
	mTick( 0 ),
	mHasPosed( false ),
	mLevel( Level::Near ) {
}

AnimationThrottle::Level AnimationThrottle::judge( const AnimationView* view, const HitboxSet& hitboxes, const Vector3& position ) {
	Vector3 boundsMin;
	Vector3 boundsMax;
	if ( !view || !hitboxes.bounds( &boundsMin, &boundsMax ) ) {
		return Level::Near; // まだ描いていない・箱が置かれていない間は間引かない
	}
	// 描くかを決める箱（DrawCulling::showsPosed）と同じ広げ方にする（描く体を「映らない」段にしないため）。
	Vector3 margin( DrawCulling::kPoseMargin );
	boundsMin -= margin;
	boundsMax += margin;
	if ( !view->frustum.mayContain( boundsMin, boundsMax ) ) {
		return Level::Hidden;
	}
	// 描く時と同じく、建物の向こうに隠れた体も映らない（DrawCulling::showsPosed と同じ足元で聞く）。
	if ( view->visibility && !view->visibility->showsPoint( view->viewCell, position ) ) {
		return Level::Hidden;
	}
	Vector3 toBody;
	toBody.setSub( position, view->eye );
	float farDistance = gThrottleParams.farDistance;
	return ( toBody.squareLength() >= farDistance * farDistance ) ? Level::Far : Level::Near;
}

bool AnimationThrottle::shouldPose( Level level, bool mustPose ) {
	mLevel = level;
	++mTick;
	unsigned interval = kMinInterval;
	if ( level == Level::Far ) {
		interval = intervalOf( gThrottleParams.farIntervalTicks );
	} else if ( level == Level::Hidden ) {
		interval = intervalOf( gThrottleParams.hiddenIntervalTicks );
	}
	// 間引かない間が終わった後も、体ごとのずれ（mPhase）は保ったままの刻みに戻る。
	bool poses = !mHasPosed || mustPose || ( ( mTick + mPhase ) % interval == 0 );
	if ( !poses ) {
		PROFILE_COUNT( "Anim.skipped", 1 ); // 間引いた回数
		return false;
	}
	mHasPosed = true;
	return true;
}

AnimationThrottle::Level AnimationThrottle::level() const {
	return mLevel;
}
