#ifndef INCLUDED_LOOT_SCREEN_H
#define INCLUDED_LOOT_SCREEN_H

#include "GameSrc/Scene/Game/ItemIcons.h"
#include "GameSrc/Scene/Game/GridPairPicker.h"
#include "GameSrc/Interaction/ItemCatalog.h"
#include "GameLib/Math/Vector2.h"

class UiRenderer;
class ItemGrid;

// 漁れる場所の中を見る画面（SummaryPlan/個別仕様/R3_Plan.txt Q8・Q9・6-2）。左にカバン、右に場所の中身を並べる。
// 動かし方は拠点の画面と同じ（GridPairPicker：決定で持ち上げ → 置く。F で反対のマス目へおまかせで移す）。
// カバンから場所へ置いていくこともできる（その日のうちは場所に残る）。カバンに 2 丁目の銃は入らない（赤い枠）。
// 持つのは画面の入力と描画と「今どこを選んでいるか」だけ。世界と音を止める・閉じるのは本編（GameScene。カバンの画面と同じ分け方）。
class LootScreen{
public:
	// renderer: 借りる。所有権は受け取らない。nullptr 不可。この画面より長く生きていること。
	explicit LootScreen( UiRenderer* renderer );

	// 開くたびに、場所の中身の左上を選んだ状態から見せる（F を押せばすぐカバンへ移せるように）。
	// lootRule: 右のマス目に入れてよい物（漁れる場所は Free・脱出の車の給油口は FuelOnly）。
	void open( GridRule lootRule );
	// ESC：持ち上げている途中なら取りやめて true。何もしていなければ false（本編が閉じる）。
	bool back();
	// 入力を読んで選ぶ・動かす。開いている間、1回の描画につき1回（本編の beginFrame で）。どれも nullptr 不可。
	// bag: カバンのマス目（Inventory::gridForTransfer）。loot: 場所の中身（LootContainerComponent::lootGrid）。
	void handleInput( ItemGrid* bag, ItemGrid* loot );
	// 暗くする板と 2 つのマス目を描く（中で UiRenderer::draw まで行う。同じ描画で HUD は描かないこと）。
	// title: 場所の名前（右の見出し）。equipped: 手に持っている銃（カバンのそのマスに「E」の印。素手なら ITEM_NONE）。
	void draw( const ItemGrid& bag, const ItemGrid& loot, const char* title, ItemId equipped );

private:
	void layout( const ItemGrid& bag, const ItemGrid& loot );
	void drawFooter( const ItemGrid& bag, const ItemGrid& loot );

	UiRenderer* mRenderer; // 借りているだけ
	ItemIcons mIcons;      // 物の絵（作った時に読み込みを始める）
	GridPairPicker mPicker; // 左がカバン・右が場所の中身。mIcons を借りるので、mIcons より後に宣言する
	GameLib::Math::Vector2 mPreviousCursor; // 前回のカーソルの位置（動いたかどうかを決めるため）
};

#endif
