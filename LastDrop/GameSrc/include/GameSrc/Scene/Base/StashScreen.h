#ifndef INCLUDED_STASH_SCREEN_H
#define INCLUDED_STASH_SCREEN_H

#include "GameSrc/UI/UiRect.h"
#include "GameSrc/Scene/Game/ItemIcons.h"
#include "GameSrc/Scene/Game/ItemGridView.h"
#include "GameLib/Math/Vector2.h"

class UiRenderer;
class RunState;
class ItemGrid;

// 拠点の画面（R2_Plan.txt Q6・6-1）。倉庫（6 × 8）とカバン（2 × 行）の 2 つのマス目を並べ、下に Deploy（出撃）のボタン。
// 物を選んで決定で持ち上げ、置きたいマス（隣のマス目でもよい）で決定して置く（カバンの画面の Move と同じ動き。拠点では捨てない・持たないので
// メニューは出さない）。入らない所（空きが無い・カバンに 2 丁目の銃）は赤い枠。
// 持つのは画面の入力と描画と「今どこを選んでいるか」だけ。出撃する・保存する・シーンを替えるのは BaseScene（画面から流れを触らない）。
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
	// どこを選んでいるか。Tab で 倉庫 → カバン → Deploy の順に移る。
	enum class Focus{
		Stash,
		Bag,
		Deploy,
	};

	// 2 つのマス目を画面の横の真ん中に並べる（画面の幅は窓で変わるので、読む・描く前に毎回）。
	void layout( const RunState& run );
	// 上下左右で選び直す。マス目の端からは隣のマス目・Deploy へ移る。
	void moveSelection( bool up, bool down, bool left, bool right, const RunState& run );
	// 決定。持ち上げる・置く・出撃する。
	Request decide( RunState* run );
	// 持ち上げている物を、選んでいる所へ動かせるか（同じ所は取りやめなので true）。
	bool canPlace( const RunState& run ) const;
	UiRect deployRect( const RunState& run ) const;

	static const ItemGrid& grid( const RunState& run, Focus focus );
	static ItemGrid& grid( RunState* run, Focus focus );
	ItemGridView& view( Focus focus );
	const ItemGridView& view( Focus focus ) const;

	void drawSelection( const RunState& run );
	void drawDeploy( const RunState& run );
	void drawFooter( const RunState& run );

	UiRenderer* mRenderer; // 借りているだけ
	ItemIcons mIcons;      // 物の絵（作った時に読み込みを始める）
	ItemGridView mStashView; // mIcons を借りるので、mIcons より後に宣言する
	ItemGridView mBagView;
	Focus mFocus;
	int mSelected;        // 選んでいるマス（Focus が Deploy のときは使わない）
	bool mMoving;         // 物を持ち上げているか
	Focus mMovingGrid;    // 持ち上げている物があるマス目
	int mMovingFrom;      // 持ち上げている物の中身を持つマス
	GameLib::Math::Vector2 mPreviousCursor; // 前回のカーソルの位置（動いたかどうかを決めるため）
};

#endif
