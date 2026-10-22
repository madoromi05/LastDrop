#ifndef INCLUDED_MENU_SCENE_H
#define INCLUDED_MENU_SCENE_H

#include "GameSrc/Scene/IScene.h"
#include "GameSrc/Scene/Menu/MenuLayout.h"
#include "GameSrc/UI/UiScreen.h"

class GameResources;

// 見出しとボタンだけの画面（タイトル・クリア画面）。中身は MenuDefinition で受け取り、ここは画面の動かし方だけを持つ
// （並べ方は MenuLayout）。
// マウスでも、上下キー＋決定キー（Enter / Space）でも操作できる（操作の扱いは UiCanvas）。
// 画面ごとの違い（文言・色・ボタンの行き先）は MenuCatalog の1か所にあるので、画面を足すときにシーンのクラスは増えない。
// 画面ごとに違う動き（演出など）が要るようになったら、その画面だけ別のシーンのクラスに分ける。
class MenuScene : public IScene{
public:
	// resources: UI の道具を借りる。所有権は受け取らない。nullptr 不可。このシーンより長く生きていること。
	// definition: 並べる中身。写して使うので、渡した後は消してよい。
	MenuScene( GameResources* resources, const MenuDefinition& definition );
	virtual void beginFrame();
	virtual void update();
	virtual void draw();
private:
	UiScreen mScreen;
};

#endif
