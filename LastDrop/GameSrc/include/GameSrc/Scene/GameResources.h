#ifndef INCLUDED_GAME_RESOURCES_H
#define INCLUDED_GAME_RESOURCES_H

#include "GameSrc/Render/MeshCache.h"
#include "GameSrc/Audio/AudioEngine.h"
#include "GameSrc/Audio/MusicPlayer.h"
#include "GameSrc/UI/UiRenderer.h"
#include "GameSrc/Core/GameSettings.h"
#include "GameSrc/Raid/RaidRecord.h"
#include "GameSrc/Map/TownCache.h"
#include "GameSrc/Map/TownEdits.h"
#include "GameSrc/Map/TownEditFileState.h"

// シーンをまたいで使い回す、作るのが重い物の持ち主。起動から終了まで1つだけある（SceneManager が持つ）
// ・MeshCache: メッシュの設計図とテクスチャ、スキニングのシェーダ
// ・AudioEngine: 音の装置と、読み込んだ全ての音
// ・MusicPlayer: 画面の BGM。シーンを替えても同じ曲なら鳴り続け、違う曲ならつないで切り替える
// ・UiRenderer: 画面の文字のフォントと字形
// ・GameSettings: 遊ぶ人が設定画面で変える値（音量・感度）。重くはないが、タイトルと本編の両方の設定画面で同じ値を使うためここに置く
// ・RaidRecord: 出撃と結果の画面の間で受け渡す物（最後の結果・F6 で固定した次の条件）。シーンは作り直すと消えるのでここに置く
// ・TownCache: 町の決まった形から作る重い物（ゾンビの歩ける床・建物の隠れの表）。出撃のたびに作り直さない
// ・TownEdits: 町の手直し（Resource/Map/TownEdit.xml。起動時に読む）。保存していない手直しも、起動中は出撃のたびに使う
// ・TownEditFileState: 手直しと TownEdit.xml の間の様子（起動時に読めなかった行の理由など）。F6 の Town タブが出し、Bake が先に保存するかを決める
// シーンが自分で持つと、シーンを作り直すたび（クリア → Retry など）に全部を読み直すことになる
// ここに置けば、読み込むのは最初の1回だけで、どの画面でも同じ音・同じフォントが使える
//
// シーンはコンストラクタでこれを借り、要る物だけを自分の部品へ貸す（どこからでも取れる入口にはしない
// 誰が何を使っているかを、コンストラクタの引数でたどれるようにするため）
// 一度読み込んだ物は終了まで持ち続ける。マップが増えて持ちきれなくなったら、ここに「使わない物を手放す」入口を足す
class GameResources{
public:
	// Graphics::Manager ができた後に作る（シェーダとフォントを作るため）
	GameResources();
	GameResources( const GameResources& ) = delete;
	GameResources& operator=( const GameResources& ) = delete;

	MeshCache& meshes();
	AudioEngine& audio();
	MusicPlayer& music();
	UiRenderer& uiRenderer();
	GameSettings& settings();
	RaidRecord& raidRecord();
	TownCache& town();
	TownEdits& townEdits();
	TownEditFileState& townEditFile();

private:
	MeshCache mMeshes;
	AudioEngine mAudio;
	MusicPlayer mMusic;	// mAudio を借りるので、その後に並べる（先に壊れる）
	UiRenderer mUiRenderer;
	GameSettings mSettings;
	RaidRecord mRaidRecord;
	TownCache mTown;
	TownEdits mTownEdits;
	TownEditFileState mTownEditFile;
};

#endif
