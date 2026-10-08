#ifndef INCLUDED_SCENE_MANAGER_H
#define INCLUDED_SCENE_MANAGER_H

#include "FixedStepClock.h"

class IScene;

// 現在のシーンを保持し、切り替え・更新・破棄を一手に担う。
// シーン分岐を Main の if/else から切り出すための窓口（既存 Pad/Manager と同じ singleton 流儀）。
class SceneManager{
public:
	// シーンの種類の一覧。ここを見れば全シーンが分かる。増やすときはここに1行足し、
	// SceneManager::change( SceneId ) の生成分岐（SceneManager.cpp）にも1行足す。
	enum SceneId{
		SCENE_SKIN_TEST, // スキニング検証シーン。
		SCENE_FPS_TEST,  // 自キャラを FPS 視点で動かす検証シーン。

		SCENE_TITLE,     // タイトル画面。
		SCENE_GAME,      // ゲーム本編（ガソリンを車へ運ぶ）。
		SCENE_CLEAR,     // クリア画面。
	};

	static SceneManager& instance();
	// 種別で切り替える（推奨）。対応する実体を内部で生成する。
	void change( SceneId id );
	// 次の update の終わり（今のシーンの draw の後）に、種別 id のシーンへ切り替える予約をする。
	// シーンの中（ボタンを押したときなど）から切り替えるときはこちらを使う。change はその場で今のシーンを消すので、
	// シーンの関数の途中で呼ぶと、実行中のシーン自身が消えてしまう。何回呼んでも最後の予約だけが使われる。
	void requestChange( SceneId id );
	// 実体を直接渡して切り替える。next の所有権を受け取り、現シーンは破棄する。
	void change( IScene* next );
	// 1回の描画ぶん、現在のシーンを動かす（無ければ何もしない）。
	// beginFrame → 経った時間ぶんの update（0回以上） → draw の順に呼ぶ（IScene.h）。
	void update();
	// 現在のシーンを破棄する（終了時）。
	void destroy();
	// 現在のシーンを返す（無ければ0）。
	IScene* current() const;
private:
	SceneManager();
	~SceneManager();
	SceneManager( const SceneManager& );
	void operator=( const SceneManager& );
	IScene* mCurrent;
	bool mHasRequest;     // requestChange の予約があるか
	SceneId mRequestedId; // 予約された切り替え先
	FixedStepClock mClock; // 1回の描画で update を何回呼ぶか
};

#endif
