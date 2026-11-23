#ifndef INCLUDED_TOWN_EDIT_RELOAD_H
#define INCLUDED_TOWN_EDIT_RELOAD_H

#include "GameSrc/Raid/DayCondition.h"

class RaidRecord;

// F6 の Town タブが今の出撃を読み直し（Apply・Bake・Bake all）、読み直した出撃を読み終えるまで待つ
// 読み直す出撃の日の条件と車の置き場所は、RaidRecord の次の 1 回だけの決め事（overrideNextRaid）で渡す
// 決め事は次に作る出撃が使って消し、出撃でないシーンに替わったら SceneManager が捨てる。F6 の条件の固定は触らないので、戻す物は無い
// 読み終えたかは、決め事が使われた後に出撃の世界が F6 へ渡ったかで見る（GameScene は読み終えてから渡す）
// Debug・Profile ビルドだけ（Release では中身をビルドしない）
class TownEditReload{
public:
	enum class Result{
		Idle,		// 待っていない
		Waiting,	// 読み直した出撃をまだ読み終えていない
		Loaded,		// 頼んだ車の置き場所の出撃を読み終えた。この呼び出しで待つのをやめる
		Replaced,	// 読み終えた出撃の車が頼んだ置き場所と違う（読み直しの予約が別の切り替えに替わった）。この呼び出しで待つのをやめる
	};

	TownEditReload();

	// record へ次の 1 回の出撃の condition と carSpot を置き、待ち始める。出撃を読み直す予約は呼ぶ側がする（SceneManager::requestRetryRaid）
	// 待っている間（isPending）は呼ばない。record: nullptr 不可。carSpot: 0 以上
	void begin( RaidRecord* record, DayCondition condition, int carSpot );
	bool isPending() const;
	// 読み直しが済んだかを見る（F6 が開いている間、毎回呼ぶ）。待っていなければ Idle
	// raidLoaded: 出撃の世界を読み終えて F6 へ渡っているか。loadedSpot: その出撃の車の置き場所（raidLoaded のときだけ読む）
	Result poll( const RaidRecord& record, bool raidLoaded, int loadedSpot );

private:
	bool mPending;
	int mCarSpot;	// begin で頼んだ車の置き場所
};

#endif
