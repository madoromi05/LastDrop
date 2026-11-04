#ifndef INCLUDED_ITEM_GRID_VIEW_H
#define INCLUDED_ITEM_GRID_VIEW_H

#include "GameSrc/UI/UiRect.h"
#include "GameSrc/Interaction/ItemGrid.h"
#include "GameLib/Math/Vector2.h"

class UiRenderer;
class ItemIcons;

// マス目（ItemGrid）1 つを画面に描く・カーソルがどのマスかを調べる（R2_Plan.txt 5 章。2026-11-05 に InventoryScreen から分けた）。
// 町のカバンの画面（InventoryScreen）・拠点の画面（StashScreen）・1 日の結果とスコアの画面（持ち帰った物の一覧）の 3 か所で使う。
// どこに・どの大きさで置くかは使う側が setLayout で決める（町のカバンは 64、拠点の 2 つ並びは小さく）。
// 選ぶ・動かすの状態は持たない（それは画面の仕事）。描くのは積むだけで、UiRenderer::draw は使う側が 1 回だけ呼ぶ。
class ItemGridView{
public:
	// renderer / icons: 借りる。所有権は受け取らない。nullptr 不可。この部品より長く生きていること。
	ItemGridView( UiRenderer* renderer, ItemIcons* icons );

	// マス目の左上（画素。画面上の位置）と、マス 1 つの一辺（画素）。描く前に毎回決める（画面の幅は窓で変わる）。
	void setLayout( const GameLib::Math::Vector2& origin, float cellSize );
	// マスの背景（空のマス・物のあるマス）の濃さ（0〜1）。後ろの町を透かして見せるとき（町のカバンの画面。InventoryPickup_Plan.txt Q6）。
	// 物の絵と文字は濃いまま。呼ばなければ今までの濃さ（空 0.85・物 0.9）。
	void setCellAlpha( float alpha );
	// マス目の全体の幅・高さ（画素）。置く位置を決めるのに使う（setLayout の前でもよい）。
	static float width( const ItemGrid& grid, float cellSize );
	static float height( const ItemGrid& grid, float cellSize );

	UiRect cellRect( const ItemGrid& grid, int cell ) const;
	// owner のマスから size マスぶん（横）の範囲。
	UiRect spanRect( const ItemGrid& grid, int owner, int size ) const;
	// point の下のマス。無ければ -1。
	int cellAt( const ItemGrid& grid, const GameLib::Math::Vector2& point ) const;

	// マスの背景 → 物の絵 → 数の順に積む（UiRenderer は絵が変わるたびに描く回数が増えるので、背景と絵を交互に積まない）。
	// 重ねられる物は右下に数、銃は左下に「弾倉の数/弾倉の大きさ」（ItemStack::loadedRounds。空は赤。InventoryPickup_Plan.txt Q2）。
	// lifted: 持ち上げている物の中身を持つマス（そこは薄く描く。無ければ -1）。
	void drawCells( const ItemGrid& grid, int lifted );
	// マス目の外の物 stack（拾って持っている物）を、owner のマスから薄く重ねて積む（置こうとしている所に見せる）。
	void drawGhost( const ItemGrid& grid, int owner, const ItemStack& stack );
	// 物 item のマスの左上に、短い印 mark（「E」など）を積む（数と同じく暗い板の上）。item が無ければ何もしない。
	void drawMark( const ItemGrid& grid, ItemId item, const char* mark );
	// rect の縁だけを thickness の太さで積む（選んだ枠・置ける所の枠）。
	void addFrame( const UiRect& rect, float thickness, unsigned color );

private:
	// 重ねられる物の数（右下）と、銃の弾倉の数（左下）を積む（読みやすいように、数の下に暗い板を敷く）。
	void drawAmounts( const ItemGrid& grid );
	// area に item の絵を、縦横の比を保って収めて積む。絵が読めていなければ何もしない。alpha: 絵の濃さ。
	void drawIcon( const UiRect& area, ItemId item, float alpha );
	// area の角に、text を暗い板の上に積む。right: 右下（false なら左下）。
	void drawCornerText( const UiRect& area, const char* text, bool right, float plateWidth, unsigned color );

	UiRenderer* mRenderer; // 借りているだけ
	ItemIcons* mIcons;     // 借りているだけ
	GameLib::Math::Vector2 mOrigin;
	float mCellSize;
	float mEmptyAlpha; // 空のマスの背景の濃さ
	float mItemAlpha;  // 物のあるマスの背景の濃さ
};

#endif
