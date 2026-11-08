#ifndef INCLUDED_GRID_PAIR_PICKER_H
#define INCLUDED_GRID_PAIR_PICKER_H

#include "GameSrc/Scene/Game/Screen/ItemGridView.h"
#include "GameSrc/Interaction/Item/ItemTransfer.h"
#include "GameLib/Math/Vector2.h"

class UiRenderer;
class ItemIcons;

// 左右に並べた 2 つのマス目の間で、物を「選ぶ・持ち上げる・置く・F でおまかせで移す」係（SummaryPlan/個別仕様/R3_Plan.txt 5 章）。
// 拠点の画面（StashScreen：倉庫 ⇔ カバン）と、漁れる場所の画面（LootScreen：カバン ⇔ 場所の中身）が使う。
// 2026-11-05 に StashScreen の中から分けた（同じ動きを 2 つの画面に書くと、直すときに片方だけ直す漏れが起きるため）。
// 持つのは「どちらの・どのマスを選んでいるか」「何を持ち上げているか」と、2 つのマス目を描く ItemGridView だけ。
// マス目の中身は持たない（呼ぶたびに借りる）。入れてよい物の決まり（GridRule）は作るときに左右ごとに決める。
// 見出し・ボタン・下の欄は使う側の画面が描く。UiRenderer::draw も使う側が 1 回だけ呼ぶ。
class GridPairPicker{
public:
	enum class Side{
		Left,
		Right,
	};
	// moveSelection の答え（マス目の外へ出ようとした所。使う側がボタンへ移すなどに使う）。
	enum class Exit{
		None,
		BelowRight, // 右のマス目の一番下で「下」
	};

	// renderer / icons: 借りる。所有権は受け取らない。nullptr 不可。この係より長く生きていること。
	// leftRule / rightRule: 左右のマス目に入れてよい物の決まり。
	GridPairPicker( UiRenderer* renderer, ItemIcons* icons, GridRule leftRule, GridRule rightRule );

	// マスの背景の濃さ（ItemGridView::setCellAlpha。町の中の画面は後ろを透かす）。
	void setCellAlpha( float alpha );
	// 右のマス目の決まりを変える（1 つの画面で、漁れる場所と脱出の車の給油口を見せ分けるため）。
	void setRightRule( GridRule rule );
	// 2 つのマス目の左上とマスの一辺（画素）。読む・描く前に毎回（画面の幅は窓で変わる）。
	void layout( const GameLib::Math::Vector2& leftOrigin, const GameLib::Math::Vector2& rightOrigin, float cellSize );
	const ItemGridView& view( Side side ) const;
	// 使う側が印（手に持っている銃の「E」など）を積むときだけ使う。
	ItemGridView& view( Side side );

	// side の cell を選び、持ち上げていれば取りやめる（画面を開いたとき）。
	void reset( Side side, int cell );
	// 選ぶ所だけを変える（持ち上げたまま。拠点の画面で Deploy から戻るとき）。
	void select( Side side, int cell );
	// 持ち上げている途中なら取りやめて true。何もしていなければ false（使う側が画面を閉じる・戻る）。
	bool back();
	bool isMoving() const;
	Side focus() const;
	int selected() const;

	// 中身が外から変わった（F6・別の画面）ときに、無くなったマス・物を指していたら選び直す・取りやめる。毎回の入力の頭で呼ぶ。
	void forgetStale( const ItemGrid& left, const ItemGrid& right );
	// cursor の下のマスを選ぶ。マスの上なら true（マウスが動いたときに使う側が呼ぶ）。
	bool point( const ItemGrid& left, const ItemGrid& right, const GameLib::Math::Vector2& cursor );
	// 上下左右で選び直す。左右の端からは隣のマス目へ移る（同じ行。行の数が違えば収める）。
	Exit moveSelection( bool up, bool down, bool left, bool right, const ItemGrid& leftGrid, const ItemGrid& rightGrid );
	// 決定：持ち上げる（物があるマス）・置く（入らない所なら何もしない）・元の場所なら取りやめ。
	void decide( ItemGrid* left, ItemGrid* right );
	// F：選んでいる物を、反対のマス目のおまかせの所へ移す（持ち上げている間は使わない）。動いたら true。
	bool quickMove( ItemGrid* left, ItemGrid* right );

	// 下の欄に名前を出す物（持ち上げている物、無ければ選んでいる物。空のマスなら nullptr）。
	const ItemStack* focusedStack( const ItemGrid& left, const ItemGrid& right ) const;
	// 2 つのマス目と、選んでいる枠（持ち上げている間は置く所の枠。入らない所は赤）を積む。
	// showSelection: 枠を出すか（拠点の画面で Deploy を選んでいる間は出さない）。
	void draw( const ItemGrid& left, const ItemGrid& right, bool showSelection );

private:
	const ItemGrid& gridOf( const ItemGrid& left, const ItemGrid& right, Side side ) const;
	ItemGrid* gridOf( ItemGrid* left, ItemGrid* right, Side side ) const;
	GridRule ruleOf( Side side ) const;
	ItemGridView& viewOf( Side side );
	// 持ち上げている物を、選んでいる所へ置けるか（元の場所は取りやめなので true）。
	bool canPlace( const ItemGrid& left, const ItemGrid& right ) const;
	bool isLiftedSpot( const ItemGrid& left, const ItemGrid& right ) const;

	ItemGridView mLeftView;
	ItemGridView mRightView;
	GridRule mLeftRule;
	GridRule mRightRule;
	Side mFocus;
	int mSelected;     // 選んでいるマス
	bool mMoving;      // 物を持ち上げているか
	Side mMovingSide;  // 持ち上げている物があるマス目
	int mMovingFrom;   // 持ち上げている物の中身を持つマス
};

#endif
