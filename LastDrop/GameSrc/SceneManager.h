#ifndef INCLUDED_SCENE_MANAGER_H
#define INCLUDED_SCENE_MANAGER_H

class IScene;

// 現在のシーンを保持し、切り替え・更新・破棄を一手に担う。
// シーン分岐を Main の if/else から切り出すための窓口（既存 Pad/Manager と同じ singleton 流儀）。
class SceneManager{
public:
	// シーンの種類の一覧。ここを見れば全シーンが分かる。増やすときはここに1行足し、
	// SceneManager::change( SceneId ) の生成分岐（SceneManager.cpp）にも1行足す。
	enum SceneId{
		SCENE_GAME,      // 従来ゲーム（設定＋プレイ）。current を空にしてフォールバック実行する。
		SCENE_SKIN_TEST, // スキニング検証シーン。
		SCENE_TPS_TEST,  // 自キャラを TPS 視点で動かす検証シーン。
	};

	static SceneManager& instance();
	// 種別で切り替える（推奨）。対応する実体を内部で生成する。
	void change( SceneId id );
	// 実体を直接渡して切り替える。next の所有権を受け取り、現シーンは破棄する。
	void change( IScene* next );
	// 現在のシーンを1フレーム更新する（無ければ何もしない）。
	void update();
	// 現在のシーンを破棄する（終了時）。
	void destroy();
	// 現在のシーンを返す（無ければ0）。呼び出し側の分岐判定に使う。
	IScene* current() const;
private:
	SceneManager();
	~SceneManager();
	SceneManager( const SceneManager& );
	void operator=( const SceneManager& );
	IScene* mCurrent;
};

#endif
