#include "GameSrc/Zombie/ZombieSway.h"
#include "GameSrc/Zombie/ZombieFlinchLayout.h"
#include "GameSrc/Core/GameTime.h"
#include "GameSrc/Math/ScalarMath.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Functions.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix34.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const float kFullTurn = 360.f; // 波の 1 往復（度。GameLib の sin は度で受ける）
	// ゆっくりの波（よろめき）の長さは、ふだんの揺れの何倍か。整数の比にしないことで、2 つの波が重なる時がたまにしか来ない。
	const float kLurchPeriodRatio = 2.7f;
	// 2 つの波の割合（足すと 1）。ふだんは最大の 7 割ほどで揺れ、2 つが重なったときだけ最大まで傾く。
	const float kMainShare = 0.7f;
	const float kLurchShare = 0.3f;
	// 強さを 0 から 1 まで（1 から 0 まで）寄せ切る長さ（秒）。追うのをやめたら、これだけかけて揺れが収まる。
	const float kFadeSeconds = 0.5f;
	const float kFadePerTick = 1.f / ( kFadeSeconds * GameTime::kTicksPerSecond );
	// これより小さい角度（度）は曲げない（揺れていない体で毎回計算しないため）。
	const float kNegligibleDegree = 0.01f;
	// 倒す向き（骨格の根の空間の横。体の向き 0 が +Z なので、横は +X）。
	const Vector3 kSideDirection( 1.f, 0.f, 0.f );
	const float kLurchStartOffset = 0.5f; // ゆっくりの波は、ふだんの揺れと半周ずらして始める（始めからいきなり大きく傾かない）
}

ZombieSway::ZombieSway( const ZombieFlinchLayout* layout, float periodSeconds, float startPhase ) :
	mLayout( layout ),
	mGroup( layout ? layout->groupOf( BodyPart::Torso ) : 0 ),
	mPhaseStep( 1.f / ( periodSeconds * GameTime::kTicksPerSecond ) ),
	mPhase( ScalarMath::wrapUnit( startPhase ) ),
	mLurchPhase( ScalarMath::wrapUnit( startPhase + kLurchStartOffset ) ),
	mStrength( 0.f ),
	mLean( 0.f ),
	mDegree( 0.f ) {
	ASSERT( mLayout && "ZombieSway : layout is null." );
	ASSERT( periodSeconds > 0.f && "ZombieSway : period must be positive." );
}

void ZombieSway::update( float targetStrength, float maxDegree ) {
	float target = ScalarMath::clamp( targetStrength, 0.f, 1.f );
	mStrength += ScalarMath::clampSigned( target - mStrength, kFadePerTick );
	mPhase = ScalarMath::wrapUnit( mPhase + mPhaseStep );
	mLurchPhase = ScalarMath::wrapUnit( mLurchPhase + mPhaseStep / kLurchPeriodRatio );
	float wave = kMainShare * Math::sin( kFullTurn * mPhase ) + kLurchShare * Math::sin( kFullTurn * mLurchPhase );
	mLean = wave * mStrength;
	mDegree = mLean * maxDegree;
}

float ZombieSway::lean() const {
	return mLean;
}

void ZombieSway::apply( std::vector< Matrix34 >* pose ) {
	if ( Math::abs( mDegree ) < kNegligibleDegree ) {
		return;
	}
	// 負の角度は、反対の横へ倒す。
	Vector3 direction = kSideDirection;
	if ( mDegree < 0.f ) {
		direction *= -1.f;
	}
	mLayout->bend( mGroup, direction, Math::abs( mDegree ), pose );
}
