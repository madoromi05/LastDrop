#ifndef INCLUDED_RUN_STATE_H
#define INCLUDED_RUN_STATE_H

#include "GameSrc/Interaction/Item/ItemGrid.h"
#include "GameSrc/Raid/DayResult.h"
#include "GameSrc/Raid/Score.h"
#include "GameSrc/Raid/DayCondition.h"

// 5 日間の今の様子と、5 日間をまたいで残る物の持ち主（R2_Plan.txt 5 章）。起動から終了まで 1 つだけ（GameResources が持つ。GameSettings と同じ扱い）。
// ・今の回：何日目か・倉庫とカバン（ItemGrid）・ここまでに倒した数・日ごとの条件（DayCondition_Plan.txt）
// ・回をまたいで残る：ハイスコア（段階R5 で強化の段を足す）
// ・前の結果：最後の 1 日の結果（DayResult）・最後の 5 日間の終わり（RunEnd）。結果・スコアの画面が読む
// 何をどう変えるか（出撃・脱出・ゲームオーバーで何が残るか）は RunRules が決める。ここは持つのと、ファイル（save.xml）へ書く・読むだけ。
// 保存の書き方は GameSettings と同じ（PseudoXml を文字列にして fstream）。形は R2_Plan.txt 7 章。
class RunState{
public:
	static const int kDayCount = 5;        // 何日生き残ればクリアか（ユーザーの決定 2026-11-05）
	static const int kStashColumns = 6;    // 倉庫は 6 × 8（48 マス。R2_Plan.txt Q7）
	static const int kStashRows = 8;
	static const int kBagColumns = 2;      // カバンは 2 × 4（8 マス。GridInventory_Plan.txt Q1。強化で行が増えるのは段階R5）
	static const int kBagRows = 4;

	// 何も入っていない 1 日目（始めの倉庫を入れるのは RunRules::startNewRun）。
	RunState();

	// ── 今の回 ──
	int day() const;                 // 何日目か（1〜kDayCount）
	ItemGrid& stash();
	const ItemGrid& stash() const;
	ItemGrid& bag();
	const ItemGrid& bag() const;
	int kills() const;               // 前の日までに倒した数（今の日の分は脱出・ゲームオーバーで足す）
	void setDay( int day );
	void setKills( int kills );
	// 何日目にどの条件か（day: 1〜kDayCount）。5 日間の始めに RunRules が 5 日分を決めて保存する
	// （その日をやり直しても引き直せない。DayCondition_Plan.txt Q3）。
	DayCondition condition( int day ) const;
	void setCondition( int day, DayCondition condition );

	// ── 回をまたいで残る ──
	int bestScore() const;
	void setBestScore( int score );

	// ── 前の結果（画面が読む。保存はしない）──
	const DayResult& lastDay() const;
	void setLastDay( const DayResult& result );
	const RunEnd& lastRunEnd() const;
	void setLastRunEnd( const RunEnd& end );
	// 5 日間が終わって、スコアの画面をまだ見せていないか（1 日の結果の画面の Continue の行き先を決める）。
	bool isRunEndPending() const;
	void setRunEndPending( bool pending );

	// ── ファイル ──
	// save.xml から読む。ファイルが無い・壊れているときは false を返し、中身は変えない（呼ぶ側が 1 日目で始める）。
	// 読めない物の名前・マスからはみ出す物は読み飛ばす（ファイルを手で直しても止まらない）。
	bool load();
	// save.xml へ書く（save.xml.tmp に書いてから入れ替える。書く途中で落ちても前のファイルが残る）。書けなければ false（遊ぶのは続けられる）。
	bool save() const;
	// save.xml を消す（F6 の Raid 欄。次の load は false になり、1 日目から）。
	static void erase();

private:
	int mDay;
	ItemGrid mStash;
	ItemGrid mBag;
	int mKills;
	DayCondition mConditions[ kDayCount ]; // 日 − 1 で引く
	int mBestScore;
	DayResult mLastDay;
	RunEnd mLastRunEnd;
	bool mRunEndPending;
};

#endif
