#include "GameSrc/Audio/PlayerFoley.h"
#include "GameSrc/Audio/AudioEngine.h"

using namespace GameLib::Math;

namespace{
	// ── 状態の切り替わりの音の大きさ ──
	// 1 を超える値は元の WAV より大きく鳴らす（miniaudio は増幅もできる）。
	// 銃の音の WAV は一番大きい所が 0.7 なので、1.4（0.98）までは割れない。ただ足音より目立つとうるさいので、
	// 0.5 以下に抑える（目立たせたいときは他の音を下げる）。
	const float kWeaponDrawVolume = 0.5f;
	const float kWeaponHolsterVolume = 0.45f;
	const float kAimInVolume = 0.35f;
	const float kAimOutVolume = 0.25f;
	// 銃の音（出し入れ・覗く。今はどれもスリングと袖が擦れる音だけ）は元の WAV より低く鳴らす（0.85 倍 ≈ 3 半音下）。
	// 低くすると厚手の布の重さが出る。そのぶん音は少し長くなる（1 / 0.85 倍）。
	const float kWeaponPitch = 0.85f;
	// 銃の出し入れ・覗く／やめるときに、腕を動かした服の擦れを重ねる。
	// 銃の音（スリングの擦れ）が主役なので、それより小さくする。音程は下げない（下げると服の軽さが消える）。
	const float kClothDrawVolume = 0.2f;
	const float kClothHolsterVolume = 0.2f;
	const float kClothAimInVolume = 0.125f;
	const float kClothAimOutVolume = 0.1f;

	// ── 銃声の大きさ ──
	// 元の WAV のまま鳴らす。足音・装備の音（0.5 以下）よりはっきり大きく、連射で重なっても割れにくい大きさ。
	const float kGunshotVolume = 1.f;

	// ── 一歩ごとの音の大きさ（Gait の順）──
	// 装備（銃を持っている間だけ）は走ると大きく揺れる。
	// 服の擦れは足音に被って邪魔だったので一歩ごとには鳴らさない（状態の切り替わりでだけ鳴らす）。
	const float kGearVolumes[]  = { 0.f, 0.06f, 0.175f };
	static_assert( sizeof( kGearVolumes ) / sizeof( kGearVolumes[ 0 ] ) == GAIT_NUM, "kGearVolumes must list every Gait" );
}

PlayerFoley::PlayerFoley( AudioEngine* audio ) :
	mAudio( audio ),
	mHasPrevious( false ),
	mWasWeaponDrawn( false ),
	mWasAiming( false ) {
}

void PlayerFoley::update( const FoleyState& state, bool stepped ) {
	if ( mHasPrevious ) {
		playTransitions( state );
	}
	if ( stepped ) {
		playStep( state );
	}
	if ( state.fired ) {
		mAudio->play( SOUND_GUNSHOT, AudioEngine::BUS_EFFECT, state.bodyPosition, kGunshotVolume );
	}
	mHasPrevious = true;
	mWasWeaponDrawn = state.weaponDrawn;
	mWasAiming = state.aiming;
}

void PlayerFoley::playTransitions( const FoleyState& state ) {
	if ( state.weaponDrawn != mWasWeaponDrawn ) {
		SoundId id = state.weaponDrawn ? SOUND_WEAPON_DRAW : SOUND_WEAPON_HOLSTER;
		float volume = state.weaponDrawn ? kWeaponDrawVolume : kWeaponHolsterVolume;
		mAudio->play( id, AudioEngine::BUS_EFFECT, state.bodyPosition, volume, kWeaponPitch );
		float clothVolume = state.weaponDrawn ? kClothDrawVolume : kClothHolsterVolume;
		mAudio->play( SOUND_CLOTH_RUSTLE, AudioEngine::BUS_EFFECT, state.bodyPosition, clothVolume );
	}
	if ( state.aiming != mWasAiming ) {
		SoundId id = state.aiming ? SOUND_AIM_IN : SOUND_AIM_OUT;
		float volume = state.aiming ? kAimInVolume : kAimOutVolume;
		mAudio->play( id, AudioEngine::BUS_EFFECT, state.bodyPosition, volume, kWeaponPitch );
		float clothVolume = state.aiming ? kClothAimInVolume : kClothAimOutVolume;
		mAudio->play( SOUND_CLOTH_RUSTLE, AudioEngine::BUS_EFFECT, state.bodyPosition, clothVolume );
	}
}

void PlayerFoley::playStep( const FoleyState& state ) {
	if ( state.weaponDrawn ) {
		mAudio->play( SOUND_GEAR_RATTLE, AudioEngine::BUS_EFFECT, state.bodyPosition, kGearVolumes[ state.gait ] );
	}
}
