#ifndef INCLUDED_GAME_RESOURCES_H
#define INCLUDED_GAME_RESOURCES_H

#include "GameSrc/Render/MeshCache.h"
#include "GameSrc/Audio/AudioEngine.h"
#include "GameSrc/UI/UiRenderer.h"
#include "GameSrc/Core/GameSettings.h"

// シーンをまたいで使い回す、作るのが重い物の持ち主。起動から終了まで1つだけある（SceneManager が持つ）。
// ・MeshCache: メッシュの設計図とテクスチャ、スキニングのシェーダ
// ・AudioEngine: 音の装置と、読み込んだ全ての音
// ・UiRenderer: 画面の文字のフォントと字形
// ・GameSettings: 遊ぶ人が設定画面で変える値（音量・感度）。重くはないが、タイトルと本編の両方の設定画面で同じ値を使うためここに置く
// シーンが自分で持つと、シーンを作り直すたび（クリア → Retry など）に全部を読み直すことになる。
// ここに置けば、読み込むのは最初の1回だけで、どの画面でも同じ音・同じフォントが使える。
//
// シーンはコンストラクタでこれを借り、要る物だけを自分の部品へ貸す（どこからでも取れる入口にはしない。
// 誰が何を使っているかを、コンストラクタの引数でたどれるようにするため）。
// 一度読み込んだ物は終了まで持ち続ける。マップが増えて持ちきれなくなったら、ここに「使わない物を手放す」入口を足す。
class GameResources{
public:
	// Graphics::Manager ができた後に作る（シェーダとフォントを作るため）。
	GameResources();
	GameResources( const GameResources& ) = delete;
	GameResources& operator=( const GameResources& ) = delete;

	MeshCache& meshes();
	AudioEngine& audio();
	UiRenderer& uiRenderer();
	GameSettings& settings();

private:
	MeshCache mMeshes;
	AudioEngine mAudio;
	UiRenderer mUiRenderer;
	GameSettings mSettings;
};

#endif
