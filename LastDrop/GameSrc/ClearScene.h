#ifndef INCLUDED_CLEAR_SCENE_H
#define INCLUDED_CLEAR_SCENE_H

#include "IScene.h"
#include "UiScreen.h"

// クリア画面（ガソリンを車へ運び終えた後）。見出しと、Retry（本編をもう一度）・Title（タイトルへ）のボタン。
class ClearScene : public IScene{
public:
	ClearScene();
	virtual void beginFrame();
	virtual void update();
	virtual void draw();
private:
	UiScreen mScreen;
};

#endif
