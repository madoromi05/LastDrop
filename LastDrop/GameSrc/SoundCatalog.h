#ifndef INCLUDED_SOUND_CATALOG_H
#define INCLUDED_SOUND_CATALOG_H

#include <string>

// ゲームで鳴らす音の一覧（音の目録）。ゲームコードは音を SoundId だけで呼び、ファイル名は知らない。
// ファイル名・揺らぎの数・聞こえる距離は「音素材の性質」なので、この目録（SoundCatalog.cpp）だけに書く。
//
// 目録はゲーム本体と音の生成ツール（tools/SoundGen）の両方がリンクする。
// ツールはこの目録のファイル名で WAV を書き出し、ゲームは同じ名前で読むので、名前の食い違いが起きない。
// 録音素材に差し替えるときは、目録の名前の WAV を Resource/Sound に置き、目録で recorded にする（SoundGen が上書きしなくなる）。
// ゲームのコードは変わらない。
enum SoundId{
	// ── 足音（床の材質ごと）──
	SOUND_FOOTSTEP_CONCRETE,
	SOUND_FOOTSTEP_METAL,
	SOUND_FOOTSTEP_WOOD,
	SOUND_FOOTSTEP_GRASS,
	SOUND_FOOTSTEP_GRAVEL,
	SOUND_FOOTSTEP_WATER,
	SOUND_FOOTSTEP_GLASS,
	// ── 装備・動作音（フォーリー）──
	SOUND_CLOTH_RUSTLE,		// 服の擦れ（銃を出す／しまう／覗く／やめるとき）
	SOUND_GEAR_RATTLE,		// 装備の揺れ（銃を持っている間の一歩ごと）
	SOUND_WEAPON_DRAW,		// 武器を出す
	SOUND_WEAPON_HOLSTER,	// 武器をしまう
	SOUND_AIM_IN,			// 覗く
	SOUND_AIM_OUT,			// 覗くのをやめる
	// ── 環境音 ──
	SOUND_AMBIENCE_WIND,	// 風（ループ）
	SOUND_AMBIENCE_BIRDS,	// 鳥（ループ）
	SOUND_AMBIENCE_INSECTS,	// 虫（ループ）
	SOUND_GENERATOR,		// 発電機（ループ。置いた場所から鳴る）
	SOUND_SIREN,			// 脱出のサイレン（1回鳴らす）
	SOUND_AIRPLANE,			// エアドロップの飛行機（ループ。動かしながら鳴らす）

	SOUND_NUM, // 音の種類の数。配列サイズや範囲判定に使う（要素数をマジックナンバーにしないため）
};

// 1種類の音の素材の性質。
struct SoundEntry{
	const char* baseName;	// ファイル名の元（"<baseName>_<揺らぎ番号>.wav"）
	int variationCount;		// 揺らぎ（同じ音の別テイク）の数。毎回同じ音だと機械的に聞こえるので、鳴らすたびに選び直す
	float pitchJitter;		// 鳴らすたびに音程をこの割合まで上下させる（0 なら揺らさない）
	float minDistance;		// この距離（m）までは減衰しない
	float maxDistance;		// この距離（m）より先は減衰しない（この距離の大きさのまま）
	bool recorded;			// 録音素材か（true なら SoundGen は書き出さない。上書きを防ぐため）
};

// 音の種類の性質を返す。id は SOUND_NUM 未満であること。
const SoundEntry& soundEntry( SoundId id );
// 揺らぎ1つぶんのファイルの、プロジェクトのフォルダからの相対パス（例: "Resource/Sound/footstep_concrete_0.wav"）。
std::string soundFilePath( SoundId id, int variation );

#endif
