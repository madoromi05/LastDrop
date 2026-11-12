#include "GameSrc/Zombie/Damage/ZombieFlinch.h"
#include "GameSrc/Zombie/Damage/ZombieFlinchLayout.h"
#include "GameSrc/Zombie/Damage/ZombieStaggerParams.h"
#include "GameSrc/Core/GameTime.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Functions.h"
#include "GameLib/Math/Matrix34.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const float kMaxDegree = 60.f;			// 1 まとまりを曲げる角度の上限（度）。フルオートで当て続けても曲がりすぎないように
	// 押した分が戻るまでの時間（秒）と、そのとき残っている割合。毎ティック同じ割合を掛けて、滑らかに減らす（行き過ぎない）
	const float kReturnSeconds = 0.25f;
	const float kRemainAfterReturn = 0.05f;
	const float kDecayPerTick = Math::pow( kRemainAfterReturn, 1.f / ( kReturnSeconds * GameTime::kTicksPerSecond ) );
	const float kNegligibleDegree = 0.1f;	// これより小さい角度（度）は 0 とみなして曲げない（戻りきった後に毎回計算しないため）
	const int kLungeHeadPushes = 2;			// 噛むときに首を押す回数（1 回 20°なので 2 回で 40°。背骨は 1 回）
}

ZombieFlinch::ZombieFlinch( const ZombieFlinchLayout* layout ) :
	mLayout( layout ),
	mPushes( layout->groups().size(), Vector3( 0.f, 0.f, 0.f ) ) {
	ASSERT( mLayout && "ZombieFlinch : layout is null." );
}

void ZombieFlinch::push( BodyPart part, const Vector3& direction ) {
	float degree = mLayout->groups()[ mLayout->groupOf( part ) ].kickDegree;
	kick( part, direction, degree * zombieStaggerParams().flinchScale );
}

void ZombieFlinch::lunge( const Vector3& rootDirection ) {
	// 噛む突き出しは、撃たれたのけぞりとは別の動きなので倍率を掛けない（首 40°・背骨 12° のまま）
	const float headDegree = mLayout->groups()[ mLayout->groupOf( BodyPart::Head ) ].kickDegree;
	const float torsoDegree = mLayout->groups()[ mLayout->groupOf( BodyPart::Torso ) ].kickDegree;
	for ( int i = 0; i < kLungeHeadPushes; ++i ) {
		kick( BodyPart::Head, rootDirection, headDegree );
	}
	kick( BodyPart::Torso, rootDirection, torsoDegree );
}

void ZombieFlinch::kick( BodyPart part, const Vector3& direction, float degree ) {
	Vector3& pushed = mPushes[ mLayout->groupOf( part ) ];
	pushed.madd( direction, degree );
	float length = pushed.length();
	if ( length > kMaxDegree ) {
		pushed *= kMaxDegree / length;
	}
}

void ZombieFlinch::update() {
	for ( Vector3& pushed : mPushes ) {
		pushed *= kDecayPerTick;
	}
}

bool ZombieFlinch::isActive() const {
	for ( const Vector3& pushed : mPushes ) {
		if ( pushed.length() >= kNegligibleDegree ) {
			return true;
		}
	}
	return false;
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
