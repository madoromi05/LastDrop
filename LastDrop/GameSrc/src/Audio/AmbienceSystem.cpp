#include "GameSrc/Audio/AmbienceSystem.h"
#include "GameSrc/Core/GameTime.h"

using namespace GameLib::Math;

namespace{
	// 時間は「秒」で決め、フレーム（1ティック。GameTime.h）に直して使う（update は1ティックに1回）。
	int toFrames( float seconds ) {
		return static_cast< int >( seconds * GameTime::kTicksPerSecond );
	}

	// ── ベース（2D）──
	struct BedSound{
		SoundId id;
		float volume;
	};
	const BedSound kBeds[] = {
		{ SOUND_AMBIENCE_WIND,    0.25f },
		{ SOUND_AMBIENCE_BIRDS,   0.18f },
		{ SOUND_AMBIENCE_INSECTS, 0.13f },
	};

	// ── 置き物（3D）──
	// 発電機は右の建物の前（通りの奥）に置く。歩いて近づくと大きくなり、向きを変えると左右が入れ替わる。
	const Vector3 kGeneratorPosition( 10.f, 0.5f, -3.7f ); // 建物の正面の壁（z = -4.56）の手前、歩道の上
	const float kGeneratorVolume = 0.35f;

	// ── エアドロップの飛行機 ──
	// 通りの奥の上空を横切る（最初の向き＝建物の方を見て、右の +X から左の -X へ）。
	// ドップラー効果で、近づく間は高く、遠ざかると低く聞こえる。
	const Vector3 kAirplaneStart( 600.f, 120.f, -150.f );
	const Vector3 kAirplaneEnd( -600.f, 120.f, -150.f );
	const float kAirplaneSpeed = 70.f;			// m/秒（輸送機の低速飛行くらい）
	const float kAirplaneVolume = 1.f;
	const float kFirstAirplaneSeconds = 10.f;	// 最初の1回は確かめやすいよう早めに飛ばす
	const float kAirplaneIntervalSeconds = 75.f;

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
	mNextAirplaneFrame( toFrames( kFirstAirplaneSeconds ) ),
	mNextSirenFrame( toFrames( kFirstSirenSeconds ) ),
	mAirplane( AudioEngine::kInvalidLoop ),
	mAirplaneFrames( 0 ) {
	for ( const BedSound& bed : kBeds ) {
		mLoops.push_back( mAudio->startLoop( bed.id, AudioEngine::BUS_AMBIENCE, bed.volume ) );
	}
	mLoops.push_back( mAudio->startLoopAt( SOUND_GENERATOR, AudioEngine::BUS_AMBIENCE, kGeneratorPosition, kGeneratorVolume ) );
}

AmbienceSystem::~AmbienceSystem() {
	for ( AudioEngine::LoopHandle loop : mLoops ) {
		mAudio->stopLoop( loop );
	}
	mAudio->stopLoop( mAirplane );
}

void AmbienceSystem::update() {
	++mFrame;
	if ( mFrame >= mNextAirplaneFrame ) {
		startAirplane();
		mNextAirplaneFrame = mFrame + toFrames( kAirplaneIntervalSeconds );
	}
	if ( mFrame >= mNextSirenFrame ) {
		mAudio->play( SOUND_SIREN, AudioEngine::BUS_AMBIENCE, kSirenPosition, kSirenVolume );
		mNextSirenFrame = mFrame + toFrames( kSirenIntervalSeconds );
	}
	moveAirplane();
}

void AmbienceSystem::startAirplane() {
	if ( mAirplane != AudioEngine::kInvalidLoop ) {
		return; // まだ前の飛行機が飛んでいる
	}
	mAirplane = mAudio->startLoopAt( SOUND_AIRPLANE, AudioEngine::BUS_AMBIENCE, kAirplaneStart, kAirplaneVolume );
	mAirplaneFrames = 0;
}

void AmbienceSystem::moveAirplane() {
	if ( mAirplane == AudioEngine::kInvalidLoop ) {
		return;
	}
	Vector3 route;
	route.setSub( kAirplaneEnd, kAirplaneStart );
	float routeLength = route.length();
	float travelled = kAirplaneSpeed * mAirplaneFrames / GameTime::kTicksPerSecond;
	if ( travelled >= routeLength ) {
		mAudio->stopLoop( mAirplane );
		mAirplane = AudioEngine::kInvalidLoop;
		return;
	}
	route.normalize();
	Vector3 position = kAirplaneStart;
	position.madd( route, travelled );
	Vector3 velocity;
	velocity.setMul( route, kAirplaneSpeed );
	mAudio->setLoopPosition( mAirplane, position, velocity );
	++mAirplaneFrames;
}
