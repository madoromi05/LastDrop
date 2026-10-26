#include "GameSrc/Audio/AmbienceSystem.h"
#include "GameSrc/Core/GameTime.h"

using namespace GameLib::Math;

namespace{
	// ── 脱出のサイレン ──
	// マップの奥の遠い所で鳴る（脱出地点の方向を音で知らせる）。
	const Vector3 kSirenPosition( -150.f, 10.f, -250.f );
	const float kSirenVolume = 0.9f;
	const float kFirstSirenSeconds = 30.f;
	const float kSirenIntervalSeconds = 120.f;
}

AmbienceSystem::AmbienceSystem( AudioEngine* audio ) :
	mAudio( audio ),
	mFrame( 0 ),
	mNextSirenFrame( GameTime::secondsToTicks( kFirstSirenSeconds ) ) {
}

void AmbienceSystem::update() {
	++mFrame;
	if ( mFrame >= mNextSirenFrame ) {
		mAudio->play( SOUND_SIREN, AudioEngine::BUS_AMBIENCE, kSirenPosition, kSirenVolume );
		mNextSirenFrame = mFrame + GameTime::secondsToTicks( kSirenIntervalSeconds );
	}
}
