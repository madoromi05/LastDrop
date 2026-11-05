#ifndef INCLUDED_BASE_SCENE_H
#define INCLUDED_BASE_SCENE_H

#include "GameSrc/Scene/IScene.h"
#include "GameSrc/Scene/Base/StashScreen.h"
#include "GameSrc/Scene/LoadingScreen.h"

class GameResources;

// 拠点（出撃の合間にいる所。RaidLoop Q2 の★A：UI だけのメニュー画面）。R2_Plan.txt Q6。
// 倉庫とカバンの間で物を動かして持って行く物を決め（StashScreen）、Deploy で町へ（RunRules::deploy の後に本編へ）。
// ESC でタイトルへ（出撃していないので何も失わない。動かした物は保存してから戻る）。
// 強化（カバン・倉庫を広げる）は段階R5 でここに足す。
class BaseScene : public IScene{
public:
	// resources: UI の道具と 5 日間の様子（RunState）を借りる。所有権は受け取らない。nullptr 不可。このシーンより長く生きていること。
	explicit BaseScene( GameResources* resources );
	// 動かした物を save.xml へ書く（タイトルへ戻る・窓を閉じる・出撃のどれでも）。
	~BaseScene();
	virtual void beginFrame();
	virtual void update();
	virtual void draw();

private:
	GameResources* mResources; // 借りているだけ
	StashScreen mScreen;
	// 出撃を押したフレームから、ロード画面を出す（本編を作る間〈シェーダーのコンパイルなど〉に倉庫の画面のまま止まって見えないように。
	// FuelEscapeLoading_Plan.txt 6 章）。出撃を押した次の beginFrame で本編への切り替えを頼む（押したフレームにロード画面を 1 回描くため）。
	LoadingScreen mLoadingScreen;
	bool mDeploying;
};

#endif
