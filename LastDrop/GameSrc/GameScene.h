#ifndef INCLUDED_GAME_SCENE_H
#define INCLUDED_GAME_SCENE_H

#include "IScene.h"
#include "UiScreen.h"

// ゲーム本編（ガソリンを3個、車へ運んだらクリア）。
// 今は段階1（画面の行き来の確認）の仮の中身: 見出しと、クリア画面へ進むボタンだけ。
// 段階2でマップ・車・ガソリン、段階3でインベントリ・操作・HUD に置き換える。
class GameScene : public IScene{
public:
	GameScene();
	virtual void beginFrame();
	virtual void update();
	virtual void draw();
private:
	UiScreen mScreen;
};

#endif
