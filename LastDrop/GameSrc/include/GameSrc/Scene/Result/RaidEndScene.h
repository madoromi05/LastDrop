#ifndef INCLUDED_RAID_END_SCENE_H
#define INCLUDED_RAID_END_SCENE_H

#include "GameSrc/Scene/IScene.h"
#include "GameSrc/UI/UiScreen.h"

class GameResources;

// 1 回の出撃の結果の画面（SummaryPlan/個別仕様/出撃と帰還/SimpleRun_Plan.txt Q8・Q9・C6）。クリアと死んだの両方を、この 1 つで出し分ける
// ・クリア：EscapedPage が並べる（ESCAPED・かかった時間・倒した数・今回の条件・「タイトルへ」）
// ・死んだ：GameOverPage が並べる（GAME OVER・経過と倒した数・「もう一度」（新しく引き直して町へ）と「タイトルへ」）
// 中身は RaidRecord の最後の結果（GameScene が置いた物）を読み、どちらのページを並べるかを選ぶだけ
class RaidEndScene : public IScene{
public:
	// resources: UI の道具と最後の結果を借りる。所有権は受け取らない。nullptr 不可。このシーンより長く生きていること
	explicit RaidEndScene( GameResources* resources );
	virtual void beginFrame();
	virtual void update();
	virtual void draw();

private:
	UiScreen mScreen;
};

#endif
