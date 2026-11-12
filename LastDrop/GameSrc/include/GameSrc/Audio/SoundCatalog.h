#ifndef INCLUDED_SOUND_CATALOG_H
#define INCLUDED_SOUND_CATALOG_H

#include <string>

// ゲームで鳴らす音の一覧（音の目録）。ゲームコードは音を SoundId だけで呼び、ファイル名は知らない
// ファイル名・揺らぎの数・聞こえる距離は「音素材の性質」なので、この目録（SoundCatalog.cpp）だけに書く
//
// 目録はゲーム本体と音の生成ツール（tools/SoundGen）の両方がリンクする
// ツールはこの目録のファイル名で WAV を書き出し、ゲームは同じ名前で読むので、名前の食い違いが起きない
// 録音素材に差し替えるときは、目録の名前の WAV を Resource/Sound に置き、目録で recorded にする（SoundGen が上書きしなくなる）
// ゲームのコードは変わらない
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
	SOUND_CLOTH_RUSTLE,			// 服の擦れ（銃を出す／しまう／覗く／やめるとき）
	SOUND_GEAR_RATTLE,			// 装備の揺れ（銃を持っている間の一歩ごと）
	SOUND_WEAPON_DRAW,			// 武器を出す
	SOUND_WEAPON_HOLSTER,		// 武器をしまう
	SOUND_AIM_IN,				// 覗く
	SOUND_AIM_OUT,				// 覗くのをやめる
	// ── 銃 ──
	SOUND_GUNSHOT,				// 銃声（1発ごと）
	SOUND_DRY_FIRE,				// 空撃ち（弾倉が空で引き金を引いた）
	SOUND_MAG_OUT,				// リロード：弾倉を抜く
	SOUND_MAG_IN,				// リロード：弾倉を挿す
	SOUND_BOLT_PULL,			// リロード：槓桿を引く
	// ── 環境音 ──
	SOUND_SIREN,				// 脱出のサイレン（1回鳴らす）
	// ── 弾が当たった音 ──
	SOUND_HIT_FLESH,			// ゾンビに当たった（頭も。弾が当たった音はこれ 1 つ）
	SOUND_ZOMBIE_BODYFALL,		// 倒れたゾンビの体が地面に着いた（低いドサッ。倒した所から）。やめた SOUND_HIT_HEAD の位置に入れた（後ろの番号を変えないため）
	// ── 弾が物に当たった音（着弾音。当たった所から鳴らす）
	SOUND_IMPACT_CONCRETE,		// コンクリート・レンガ（建物・歩道）。高い破裂と細かい破片
	SOUND_IMPACT_DIRT,			// 土・砂利（車道・町の外）。低く鈍い音と砂の落ちる音
	SOUND_IMPACT_METAL,			// 金属（車・ガソリン缶・弾）。短い破裂の後に響く
	// ── 装備（末尾に足す。SoundGen の種は音の番号から決まるので、途中に挿すと後ろの合成音が全部変わる）──
	SOUND_FLASHLIGHT_CLICK,		// 銃の懐中電灯のスイッチ（付ける・消す）
	// ── 銃（末尾に足す。理由は上と同じ）──
	SOUND_PISTOL_SHOT,			// 拳銃の銃声
	// ── 漁る・包帯（段階R3。末尾に足す。理由は上と同じ） ──
	SOUND_LOOT_SEARCH_METAL,	// 開けている間（ループ）：ゴミ箱（金属を探るガサガサ）
	SOUND_LOOT_SEARCH_WOOD,		// 開けている間（ループ）：木箱
	SOUND_LOOT_SEARCH_CLOTH,	// 開けている間（ループ）：死体（服を探る）
	// 開け終わった音・包帯を巻いた音は作らない（中を見る画面・カバンの画面の間は音の装置ごと止めるので、鳴らしても閉じた後に鳴る）
	// ── 脱出の車（末尾に足す。理由は上と同じ） ──
	SOUND_CAR_ENGINE_START,		// エンジンを掛ける（セルが回って掛かる。1 回）。車から鳴らす
	SOUND_CAR_ENGINE_IDLE,		// エンジンのアイドリング（ループ）。車から鳴らす
	SOUND_REFUEL_POUR,			// 給油している間（ループ）：缶からガソリンを注ぐ水の音（改訂 2）。自分の手元から鳴らす

	SOUND_NUM, // 音の種類の数。配列サイズや範囲判定に使う（要素数をマジックナンバーにしないため）
};

// 1種類の音の素材の性質
struct SoundEntry{
	const char* baseName;	// ファイル名の元（"<baseName>_<揺らぎ番号>.wav"）
	int variationCount;		// 揺らぎ（同じ音の別テイク）の数。毎回同じ音だと機械的に聞こえるので、鳴らすたびに選び直す
	float pitchJitter;		// 鳴らすたびに音程をこの割合まで上下させる（0 なら揺らさない）
	float minDistance;		// この距離（m）までは減衰しない
	float maxDistance;		// この距離（m）より先は減衰しない（この距離の大きさのまま）
	bool recorded;			// 録音素材か（true なら SoundGen は書き出さない。上書きを防ぐため）
	float startSeconds;		// 鳴らし始める位置（秒）。頭に無音がある録音素材で、その分を飛ばすのに使う（0 なら頭から）
};

const SoundEntry& soundEntry( SoundId id );				// 音の種類の性質を返す。id は SOUND_NUM 未満であること
std::string soundFilePath( SoundId id, int variation );	// 揺らぎ1つぶんのファイルの、プロジェクトのフォルダからの相対パス（例: "Resource/Sound/footstep_concrete_0.wav"）

#endif
