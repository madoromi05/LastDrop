#include "GameSrc/Audio/SoundCatalog.h"

namespace{
	const char* kSoundDirectory = "Resource/Sound/";
	const char* kKindDirectories[] = { "SE/Enemy/", "SE/Player/", "SE/Weapon/", "BGM/" };	// SoundKind の順
	const char* kVariationSeparator = "_";
	const char* kSoundExtension = ".wav";

	// ── 揺らぎの数 ──
	// 一歩ごとに鳴る音は、同じ音が続くと機械的に聞こえやすいので多めに持つ
	const int kStepVariations = 4;
	const int kSingle = 1;
	const int kHitFleshVariations = 3;	// 連射で続けて当たるので、体に当たった音は別テイクを持つ
	// 着弾音も連射で続くので、足音と同じ数を持つ。金属は響きが長く聞き分けやすいので少なめ
	const int kImpactVariations = 4;
	const int kImpactMetalVariations = 3;

	// ── 素材の出どころ ──
	// 録音素材は SoundGen が書き出さない（合成音で上書きしないため）
	const bool kSynthesized = false;
	const bool kRecorded = true;

	// ── 音の種類（置くフォルダと読み方）──
	// 効果音は起動のときに展開しておく（鳴らすたびに読むと、鳴るまでが遅れるため）
	// 曲は 1 曲 15MB 前後あるので展開しない。鳴らしている間だけ、ファイルから少しずつ読む
	const SoundKind kEnemy = SOUND_KIND_SE_ENEMY;
	const SoundKind kPlayer = SOUND_KIND_SE_PLAYER;
	const SoundKind kWeapon = SOUND_KIND_SE_WEAPON;
	const SoundKind kBgm = SOUND_KIND_BGM;

	// ── 音程の揺らぎ ──
	const float kStepPitchJitter = 0.06f;
	const float kFoleyPitchJitter = 0.04f;
	const float kGunshotPitchJitter = 0.03f;	// 連射で同じ音が続くので、少しだけ揺らして機械的に聞こえないようにする（大きく揺らすと別の銃に聞こえる）
	const float kHitPitchJitter = 0.08f;		// 当たった音は 1 体に 10 回/秒 続くので、銃声より大きく揺らす（同じ音の繰り返しに聞こえないように）
	// 着弾音も同じ理由で揺らす。金属は響きの高さが変わると別の物に聞こえるので、少し控えめ
	const float kImpactPitchJitter = 0.08f;
	const float kImpactMetalPitchJitter = 0.06f;
	const float kNoPitchJitter = 0.f;

	// ── 聞こえる距離（m）──
	// 足音はタルコフと同じく遠くまで届かせる（相手の位置を知る手がかりにするため）
	const float kStepMinDistance = 1.f;
	const float kStepMaxDistance = 40.f;
	// 自分の体から鳴る装備の音。他人のものとしては足音より近くでしか聞こえない
	const float kFoleyMinDistance = 0.5f;
	const float kFoleyMaxDistance = 15.f;
	// 銃声は足音よりずっと遠くまで届く大きな音（撃つと遠くの相手にも居場所が伝わる）
	const float kGunshotMinDistance = 5.f;
	const float kGunshotMaxDistance = 300.f;
	// 当たった音は足音と同じくらい届く。近い間（3m まで）は減衰させず、すぐ横で鳴る銃声に埋もれないようにする
	const float kHitMinDistance = 3.f;
	const float kHitMaxDistance = 40.f;
	// 物に当たった音（着弾音）。まずはゾンビに当たった音と同じ距離から始め、耳で直す
	// タルコフも物への着弾と体への命中の聞こえる距離を別々に調整しているので、定数を分けておく
	const float kImpactMinDistance = 3.f;
	const float kImpactMaxDistance = 40.f;
	// 脱出の車のエンジン。ゾンビを呼ぶ音なので、遠くまで聞こえる（銃声より近い）
	const float kEngineMinDistance = 3.f;
	const float kEngineMaxDistance = 80.f;
	// BGM は位置なし（2D）で鳴らすので、距離は使わない
	const float kNoDistance = 0.f;

