#include "GameSrc/Audio/SoundCatalog.h"

namespace{
	const char* kSoundDirectory = "Resource/Sound/";
	const char* kSoundExtension = ".wav";

	// ── 揺らぎの数 ──
	// 一歩ごとに鳴る音は、同じ音が続くと機械的に聞こえやすいので多めに持つ。
	const int kStepVariations = 4;
	const int kSingle = 1;
	// 連射で続けて当たるので、体に当たった音は別テイクを持つ。
	const int kHitFleshVariations = 3;
	// 着弾音も連射で続くので、足音と同じ数を持つ。金属は響きが長く聞き分けやすいので少なめ。
	const int kImpactVariations = 4;
	const int kImpactMetalVariations = 3;
	// 倒れる音は続けて倒すと重なるので 2 つ持つ（HitFeel_Plan.txt）。
	const int kBodyFallVariations = 2;

	// ── 素材の出どころ ──
	// 録音素材は SoundGen が書き出さない（合成音で上書きしないため）。
	const bool kSynthesized = false;
	const bool kRecorded = true;

	// ── 音程の揺らぎ ──
	const float kStepPitchJitter = 0.06f;
	const float kFoleyPitchJitter = 0.04f;
	// 連射で同じ音が続くので、少しだけ揺らして機械的に聞こえないようにする（大きく揺らすと別の銃に聞こえる）。
	const float kGunshotPitchJitter = 0.03f;
	// 当たった音は 1 体に 10 回/秒 続くので、銃声より大きく揺らす（同じ音の繰り返しに聞こえないように）。
	const float kHitPitchJitter = 0.08f;
	// 着弾音も同じ理由で揺らす。金属は響きの高さが変わると別の物に聞こえるので、少し控えめ。
	const float kImpactPitchJitter = 0.08f;
	const float kImpactMetalPitchJitter = 0.06f;
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
	// 当たった音は足音と同じくらい届く。近い間（3m まで）は減衰させず、すぐ横で鳴る銃声に埋もれないようにする。
	const float kHitMinDistance = 3.f;
	const float kHitMaxDistance = 40.f;
	// 倒れたゾンビの体の音。自分が倒した手応えの音なので、当たった音より近くまでしか届かなくてよい。
	const float kBodyFallMinDistance = 2.f;
	const float kBodyFallMaxDistance = 30.f;
	// 物に当たった音（着弾音）。まずはゾンビに当たった音と同じ距離から始め、耳で直す。
	// タルコフも物への着弾と体への命中の聞こえる距離を別々に調整しているので、定数を分けておく（ImpactSound_Plan.txt 1 章）。
	const float kImpactMinDistance = 3.f;
	const float kImpactMaxDistance = 40.f;
	// サイレンは遠くから鳴る大きな音。近くでは減衰させず、マップの端からでも聞こえるようにする。
	const float kSirenMinDistance = 30.f;
	const float kSirenMaxDistance = 800.f;
	// 脱出の車のエンジン（FuelEscapeLoading_Plan.txt Q5）。ゾンビを呼ぶ音なので、遠くまで聞こえる（銃声より近い）。
	const float kEngineMinDistance = 3.f;
	const float kEngineMaxDistance = 80.f;

