#include "GameSrc/Zombie/ZombieFlinch.h"
#include "GameSrc/Zombie/ZombieFlinchLayout.h"
#include "GameSrc/Core/GameTime.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Functions.h"
#include "GameLib/Math/Matrix34.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// 1 まとまりを曲げる角度の上限（度）。フルオートで当て続けても曲がりすぎないように。
	const float kMaxDegree = 40.f;
	// 押した分が戻るまでの時間（秒）と、そのとき残っている割合。毎ティック同じ割合を掛けて、滑らかに減らす（行き過ぎない）。
	const float kReturnSeconds = 0.25f;
	const float kRemainAfterReturn = 0.05f;
	const float kDecayPerTick = Math::pow( kRemainAfterReturn, 1.f / ( kReturnSeconds * GameTime::kTicksPerSecond ) );
	// これより小さい角度（度）は 0 とみなして曲げない（戻りきった後に毎回計算しないため）。
	const float kNegligibleDegree = 0.1f;
}

ZombieFlinch::ZombieFlinch( const ZombieFlinchLayout* layout ) :
	mLayout( layout ),
	mPushes( layout->groups().size(), Vector3( 0.f, 0.f, 0.f ) ) {
	ASSERT( mLayout && "ZombieFlinch : layout is null." );
}

void ZombieFlinch::push( BodyPart part, const Vector3& direction ) {
	int group = mLayout->groupOf( part );
	Vector3& pushed = mPushes[ group ];
	pushed.madd( direction, mLayout->groups()[ group ].kickDegree );
	float degree = pushed.length();
	if ( degree > kMaxDegree ) {
		pushed *= kMaxDegree / degree;
	}
}

void ZombieFlinch::update() {
	for ( Vector3& pushed : mPushes ) {
		pushed *= kDecayPerTick;
	}
}

void ZombieFlinch::apply( std::vector< Matrix34 >* pose ) {
	for ( size_t g = 0; g < mPushes.size(); ++g ) {
		float degree = mPushes[ g ].length();
		if ( degree < kNegligibleDegree ) {
			continue;
		}
		Vector3 pushDirection = mPushes[ g ];
		pushDirection *= 1.f / degree;
		mLayout->bend( static_cast< int >( g ), pushDirection, degree, pose );
	}
}
