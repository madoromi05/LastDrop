#ifndef INCLUDED_STASH_SCREEN_H
#define INCLUDED_STASH_SCREEN_H

#include "GameSrc/UI/UiRect.h"
#include "GameSrc/Scene/Game/Screen/ItemIcons.h"
#include "GameSrc/Scene/Game/Screen/GridPairPicker.h"
#include "GameLib/Math/Vector2.h"

class UiRenderer;
class ItemGrid;

// 倉庫の画面（R2_Plan.txt Q6・6-1）。倉庫（6 × 8）とカバン（2 × 行）の 2 つのマス目を並べ、下に Deploy（出撃）のボタン
// 物を選んで決定で持ち上げ、置きたいマス（隣のマス目でもよい）で決定して置く（カバンの画面の Move と同じ動き。ここでは捨てない・持たないので
// メニューは出さない）。入らない所（空きが無い・カバンに 2 丁目の銃）は赤い枠
// 2 つのマス目の間で動かす部分は GridPairPicker（漁れる場所の画面と同じ係）
// ここが持つのは、見出し・Deploy のボタン・下の欄と、「Deploy を選んでいるか」だけ。出撃する・シーンを替えるのは呼ぶ側
//
// 拠点のシーンが無いので、この画面は「コードだけ残す」
// 今はどこからも開かない。戻すときは、倉庫とカバンの ItemGrid を持つ所と、この画面を開くシーンを足す
// 何日目か・ハイスコア・その日の条件の文字は出さない（2 つのマス目だけを受け取る）
class StashScreen{
public:
	static const int kStashColumns = 6; // 倉庫は 6 × 8（48 マス）。倉庫を作る側が使う
	static const int kStashRows = 8;

	// handleInput の答え（呼ぶ側にしてほしいこと）
	enum class Request{
		None,
		Deploy, // 出撃する
	};

	// renderer: 借りる。所有権は受け取らない。nullptr 不可。この画面より長く生きていること
	explicit StashScreen( UiRenderer* renderer );

	// ESC：持ち上げている途中なら取りやめて true。何もしていなければ false（呼ぶ側が画面を閉じる）
	bool back();
	// 入力を読んで選ぶ・動かす。1回の描画につき1回（シーンの beginFrame で）。stash・bag: 物を動かす先。nullptr 不可
	Request handleInput( ItemGrid* stash, ItemGrid* bag );
	// 描く（中で UiRenderer::draw まで行う）。1回の描画につき1回
	void draw( const ItemGrid& stash, const ItemGrid& bag );

private:
	// 2 つのマス目を画面の横の真ん中に並べる（画面の幅は窓で変わるので、読む・描く前に毎回）
	void layout( const ItemGrid& stash, const ItemGrid& bag );
	// Tab：倉庫 → カバン → Deploy の順に移る
	void cycleFocus();
	UiRect deployRect( const ItemGrid& bag ) const;

	void drawDeploy( const ItemGrid& bag );
	void drawFooter( const ItemGrid& stash, const ItemGrid& bag );

	UiRenderer* mRenderer; // 借りているだけ
	ItemIcons mIcons;      // 物の絵（作った時に読み込みを始める）
	GridPairPicker mPicker; // 左が倉庫・右がカバン。mIcons を借りるので、mIcons より後に宣言する
	bool mOnDeploy;         // Deploy のボタンを選んでいるか（マス目を選んでいる間は false）
	GameLib::Math::Vector2 mPreviousCursor; // 前回のカーソルの位置（動いたかどうかを決めるため）
};

#endif
