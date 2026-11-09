#ifndef INCLUDED_SCENE_MANAGER_H
#define INCLUDED_SCENE_MANAGER_H

#include "GameSrc/Core/FixedStepClock.h"
#include <memory>

class IScene;
class GameResources;

// 現在のシーンを保持し、切り替え・更新・破棄を一手に担う。
// シーン分岐を Main の if/else から切り出すための窓口（既存 Pad/Manager と同じ singleton 流儀）。
// シーンをまたいで使い回す重い物（GameResources。メッシュ・音・UI の道具）もここが持ち、作るシーンへ貸す。
// 最初のシーンより先に作り、最後のシーンより後に消す。シーンを作り直しても読み直しにならない。
class SceneManager{
public:
	// シーンの種類の一覧。ここを見れば全シーンが分かる。増やすときはここに1行足し、
	enum SceneId{
		SCENE_TITLE,    // タイトル画面（MenuScene に MenuCatalog::title の中身を載せたもの）。
		SCENE_GAME,     // ゲーム本編（1 回の出撃）。タイトルの Start・一時停止の Retry・死んだ画面の「もう一度」の行き先。
		SCENE_RAID_END, // 出撃の結果（クリア・死んだ。RaidEndScene）。
	};

	static SceneManager& instance();
	// 使い回す物（GameResources）を作り、最初のシーン first を開く。
	// 起動時に1回だけ、Graphics::Manager ができた後に呼ぶ（シェーダとフォントを作るため）。
	void start( SceneId first );
	// 次の update の終わり（今のシーンの draw の後）に、種別 id のシーンへ切り替える予約をする。
	// シーンの中（ボタンを押したときなど）から切り替えるときに使う。その場で切り替えると、
	// シーンの関数の途中で、実行中のシーン自身が消えてしまうため。何回呼んでも最後の予約だけが使われる。
	void requestChange( SceneId id );
	// 1回の描画ぶん、現在のシーンを動かす（無ければ何もしない）。
	// 入力を読む（Pad::update）→ beginFrame → 経った時間ぶんの update（0回以上） → draw の順に呼ぶ（IScene.h）。
	void update();
	// 現在のシーンと、使い回す物を破棄する（終了時。Graphics::Manager が消える前に呼ぶ）。
	void destroy();
	// ゲームを進める時計。開発用のエディタ（EditorWindow の Scene の欄）が、一時停止・コマ送り・時間の倍率に使う。
	FixedStepClock& clock();
private:
	SceneManager();
	~SceneManager();
	SceneManager( const SceneManager& );
	void operator=( const SceneManager& );
	// 種別 id のシーンを作る。作ったシーンは mResources を借りる。
	std::unique_ptr< IScene > createScene( SceneId id ) const;
	// ロードの計測で使う、シーンの名前（Profiling/<起動時刻>/load_<名前>_*.json）。
	static const char* loadName( SceneId id );
	// 種別 id のシーンへその場で切り替える。新しいシーンを作ってから、前のシーンを消す。
	// シーンの関数の途中では呼ばない（requestChange を使う）。
	void change( SceneId id );

	// シーンが借りるので、シーンより先に宣言する（＝シーンより後に破棄される）。
	std::unique_ptr< GameResources > mResources;
	std::unique_ptr< IScene > mCurrent;
	bool mHasRequest;     // requestChange の予約があるか
	SceneId mRequestedId; // 予約された切り替え先
	FixedStepClock mClock; // 1回の描画で update を何回呼ぶか
};

#endif