	// ── 鳴らし始める位置（秒）──
	const float kFromStart = 0.f;
	// 銃声の素材は、頭の約 0.6 秒がほぼ無音で、発砲の音はその後に入っている
	// 頭から鳴らすと、引き金を引いてから音が出るまでが遅れて聞こえるので、発砲の音の直前から鳴らす
	const float kGunshotStartSeconds = 0.6f;
	const float kHitFleshStartSeconds = 0.04f;	// 体に当たった音の素材は、頭の約 0.04〜0.06 秒が無音（3 テイクで短い方に合わせる）。弾が当たった瞬間に鳴らすため飛ばす
	const float kBoltPullStartSeconds = 0.2f;	// 槓桿の音の素材は、頭の約 0.22 秒が無音。動きの合図（WeaponReload の CUE_BOLT）と同時に鳴らすため飛ばす

	// SoundId の順に並べる（static_assert で数を確かめる）
	const SoundEntry kEntries[] = {
		{ "footstep_concrete", kStepVariations, kStepPitchJitter, kStepMinDistance, kStepMaxDistance, kSynthesized, kFromStart, kPlayer },
		{ "footstep_metal",    kStepVariations, kStepPitchJitter, kStepMinDistance, kStepMaxDistance, kSynthesized, kFromStart, kPlayer },
		{ "footstep_wood",     kStepVariations, kStepPitchJitter, kStepMinDistance, kStepMaxDistance, kSynthesized, kFromStart, kPlayer },
		{ "footstep_gravel",   kStepVariations, kStepPitchJitter, kStepMinDistance, kStepMaxDistance, kSynthesized, kFromStart, kPlayer },
		{ "cloth_rustle",      kStepVariations, kFoleyPitchJitter, kFoleyMinDistance, kFoleyMaxDistance, kSynthesized, kFromStart, kPlayer },
		// 元の素材名: "Weapon Gun Machine Gun 9mm Clip Lock 01.wav"
		{ "gear_weapon",       kSingle, kFoleyPitchJitter, kFoleyMinDistance, kFoleyMaxDistance, kRecorded, kFromStart, kWeapon },
		{ "weapon_draw",       kSingle, kFoleyPitchJitter, kFoleyMinDistance, kFoleyMaxDistance, kSynthesized, kFromStart, kWeapon },
		{ "weapon_holster",    kSingle, kFoleyPitchJitter, kFoleyMinDistance, kFoleyMaxDistance, kSynthesized, kFromStart, kWeapon },
		{ "aim_in",            kSingle, kFoleyPitchJitter, kFoleyMinDistance, kFoleyMaxDistance, kSynthesized, kFromStart, kWeapon },
		{ "aim_out",           kSingle, kFoleyPitchJitter, kFoleyMinDistance, kFoleyMaxDistance, kSynthesized, kFromStart, kWeapon },
		{ "gunshot",           kSingle, kGunshotPitchJitter, kGunshotMinDistance, kGunshotMaxDistance, kRecorded, kGunshotStartSeconds, kWeapon },	// 元の素材名: "GunShot.wav"
		// 空撃ち・リロードは自分の手元の音なので、装備の音と同じ距離。素材はどれも weapons/Weapons の録音（頭に無音が無い物は頭から）
		// 元の素材名: "Weapon Gun Rifle M16 Dry Fire 02.wav"
		{ "dry_fire",          kSingle, kFoleyPitchJitter, kFoleyMinDistance, kFoleyMaxDistance, kRecorded, kFromStart, kWeapon },
		{ "mag_out",           kSingle, kFoleyPitchJitter, kFoleyMinDistance, kFoleyMaxDistance, kRecorded, kFromStart, kWeapon },	// 元の素材名: "Weapon Gun Machine Gun 9mm Clip Release 01.wav"
		{ "mag_in",            kSingle, kFoleyPitchJitter, kFoleyMinDistance, kFoleyMaxDistance, kRecorded, kFromStart, kWeapon },	// 元の素材名: "Weapon Gun Rifle M16 Clip Insertion 01.wav"
		// 元の素材名: "Weapon Gun Rifle M16 Bolt Cock 02.wav"
		{ "bolt_pull",         kSingle, kFoleyPitchJitter, kFoleyMinDistance, kFoleyMaxDistance, kRecorded, kBoltPullStartSeconds, kWeapon },
		{ "hit_flesh",         kHitFleshVariations, kHitPitchJitter, kHitMinDistance, kHitMaxDistance, kRecorded, kHitFleshStartSeconds, kEnemy },	// 元の素材名（Adobe Audition の素材集）: "Weapon Staff Hit Body 01.wav" / "Weapon Staff Hit Body Fast 01.wav" / "Weapon Staff Hit Body Fast 02.wav"
		{ "impact_concrete",   kImpactVariations, kImpactPitchJitter, kImpactMinDistance, kImpactMaxDistance, kSynthesized, kFromStart, kWeapon },
		{ "impact_dirt",       kImpactVariations, kImpactPitchJitter, kImpactMinDistance, kImpactMaxDistance, kSynthesized, kFromStart, kWeapon },
		{ "impact_metal",      kImpactMetalVariations, kImpactMetalPitchJitter, kImpactMinDistance, kImpactMaxDistance, kSynthesized, kFromStart, kWeapon },
		{ "flashlight_click",  kSingle, kFoleyPitchJitter, kFoleyMinDistance, kFoleyMaxDistance, kSynthesized, kFromStart, kWeapon },
		// 聞こえる距離はライフルの銃声と同じ。素材は頭に無音が無い
		// 元の素材名: "Weapon Gun Pistol 9mm Glock Single Shot Interior Shooting Range Short 01.wav"
		{ "pistol_shot",       kSingle, kGunshotPitchJitter, kGunshotMinDistance, kGunshotMaxDistance, kRecorded, kFromStart, kWeapon },
		// 漁る音。自分の手元の音なので、装備の音と同じ距離。ループはテイク 1 つ（継ぎ目は SoundGen が畳む）
		// ゾンビに聞かせるかは段階R4 で決める（今は鳴らすだけ）
		{ "loot_search_metal", kSingle, kNoPitchJitter, kFoleyMinDistance, kFoleyMaxDistance, kSynthesized, kFromStart, kPlayer },
		{ "loot_search_wood",  kSingle, kNoPitchJitter, kFoleyMinDistance, kFoleyMaxDistance, kSynthesized, kFromStart, kPlayer },
		// 脱出の車。手元に車の録音が無いので合成（tools/SoundGen/EngineSynth.cpp）
		{ "car_engine_start",  kSingle, kNoPitchJitter, kEngineMinDistance, kEngineMaxDistance, kSynthesized, kFromStart, kPlayer },
		{ "car_engine_idle",   kSingle, kNoPitchJitter, kEngineMinDistance, kEngineMaxDistance, kSynthesized, kFromStart, kPlayer },
		{ "refuel_pour",       kSingle, kNoPitchJitter, kFoleyMinDistance, kFoleyMaxDistance, kSynthesized, kFromStart, kPlayer },	// 給油している間。自分の手元の音なので、漁る音と同じ距離（tools/SoundGen/PourSynth.cpp）
		// BGM。C:\Audio の BGM 自動作曲（Songs/_generator/zombie_fps_songs.js）で書き出したループ用の WAV
		{ "bgm_title",         kSingle, kNoPitchJitter, kNoDistance, kNoDistance, kRecorded, kFromStart, kBgm },	// 元の曲: "01_灯の消えた町"
		{ "bgm_gameover",      kSingle, kNoPitchJitter, kNoDistance, kNoDistance, kRecorded, kFromStart, kBgm },	// 元の曲: "02_残された灯"
		{ "bgm_escaped",       kSingle, kNoPitchJitter, kNoDistance, kNoDistance, kRecorded, kFromStart, kBgm },	// 元の曲: "03_夜明けの国道_車内"（car_interior.js で車内の音を足した物）
	};
	static_assert( sizeof( kEntries ) / sizeof( kEntries[ 0 ] ) == SOUND_NUM, "kEntries must list every SoundId" );
	static_assert( sizeof( kKindDirectories ) / sizeof( kKindDirectories[ 0 ] ) == SOUND_KIND_NUM, "kKindDirectories must list every SoundKind" );
}

const SoundEntry& soundEntry( SoundId id ) {
	return kEntries[ id ];
}

bool soundStreamed( SoundId id ) {
	return kEntries[ id ].kind == SOUND_KIND_BGM;
}

std::string soundFilePath( SoundId id, int variation ) {
	const SoundEntry& entry = kEntries[ id ];
	std::string path = std::string( kSoundDirectory ) + kKindDirectories[ entry.kind ] + entry.baseName;
	if ( entry.variationCount > 1 ) {
		path += kVariationSeparator + std::to_string( variation );	// 揺らぎが 1 つだけなら番号を付けない（見分ける物が無いため）
	}
	return path + kSoundExtension;
}
