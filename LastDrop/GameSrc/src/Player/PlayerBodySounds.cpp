#include "GameSrc/Player/PlayerBodySounds.h"
#include "GameSrc/Player/PlayerCharacter.h"

using namespace GameLib::Math;

namespace{
	// 道路と歩道の材質は TownGround が、それと違う材質にする場所（水たまりなど。今は無い）は TownMap が決める。
	// これは、地面が答えられない場所（地面を並べる前・地面の外）の材質。
	const GroundMaterial kDefaultGround = GROUND_CONCRETE;
	// 装備の音を鳴らす高さ（足元から。胸のあたり）。足音は足元、装備は体から鳴らして聞こえる高さを分ける。
	const float kBodySoundHeight = 1.3f;
}

PlayerBodySounds::PlayerBodySounds( AudioEngine* audio, const IGroundSurface* ground, const std::vector< GroundZone >& zones ) :
	mGround( zones.data(), static_cast< int >( zones.size() ), ground, kDefaultGround ),
	mFootsteps( audio ),
	mFoley( audio ) {
}

void PlayerBodySounds::update( const PlayerCharacter& player ) {
	const Vector3& feet = player.position();
	bool stepped = mFootsteps.update( player.gait(), player.gaitPhase(), feet, mGround );
	PlayerFoley::FoleyState foley;
	foley.bodyPosition.set( feet.x, feet.y + kBodySoundHeight, feet.z );
	foley.gait = player.gait();
	foley.weaponDrawn = player.isWeaponDrawn();
	foley.aiming = player.isAiming();
	foley.fired = player.firedThisTick();
	mFoley.update( foley, stepped );
}

GroundMaterial PlayerBodySounds::materialAt( const Vector3& position ) const {
	return mGround.materialAt( position );
}
