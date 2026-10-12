#include "SoundCatalog.h"

namespace{
	const char* kSoundDirectory = "Resource/Sound/";
	const char* kSoundExtension = ".wav";

	// ── 揺らぎの数 ──
	// 一歩ごとに鳴る音は、同じ音が続くと機械的に聞こえやすいので多めに持つ。
	const int kStepVariations = 4;
	const int kSingle = 1;

	// ── 素材の出どころ ──
	// 録音素材は SoundGen が書き出さない（合成音で上書きしないため）。
	const bool kSynthesized = false;
	const bool kRecorded = true;

	// ── 音程の揺らぎ ──
	const float kStepPitchJitter = 0.06f;
	const float kFoleyPitchJitter = 0.04f;
	// 連射で同じ音が続くので、少しだけ揺らして機械的に聞こえないようにする（大きく揺らすと別の銃に聞こえる）。
	const float kGunshotPitchJitter = 0.03f;
	const float kNoPitchJitter = 0.f;

	// ── 聞こえる距離（m）──
	// 足音はタルコフと同じく遠くまで届かせる（相手の位置を知る手がかりにするため）。
	const float kStepMinDistance = 1.f;
	const float kStepMaxDistance = 40.f;
	// 自分の体から鳴る装備の音。他人のものとしては足音より近くでしか聞こえない。
	const float kFoleyMinDistance = 0.5f;
	const float kFoleyMaxDistance = 15.f;
	// 銃声は足音よりずっと遠くまで届く大きな音（撃つと遠くの相手にも居場所が伝わる）。
	const float kGunshotMinDistance = 5.f;
	const float kGunshotMaxDistance = 300.f;
	// 位置を持たない環境音（2D で鳴らすので距離は使わない）。
	const float kUnusedDistance = 1.f;
	const float kGeneratorMinDistance = 2.f;
	const float kGeneratorMaxDistance = 60.f;
	// サイレンと飛行機は遠くから鳴る大きな音。近くでは減衰させず、マップの端からでも聞こえるようにする。
	const float kSirenMinDistance = 30.f;
	const float kSirenMaxDistance = 800.f;
	const float kAirplaneMinDistance = 60.f;
	const float kAirplaneMaxDistance = 1500.f;

	// ── 鳴らし始める位置（秒）──
	const float kFromStart = 0.f;
	// 銃声の素材は、頭の約 0.6 秒がほぼ無音で、発砲の音はその後に入っている。
	// 頭から鳴らすと、引き金を引いてから音が出るまでが遅れて聞こえるので、発砲の音の直前から鳴らす。
	const float kGunshotStartSeconds = 0.6f;

	// SoundId の順に並べる（static_assert で数を確かめる）。
	const SoundEntry kEntries[] = {
		{ "footstep_concrete", kStepVariations, kStepPitchJitter, kStepMinDistance, kStepMaxDistance, kSynthesized, kFromStart },
		{ "footstep_metal",    kStepVariations, kStepPitchJitter, kStepMinDistance, kStepMaxDistance, kSynthesized, kFromStart },
		{ "footstep_wood",     kStepVariations, kStepPitchJitter, kStepMinDistance, kStepMaxDistance, kSynthesized, kFromStart },
		{ "footstep_grass",    kStepVariations, kStepPitchJitter, kStepMinDistance, kStepMaxDistance, kSynthesized, kFromStart },
		{ "footstep_gravel",   kStepVariations, kStepPitchJitter, kStepMinDistance, kStepMaxDistance, kSynthesized, kFromStart },
		{ "footstep_water",    kStepVariations, kStepPitchJitter, kStepMinDistance, kStepMaxDistance, kSynthesized, kFromStart },
		{ "footstep_glass",    kStepVariations, kStepPitchJitter, kStepMinDistance, kStepMaxDistance, kSynthesized, kFromStart },
		{ "cloth_rustle",      kStepVariations, kFoleyPitchJitter, kFoleyMinDistance, kFoleyMaxDistance, kSynthesized, kFromStart },
		// 元の素材名: "Weapon Gun Machine Gun 9mm Clip Lock 01.wav"
		{ "gear_weapon",       kSingle, kFoleyPitchJitter, kFoleyMinDistance, kFoleyMaxDistance, kRecorded, kFromStart },
		{ "weapon_draw",       kSingle, kFoleyPitchJitter, kFoleyMinDistance, kFoleyMaxDistance, kSynthesized, kFromStart },
		{ "weapon_holster",    kSingle, kFoleyPitchJitter, kFoleyMinDistance, kFoleyMaxDistance, kSynthesized, kFromStart },
		{ "aim_in",            kSingle, kFoleyPitchJitter, kFoleyMinDistance, kFoleyMaxDistance, kSynthesized, kFromStart },
		{ "aim_out",           kSingle, kFoleyPitchJitter, kFoleyMinDistance, kFoleyMaxDistance, kSynthesized, kFromStart },
		// 元の素材名: "GunShot.wav"
		{ "gunshot",           kSingle, kGunshotPitchJitter, kGunshotMinDistance, kGunshotMaxDistance, kRecorded, kGunshotStartSeconds },
		{ "ambience_wind",     kSingle, kNoPitchJitter, kUnusedDistance, kUnusedDistance, kSynthesized, kFromStart },
		{ "ambience_birds",    kSingle, kNoPitchJitter, kUnusedDistance, kUnusedDistance, kSynthesized, kFromStart },
		{ "ambience_insects",  kSingle, kNoPitchJitter, kUnusedDistance, kUnusedDistance, kSynthesized, kFromStart },
		{ "generator",         kSingle, kNoPitchJitter, kGeneratorMinDistance, kGeneratorMaxDistance, kSynthesized, kFromStart },
		{ "siren",             kSingle, kNoPitchJitter, kSirenMinDistance, kSirenMaxDistance, kSynthesized, kFromStart },
		{ "airplane",          kSingle, kNoPitchJitter, kAirplaneMinDistance, kAirplaneMaxDistance, kSynthesized, kFromStart },
	};
	static_assert( sizeof( kEntries ) / sizeof( kEntries[ 0 ] ) == SOUND_NUM, "kEntries must list every SoundId" );
}

const SoundEntry& soundEntry( SoundId id ) {
	return kEntries[ id ];
}

std::string soundFilePath( SoundId id, int variation ) {
	return std::string( kSoundDirectory ) + kEntries[ id ].baseName + "_" + std::to_string( variation ) + kSoundExtension;
}
