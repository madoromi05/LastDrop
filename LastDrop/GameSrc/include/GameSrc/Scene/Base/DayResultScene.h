#ifndef INCLUDED_DAY_RESULT_SCENE_H
#define INCLUDED_DAY_RESULT_SCENE_H

#include "GameSrc/Scene/IScene.h"
#include "GameSrc/Scene/Game/Screen/ItemIcons.h"
#include "GameSrc/Scene/Game/Screen/ItemGridView.h"
#include "GameSrc/Interaction/Item/ItemGrid.h"
#include "GameSrc/Raid/DayResult.h"
#include "GameSrc/UI/UiScreen.h"

class GameResources;

// 1 日の結果の画面（脱出した後。R2_Plan.txt Q12・6-3）。「DAY 2 SURVIVED」・かかった時間と倒した数・持ち帰った物の絵と数。
// Continue で拠点へ（5 日目に脱出したなら、5 日間の終わりのスコアの画面へ）。スコアの合計はここでは見せない（最後に出す。Q5）。
// 中身は RunState の前の日の結果（RunRules::finishDay が書いた物）を読むだけ。
class DayResultScene : public IScene{
public:
	// resources: UI の道具と 5 日間の様子を借りる。所有権は受け取らない。nullptr 不可。このシーンより長く生きていること。
	explicit DayResultScene( GameResources* resources );
	virtual void beginFrame();
	virtual void update();
	virtual void draw();

private:
	// 持ち帰った物を、横に並べたマス目にする（絵と数を ItemGridView で描くため）。
	static ItemGrid shownGrid( const DayResult& result );

	UiScreen mScreen;
	ItemIcons mIcons;
	ItemGridView mView; // mIcons を借りるので、mIcons より後に宣言する
	ItemGrid mShown;    // 持ち帰った物（描くだけ。倉庫とは別の写し）
	bool mHasItems;     // 持ち帰った物があるか（無ければ「Nothing」）
};

#endif
