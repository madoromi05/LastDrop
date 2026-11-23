#ifndef INCLUDED_INVENTORY_SCREEN_H
#define INCLUDED_INVENTORY_SCREEN_H

#include "GameSrc/UI/UiRect.h"
#include "GameSrc/Scene/Game/Screen/ItemIcons.h"
#include "GameSrc/Scene/Game/Screen/ItemGridView.h"
#include "GameSrc/Interaction/Item/ItemCatalog.h"
#include "GameSrc/Interaction/Item/ItemGrid.h"
#include "GameLib/Math/Vector2.h"
#include "GameSrc/Scene/Game/Screen/EcgView.h"
#include "GameSrc/Scene/Game/Screen/InventoryCursor.h"

class UiRenderer;
class Inventory;
enum class PlayerCondition;

// カバンの画面（Tab）。止めた本編の画の上に半透明の黒を重ね、マス目（2 列 × 行）と、選んだ物のメニュー（Equip / Drop / Move / Cancel）を出す
// Equip は銃だけに出す（手に持っている銃には出さない）。手に持っている銃のマスには「E」の印
// 町で E で拾うと、拾った物を持った状態で開き、入れるマスを選ぶ
// 後ろの町が透けるよう、暗くする板とマスの背景は半透明
// 物は絵（ItemIcons）で見せ、重ねられる物だけ数を右下に、銃は弾倉の数を左下に出す。名前は選んだ物だけを下の欄に出す
// 持つのは画面の入力と描画だけ。「今どこを選んで、何をしている途中か」は InventoryCursor
// 世界は触らない：世界・音を止める・動かし直すのと、捨てた物を足元に置く・銃を持ち替えるのは本編（GameScene）
// （PauseMenu と同じ分け方。画面から世界を触らない）。動かす（並べ替え）はカバンの中だけで済むので、ここから Inventory に頼む
class InventoryScreen{
public:
	using Request = InventoryCursor::Request;	// 本編への頼み（中身は InventoryCursor::Request。BagController が InventoryScreen::Request の形で読む）

	explicit InventoryScreen( UiRenderer* renderer );	// renderer: 借りる。所有権は受け取らない。nullptr 不可。この画面より長く生きていること

	void open();																	// 開くたびに、左上のマスを選んだ状態から見せる（本編が開いたときに呼ぶ）
	void openForPlacing( const Inventory& inventory, const ItemStack& incoming );	// 拾った物 incoming を持った状態で始める（本編が open の後に呼ぶ）。入る所があれば、その最初のマスを選んでおく
	bool isPlacing() const;															// 拾った物を持っているか（ESC で取りやめたら false）
	bool back();																	// ESC：拾った物を持っている・メニュー・動かしている途中なら取りやめて true（拾うのをやめても画面は開いたまま）。何もしていなければ false（本編がカバンを閉じる）
	// 入力を読んで選ぶ・動かす。開いている間、1回の描画につき1回（本編の beginFrame で）
	// 「Drop」「Equip」を選んだら、そのマス（物の中身を持つマス）を返す。取り出して足元に置く・持ち替えるのは本編
	// inventory: 動かす先。nullptr 不可。equipped: 手に持っている銃の物（素手なら ITEM_NONE。Equip を出すかを決める）
	// injured: 体力が減っているか（包帯に Use を出すかを決める）
	// incoming: 拾って持っている物（isPlacing の間。それ以外は nullptr）
	Request handleInput( Inventory* inventory, ItemId equipped, bool injured, const ItemStack* incoming );
	// 暗くする板とマス目を描く。開いている間、1回の描画につき1回。本編の 3D を描いた後に呼ぶ（上に重ねる）
	// 中で UiRenderer::draw まで行うので、同じ描画で HUD・一時停止のメニューは描かないこと（UiRenderer は 1 回の描画に 1 回だけ）
	// equipped: 手に持っている銃の物（素手なら ITEM_NONE。そのマスに「E」の印を出す）
	// exits: 見出しの下に出す、開いている脱出点の文字（「Exits: North / East」）。nullptr なら出さない
	// incoming: 拾って持っている物（isPlacing の間。それ以外は nullptr）
	void draw( const Inventory& inventory, ItemId equipped, const char* exits, const ItemStack* incoming, PlayerCondition condition );

private:
	void layout( const ItemGrid& grid );											// マス目を画面の真ん中に置く（ItemGridView へ渡す。画面の幅は窓で変わるので、読む・描く前に毎回）
	// ── 並べる位置（画素。画面上の位置。UiAnchor の中央を足し済み）──
	UiRect menuRect( const ItemGrid& grid, int index ) const;
	int menuAt( const ItemGrid& grid, const GameLib::Math::Vector2& point ) const;	// point の下のメニューの項目。無ければ -1

	void drawSelection( const Inventory& inventory, const ItemStack* incoming );
	void drawMenu( const ItemGrid& grid );
	void drawFooter( const ItemGrid& grid, const ItemStack* incoming );

	UiRenderer* mRenderer; // 借りているだけ
	ItemIcons mIcons;      // 物の絵（作った時に読み込みを始める）
	ItemGridView mView;    // マス目を描く（mIcons を借りるので、mIcons より後に宣言する）
	EcgView mEcg;		   // 左下に心電図
	InventoryCursor mCursor; // 今どこを選んで、何をしている途中か
	GameLib::Math::Vector2 mPreviousCursor; // 前回のカーソルの位置（動いたかどうかを決めるため）
};

#endif
