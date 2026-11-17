#include "GameSrc/Zombie/Move/ZombieHearing.h"
#include "GameSrc/Zombie/Component/ZombieStateComponent.h"
#include "GameSrc/Zombie/Component/ZombieBrainComponent.h"
#include "GameSrc/World/GameObjectWorld.h"
#include "GameSrc/Math/FlatMath.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

namespace{
	const float kGunshotHearingRange = 30.f; // 銃声が届く距離（m。撃った所から水平だけ）
}

ZombieHearing::ZombieHearing( GameObjectWorld* objects ) :
	mObjects( objects ) {
	ASSERT( mObjects && "ZombieHearing : objects is null." );
}

void ZombieHearing::hearGunshot( const Vector3& position ) {
	mObjects->forEach< ZombieStateComponent >( [ &position ]( ZombieStateComponent& zombie ) {
		Vector3 toZombie;
		toZombie.setSub( zombie.position(), position );
		toZombie.y = 0.f;
		if ( toZombie.squareLength() <= kGunshotHearingRange * kGunshotHearingRange ) {
			zombie.getComponent< ZombieBrainComponent >()->hearNoise( position ); // 聞くかどうか（倒れている・よろけている等）はゾンビが決める
		}
	} );
}

void ZombieHearing::hearNoise( const Vector3& position ) {
	hearGunshot( position ); // 届く距離・寄って来方は銃声と同じ
}

void ZombieHearing::hearFootsteps( const Vector3& feet, float range ) {
	if ( range <= 0.f ) {
		return; // 止まっている（つかまれている間も止まっている）
	}
	mObjects->forEach< ZombieStateComponent >( [ &feet, range ]( ZombieStateComponent& zombie ) {
		if ( FlatMath::distance( zombie.position(), feet ) <= range ) {
			zombie.getComponent< ZombieBrainComponent >()->hearFootsteps(); // 聞くかどうかはゾンビが決める（銃声と同じ）
		}
	} );
}
