#include "GameSrc/Zombie/Move/ZombieCrowd.h"
#include "GameSrc/Zombie/Move/ZombieCrowdParams.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Functions.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// 分離の値（F6 で動かせる）
	ZombieCrowdParams gParams;

	// これより近い（ほぼ重なっている）仲間は、離れる向きが決まらないので数えない（m）
	// 重なりは CharacterMover が掃引の頭で外へ出す
	const float kMinSeparationDistance = 0.001f;
}

ZombieCrowdParams& zombieCrowdParams() {
	return gParams;
}

void ZombieCrowd::clear() {
	mMembers.clear();
}

void ZombieCrowd::add( const GameObject* owner, const Vector3& position ) {
	mMembers.push_back( Member{ owner, position } );
}

Vector3 ZombieCrowd::separation( const GameObject* self, const Vector3& position, float radius ) const {
	Vector3 sum( 0.f, 0.f, 0.f );
	if ( radius <= 0.f ) {
		return sum;
	}
	for ( const Member& member : mMembers ) {
		if ( member.owner == self ) {
			continue;
		}
		Vector3 away( position.x - member.position.x, 0.f, position.z - member.position.z );
		float distance = away.length();
		if ( distance >= radius || distance < kMinSeparationDistance ) {
			continue;
		}
		// 自分への向き（長さ 1）に、近いほど大きい重みを掛ける
		sum.madd( away, ( 1.f - distance / radius ) / distance );
	}
	float length = sum.length();
	if ( length > 1.f ) {
		sum *= 1.f / length;
	}
	return sum;
}
