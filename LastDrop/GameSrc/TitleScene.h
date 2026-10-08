#ifndef INCLUDED_TITLE_SCENE_H
#define INCLUDED_TITLE_SCENE_H

#include "IScene.h"
#include "UiScreen.h"

// タイトル画面。ゲーム名と、Start（本編へ）・Quit（終了）のボタン。
// マウスでも、上下キー＋決定キー（Enter / Space）でも操作できる（操作の扱いは UiCanvas）。
class TitleScene : public IScene{
public:
	TitleScene();
	virtual void beginFrame();
	virtual void update();
	virtual void draw();
private:
	UiScreen mScreen;
};

#endif
