#include "FootstepEmitter.h"
#include "AudioEngine.h"

using namespace GameLib::Math;

namespace{
	// 足が地面に着く位相。歩き・走りのアニメは、どれも1周の前半で左足・後半で右足が前に出る（PlayerCharacter.cpp の isGaitCycle）。
	// 位相 0 と 0.5 を「足が着いた瞬間」とみなす。見た目の接地と音がずれて聞こえたら kContactPhaseOffset で合わせる
	// （正にすると音が遅れる）。
	const float kContactPhases[] = { 0.f, 0.5f };
	const float kContactPhaseOffset = 0.f;

	// 歩き方ごとの大きさと高さ。Gait の順に並べる。
	// 走りは踏み込みが強いので大きくする。音程は上げない（上げると体重の低音まで上がって軽く聞こえるため）。
	struct GaitSound{
		float volume;
		float pitch;
	};
	const GaitSound kGaitSounds[] = {
		{ 0.f,   1.f },	// GAIT_STILL（鳴らさない）
		{ 0.45f, 1.f },	// GAIT_WALK
		{ 1.f,   1.f },	// GAIT_RUN
	};
	static_assert( sizeof( kGaitSounds ) / sizeof( kGaitSounds[ 0 ] ) == GAIT_NUM, "kGaitSounds must list every Gait" );

	// 床の材質ごとの足音。GroundMaterial の順に並べる。
	const SoundId kMaterialSounds[] = {
		SOUND_FOOTSTEP_CONCRETE,
		SOUND_FOOTSTEP_METAL,
		SOUND_FOOTSTEP_WOOD,
		SOUND_FOOTSTEP_GRASS,
		SOUND_FOOTSTEP_GRAVEL,
		SOUND_FOOTSTEP_WATER,
		SOUND_FOOTSTEP_GLASS,
	};
	static_assert( sizeof( kMaterialSounds ) / sizeof( kMaterialSounds[ 0 ] ) == GROUND_NUM, "kMaterialSounds must list every GroundMaterial" );

	const float kFullCycle = 1.f;

	// 位相が previous から current へ進む間に target をまたいだか。1周して 0 に戻った場合（current < previous）も数える。
	bool crossed( float previous, float current, float target ) {
		if ( current >= previous ) {
			return previous < target && target <= current;
		}
		return target > previous || target <= current;
	}

	// 位相を 0〜1 に収める（オフセットを足して 1 を超えた・0 を下回ったとき用）。
	float wrapPhase( float phase ) {
		while ( phase >= kFullCycle ) {
			phase -= kFullCycle;
		}
		while ( phase < 0.f ) {
			phase += kFullCycle;
		}
		return phase;
	}
}

FootstepEmitter::FootstepEmitter( AudioEngine* audio ) :
	mAudio( audio ),
	mPreviousGait( GAIT_STILL ),
	mPreviousPhase( 0.f ) {
}

bool FootstepEmitter::update( Gait gait, float phase, const Vector3& feet, GroundMaterial material ) {
	// 止まっている間は鳴らさない。動き出した最初のフレームも鳴らさず位相を覚えるだけにする
	// （止まっていた間の位相は別のアニメのものなので、それと比べると着いていない足の音が鳴ってしまう）。
	bool wasMoving = ( mPreviousGait != GAIT_STILL );
	float previousPhase = mPreviousPhase;
	mPreviousGait = gait;
	mPreviousPhase = phase;
	if ( gait == GAIT_STILL || !wasMoving ) {
		return false;
	}
	bool stepped = false;
	for ( float contact : kContactPhases ) {
		if ( crossed( previousPhase, phase, wrapPhase( contact + kContactPhaseOffset ) ) ) {
			stepped = true;
		}
	}
	if ( stepped ) {
		const GaitSound& sound = kGaitSounds[ gait ];
		mAudio->play( kMaterialSounds[ material ], AudioEngine::BUS_EFFECT, feet, sound.volume, sound.pitch );
	}
	return stepped;
}
