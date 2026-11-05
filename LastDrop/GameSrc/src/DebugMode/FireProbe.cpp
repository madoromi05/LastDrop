#include "GameSrc/DebugMode/FireProbe.h"

#include "Framework.h"
#include <cstdio>
#include <fstream>

namespace{
	const char* kLogFile = "Profiling/fire_log.txt"; // 計測の CSV と同じフォルダ（起動ごとのフォルダではなく 1 つに足していく）
	const int kWatchTicks = 60;   // 押してから撃てたかを見るティック（1 秒。押したまま待っても撃てないかも分かる）
	const int kLineSize = 512;

	int flag( bool value ) {
		return value ? 1 : 0;
	}
}

FireProbe::FireProbe() :
	mFrame( 0 ),
	mTicksThisFrame( 0 ),
	mPressNumber( 0 ),
	mWatchTicks( 0 ),
	mWatchedTicks( 0 ),
	mWasHeld( false ),
	mReady( false ) {
	char line[ kLineSize ];
	std::snprintf( line, sizeof( line ), "=== GameScene created  time=%u", GameLib::Framework::instance().time() );
	write( line );
}

void FireProbe::markReady() {
	mReady = true;
	char line[ kLineSize ];
	std::snprintf( line, sizeof( line ), "--- ready  time=%u", GameLib::Framework::instance().time() );
	write( line );
}

void FireProbe::onFrame( const FrameState& frame, const GunState& gun ) {
	if ( !mReady ) {
		return;
	}
	++mFrame;
	int ticks = mTicksThisFrame;
	mTicksThisFrame = 0;
	bool pressed = frame.rawFireHeld && !mWasHeld;
	bool released = !frame.rawFireHeld && mWasHeld;
	mWasHeld = frame.rawFireHeld;
	if ( !pressed && !released ) {
		return;
	}
	if ( pressed ) {
		++mPressNumber;
		mWatchTicks = kWatchTicks;
		mWatchedTicks = 0;
	}
	char line[ kLineSize ];
	std::snprintf( line, sizeof( line ),
		"%s #%d frame=%d time=%u prevTicks=%d | cmdFire=%d suppressed=%d locked=%d active=%d paused=%d bag=%d loot=%d freeCam=%d dead=%d"
		" | gun=%d inHand=%d aim=%d reload=%d run=%d rounds=%d",
		pressed ? "PRESS  " : "RELEASE", mPressNumber, mFrame, GameLib::Framework::instance().time(), ticks,
		flag( frame.commandFire ), flag( frame.suppressed ), flag( frame.cursorLocked ), flag( frame.windowActive ),
		flag( frame.paused ), flag( frame.bagOpen ), flag( frame.lootOpen ), flag( frame.freeCamera ), flag( frame.dead ),
		flag( gun.hasGun ), flag( gun.inHand ), flag( gun.aiming ), flag( gun.reloading ), flag( gun.running ), gun.rounds );
	write( line );
}

void FireProbe::onTick( bool fired, bool dryFired ) {
	++mTicksThisFrame;
	if ( mWatchTicks <= 0 ) {
		return;
	}
	--mWatchTicks;
	++mWatchedTicks;
	char line[ kLineSize ];
	if ( fired || dryFired ) {
		std::snprintf( line, sizeof( line ), "  -> #%d %s after %d ticks", mPressNumber, fired ? "FIRED" : "DRY FIRE", mWatchedTicks );
		write( line );
		mWatchTicks = 0;
	} else if ( mWatchTicks == 0 ) {
		std::snprintf( line, sizeof( line ), "  -> #%d NOT FIRED within %d ticks", mPressNumber, mWatchedTicks );
		write( line );
	}
}

void FireProbe::write( const char* line ) const {
	// 押したときだけ書くので、毎回開いて足す（強制終了しても、そこまでの行は残る）。
	std::ofstream file( kLogFile, std::ios::app );
	if ( file ) {
		file << line << '\n';
	}
}
