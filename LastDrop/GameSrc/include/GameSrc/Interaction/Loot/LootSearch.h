#ifndef INCLUDED_LOOT_SEARCH_H
#define INCLUDED_LOOT_SEARCH_H

#include "GameSrc/World/GameObjectHandle.h"

class LootContainerComponent;
class GameObject;

// 漁れる場所を開けている途中（E を押してから中を見る画面が開くまで。SummaryPlan/個別仕様/出撃と帰還/R3_Plan.txt Q6）。
// どの場所を開けているか・経った秒・かかる秒だけを持つ。何をしたら取りやめか（動く・撃つ・構える・攻撃された・E をもう一度）を決めるのは
// 呼ぶ側（GameScene が操作と出来事から interrupted を作る）。Interactor に足すと「見る係」と「待つ係」が混ざるので分けた。
// 場所は GameObjectHandle で指す（開けている間に死体が消えても、消えた物を指したままにならない）。
// 世界が止まっている間（一時停止・カバン）は tick を呼ばないので、待つ時間も止まる。
// 2026-11-05 から、脱出の車への給油（給油口を閉じた後の RaidParams::refuelSeconds）も同じ待ち方をする（FuelEscapeLoading_Plan.txt Q4・FuelPort_Plan.txt）。
// 待つ物は GameObject で受け取り、待ち終えた後に何をするか（中を見る・給油）は呼ぶ側が物の部品で決める。
class LootSearch{
public:
	// tick の答え。
	enum class Step{
		Idle,      // 開けていない
		Searching, // まだ開けている途中
		Cancelled, // このティックで取りやめになった（場所が消えた時も）
		Finished,  // このティックで開け終わった（container で場所を受け取って、中を見る画面を開く）
	};

	LootSearch();

	// target（漁れる場所・脱出の車の物）を待ち始める。seconds: かかる秒（0 より大きいこと。0 の物は待たずに使うので、呼ぶ側がここを通さない）。
	void start( const GameObject* target, float seconds );
	// 取りやめる（開けていなければ何もしない）。
	void cancel();
	bool isSearching() const;
	// 1 ティック進める。seconds: 1 ティックの秒。interrupted: このティックに取りやめになる操作・出来事があったか。
	Step tick( float seconds, bool interrupted );
	// 開けている場所（Finished を返した直後まで読める。消えた・開けていなければ nullptr）。
	LootContainerComponent* container() const;
	// 待っている物（Finished を返した直後まで読める。消えた・待っていなければ nullptr）。
	GameObject* target() const;
	// 進み具合（0〜1。画面の輪に使う）。開けていなければ 0。
	float progress() const;

private:
	GameObjectHandle mTarget;
	float mElapsed;
	float mDuration;
	bool mActive;
};

#endif
