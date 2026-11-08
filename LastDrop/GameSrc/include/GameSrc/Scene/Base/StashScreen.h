#ifndef INCLUDED_STASH_SCREEN_H
#define INCLUDED_STASH_SCREEN_H

#include "GameSrc/UI/UiRect.h"
#include "GameSrc/Scene/Game/Screen/ItemIcons.h"
#include "GameSrc/Scene/Game/Screen/GridPairPicker.h"
#include "GameLib/Math/Vector2.h"

class UiRenderer;
class RunState;

// 拠点の画面（R2_Plan.txt Q6・6-1）。倉庫（6 × 8）とカバン（2 × 行）の 2 つのマス目を並べ、下に Deploy（出撃）のボタン。
// 物を選んで決定で持ち上げ、置きたいマス（隣のマス目でもよい）で決定して置く（カバンの画面の Move と同じ動き。拠点では捨てない・持たないので
// メニューは出さない）。入らない所（空きが無い・カバンに 2 丁目の銃）は赤い枠。
// 2 つのマス目の間で動かす部分は GridPairPicker（漁れる場所の画面と同じ係。R3_Plan.txt 5 章。2026-11-05 に分けた）。
// ここが持つのは、見出し・Deploy のボタン・下の欄と、「Deploy を選んでいるか」だけ。
// 出撃する・保存する・シーンを替えるのは BaseScene（画面から流れを触らない）。
class StashScreen{
public:
	// handleInput の答え（BaseScene にしてほしいこと）。
	enum class Request{
		None,
		Deploy, // 出撃する
	};

	// renderer: 借りる。所有権は受け取らない。nullptr 不可。この画面より長く生きていること。
	explicit StashScreen( UiRenderer* renderer );

	// ESC：持ち上げている途中なら取りやめて true。何もしていなければ false（BaseScene がタイトルへ戻る）。
	bool back();
	// 入力を読んで選ぶ・動かす。1回の描画につき1回（BaseScene の beginFrame で）。run: 倉庫とカバンを動かす先。nullptr 不可。
	Request handleInput( RunState* run );
	// 描く（中で UiRenderer::draw まで行う）。1回の描画につき1回。
	void draw( const RunState& run );

private:
	// 2 つのマス目を画面の横の真ん中に並べる（画面の幅は窓で変わるので、読む・描く前に毎回）。
	void layout( const RunState& run );
	// Tab：倉庫 → カバン → Deploy の順に移る。
	void cycleFocus( const RunState& run );
	UiRect deployRect( const RunState& run ) const;

	void drawDeploy( const RunState& run );
	void drawFooter( const RunState& run );

	UiRenderer* mRenderer; // 借りているだけ
	ItemIcons mIcons;      // 物の絵（作った時に読み込みを始める）
	GridPairPicker mPicker; // 左が倉庫・右がカバン。mIcons を借りるので、mIcons より後に宣言する
	bool mOnDeploy;         // Deploy のボタンを選んでいるか（マス目を選んでいる間は false）
	GameLib::Math::Vector2 mPreviousCursor; // 前回のカーソルの位置（動いたかどうかを決めるため）
};

#endif
