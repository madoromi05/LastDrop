#include "GameSrc/Zombie/ZombieFlinch.h"
#include "GameSrc/Zombie/ZombieFlinchLayout.h"
#include "GameSrc/Math/AxisRotation.h"
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
	// 骨の向きと押す向きがほぼ平行（外積の長さがこれ未満）なら、回す軸が決まらないので曲げない。
	const float kParallelLimit = 0.001f;

	Vector3 translationOf( const Matrix34& m ) {
		return Vector3( m.m03, m.m13, m.m23 );
	}
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
	const std::vector< ZombieFlinchLayout::Group >& groups = mLayout->groups();
	for ( size_t g = 0; g < groups.size(); ++g ) {
		float degree = mPushes[ g ].length();
		if ( degree < kNegligibleDegree ) {
			continue;
		}
		Vector3 pushDirection = mPushes[ g ];
		pushDirection *= 1.f / degree;
		// 根の側の関節から順に、「今の」関節の位置で曲げる（前の関節で動いた後の位置。FirstPersonBody::bendSpine と同じ）。
		for ( const ZombieFlinchLayout::Joint& joint : groups[ g ].joints ) {
			Vector3 center = translationOf( ( *pose )[ joint.bone ] );
			Vector3 boneDirection;
			boneDirection.setSub( translationOf( ( *pose )[ joint.tipBone ] ), center );
			boneDirection.normalize();
			// 骨の向き × 押す向き の軸で正に回すと、骨の先が押す向きへ倒れる。
			// 外積の長さ（sin）を角度に掛けて、骨が押す向きに近いほど少なく倒す（平行なら倒れない）。
			Vector3 axis;
			axis.setCross( boneDirection, pushDirection );
			float sine = axis.length();
			if ( sine < kParallelLimit ) {
				continue;
			}
			axis *= 1.f / sine;
			Matrix34 rotation = AxisRotation::about( center, axis, degree * joint.share * sine );
			for ( int b : joint.followers ) {
				Matrix34 bent;
				bent.setMul( rotation, ( *pose )[ b ] );
				( *pose )[ b ] = bent;
			}
		}
	}
}