	// ── 鳴らし始める位置（秒）──
	const float kFromStart = 0.f;
	// 銃声の素材は、頭の約 0.6 秒がほぼ無音で、発砲の音はその後に入っている。
	// 頭から鳴らすと、引き金を引いてから音が出るまでが遅れて聞こえるので、発砲の音の直前から鳴らす。
	const float kGunshotStartSeconds = 0.6f;
	// 体に当たった音の素材は、頭の約 0.04〜0.06 秒が無音（3 テイクで短い方に合わせる）。弾が当たった瞬間に鳴らすため飛ばす。
	const float kHitFleshStartSeconds = 0.04f;
	// 槓桿の音の素材は、頭の約 0.22 秒が無音。動きの合図（WeaponReload の CUE_BOLT）と同時に鳴らすため飛ばす。
	const float kBoltPullStartSeconds = 0.2f;

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
		// 空撃ち・リロードは自分の手元の音なので、装備の音と同じ距離。素材はどれも weapons/Weapons の録音（頭に無音が無い物は頭から）。
		// 元の素材名: "Weapon Gun Rifle M16 Dry Fire 02.wav"
		{ "dry_fire",          kSingle, kFoleyPitchJitter, kFoleyMinDistance, kFoleyMaxDistance, kRecorded, kFromStart },
		// 元の素材名: "Weapon Gun Machine Gun 9mm Clip Release 01.wav"
		{ "mag_out",           kSingle, kFoleyPitchJitter, kFoleyMinDistance, kFoleyMaxDistance, kRecorded, kFromStart },
		// 元の素材名: "Weapon Gun Rifle M16 Clip Insertion 01.wav"
		{ "mag_in",            kSingle, kFoleyPitchJitter, kFoleyMinDistance, kFoleyMaxDistance, kRecorded, kFromStart },
		// 元の素材名: "Weapon Gun Rifle M16 Bolt Cock 02.wav"
		{ "bolt_pull",         kSingle, kFoleyPitchJitter, kFoleyMinDistance, kFoleyMaxDistance, kRecorded, kBoltPullStartSeconds },
		{ "siren",             kSingle, kNoPitchJitter, kSirenMinDistance, kSirenMaxDistance, kSynthesized, kFromStart },
		// 元の素材名（Adobe Audition の素材集）: "Weapon Staff Hit Body 01.wav" / "Weapon Staff Hit Body Fast 01.wav" / "Weapon Staff Hit Body Fast 02.wav"
		{ "hit_flesh",         kHitFleshVariations, kHitPitchJitter, kHitMinDistance, kHitMaxDistance, kRecorded, kHitFleshStartSeconds },
		// 2026-11-06 頭に当たった音（hit_head）をやめた所に入れた（後ろの番号がずれないように。合成は BodyFallSynth.cpp）。
		{ "zombie_bodyfall",   kBodyFallVariations, kFoleyPitchJitter, kBodyFallMinDistance, kBodyFallMaxDistance, kSynthesized, kFromStart },
		{ "impact_concrete",   kImpactVariations, kImpactPitchJitter, kImpactMinDistance, kImpactMaxDistance, kSynthesized, kFromStart },
		{ "impact_dirt",       kImpactVariations, kImpactPitchJitter, kImpactMinDistance, kImpactMaxDistance, kSynthesized, kFromStart },
		{ "impact_metal",      kImpactMetalVariations, kImpactMetalPitchJitter, kImpactMinDistance, kImpactMaxDistance, kSynthesized, kFromStart },
		{ "flashlight_click",  kSingle, kFoleyPitchJitter, kFoleyMinDistance, kFoleyMaxDistance, kSynthesized, kFromStart },
		// 聞こえる距離はライフルの銃声と同じ。素材は頭に無音が無い。
		// 元の素材名: "Weapon Gun Pistol 9mm Glock Single Shot Interior Shooting Range Short 01.wav"
		{ "pistol_shot",       kSingle, kGunshotPitchJitter, kGunshotMinDistance, kGunshotMaxDistance, kRecorded, kFromStart },
		// 漁る音（R3_Plan.txt Q7）。自分の手元の音なので、装備の音と同じ距離。ループはテイク 1 つ（継ぎ目は SoundGen が畳む）。
		// ゾンビに聞かせるかは段階R4 で決める（今は鳴らすだけ）。
		{ "loot_search_metal", kSingle, kNoPitchJitter, kFoleyMinDistance, kFoleyMaxDistance, kSynthesized, kFromStart },
		{ "loot_search_wood",  kSingle, kNoPitchJitter, kFoleyMinDistance, kFoleyMaxDistance, kSynthesized, kFromStart },
		{ "loot_search_cloth", kSingle, kNoPitchJitter, kFoleyMinDistance, kFoleyMaxDistance, kSynthesized, kFromStart },
		// 脱出の車（FuelEscapeLoading_Plan.txt Q5）。手元に車の録音が無いので合成（tools/SoundGen/EngineSynth.cpp）。
		{ "car_engine_start",  kSingle, kNoPitchJitter, kEngineMinDistance, kEngineMaxDistance, kSynthesized, kFromStart },
		{ "car_engine_idle",   kSingle, kNoPitchJitter, kEngineMinDistance, kEngineMaxDistance, kSynthesized, kFromStart },
		// 給油している間（改訂 2）。自分の手元の音なので、漁る音と同じ距離（tools/SoundGen/PourSynth.cpp）。
		{ "refuel_pour",       kSingle, kNoPitchJitter, kFoleyMinDistance, kFoleyMaxDistance, kSynthesized, kFromStart },
	};
	static_assert( sizeof( kEntries ) / sizeof( kEntries[ 0 ] ) == SOUND_NUM, "kEntries must list every SoundId" );
}

const SoundEntry& soundEntry( SoundId id ) {
	return kEntries[ id ];
}

std::string soundFilePath( SoundId id, int variation ) {
	return std::string( kSoundDirectory ) + kEntries[ id ].baseName + "_" + std::to_string( variation ) + kSoundExtension;
}
