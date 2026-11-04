#ifndef INCLUDED_RUN_END_SCENE_H
#define INCLUDED_RUN_END_SCENE_H

#include "GameSrc/Scene/IScene.h"
#include "GameSrc/UI/UiScreen.h"

class GameResources;

// 5 日間の終わりの画面（R2_Plan.txt Q5・6-4）。クリア（5 日目に脱出）とゲームオーバー（死んだ・時間切れ）の両方。
// 見出し（SURVIVED 5 DAYS／KILLED IN ACTION／MISSING IN ACTION）・終わった日・スコアの 4 つの内訳と合計・ハイスコア（NEW BEST）。
// Title でタイトルへ（5 日間はもう 1 日目の形に戻してある＝次の Start は 1 日目から）。
// 中身は RunState の最後の終わり（RunRules が書いた RunEnd）を読むだけ。2026-11-05 に、見出しとボタンだけのクリア・ゲームオーバーの画面から替えた。
class RunEndScene : public IScene{
public:
	// resources: UI の道具と 5 日間の様子を借りる。所有権は受け取らない。nullptr 不可。このシーンより長く生きていること。
	explicit RunEndScene( GameResources* resources );
	virtual void beginFrame();
	virtual void update();
	virtual void draw();

private:
	// 内訳の 1 行（名前・掛け算・点）を index 行目に足す。detail は空でもよい。
	void addScoreLine( int index, const char* name, const char* detail, int points, unsigned color );

	UiScreen mScreen;
};

#endif
