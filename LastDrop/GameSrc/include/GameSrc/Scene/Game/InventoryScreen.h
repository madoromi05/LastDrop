#ifndef INCLUDED_INVENTORY_SCREEN_H
#define INCLUDED_INVENTORY_SCREEN_H

#include "GameSrc/UI/UiRect.h"
#include "GameSrc/Scene/Game/ItemIcons.h"
#include "GameSrc/Interaction/ItemCatalog.h"
#include "GameLib/Math/Vector2.h"

class UiRenderer;
class Inventory;
class ItemGrid;

// カバンの画面（Tab）。止めた本編の画の上に半透明の黒を重ね、マス目（2 列 × 行）と、選んだ物のメニュー（Equip / Drop / Move / Cancel）を出す。
// Equip は銃だけに出す（手に持っている銃には出さない）。手に持っている銃のマスには「E」の印（Pistol_Plan.txt Q2）。
// 物は絵（ItemIcons）で見せ、重ねられる物だけ数を右下に出す。名前は選んだ物だけを下の欄に出す（SummaryPlan/個別仕様/ItemIcon_Plan.txt）。
// 持つのは画面の入力と描画と「今どこを選んでいるか」だけ。世界・音を止める・動かし直すのと、捨てた物を足元に置く・銃を持ち替えるのは本編（GameScene）
// （PauseMenu と同じ分け方。画面から世界を触らない）。動かす（並べ替え）はカバンの中だけで済むので、ここから Inventory に頼む。
// SummaryPlan/個別仕様/GridInventory_Plan.txt。
class InventoryScreen{
public:
	// handleInput の答え（本編にしてほしいこと）。
	struct Request{
		enum Kind{
			NONE,  // 何もしない
			DROP,  // cell の物を取り出して足元に置く
			EQUIP, // cell の銃を手に持つ
		};
		Kind kind;
		int cell; // 物の中身を持つマス（NONE なら使わない）
	};

	// renderer: 借りる。所有権は受け取らない。nullptr 不可。この画面より長く生きていること。
	explicit InventoryScreen( UiRenderer* renderer );

	// 開くたびに、左上のマスを選んだ状態から見せる（本編が開いたときに呼ぶ）。
	void open();
	// ESC：メニュー・動かしている途中なら取りやめて true。何もしていなければ false（本編がカバンを閉じる）。
	bool back();
	// 入力を読んで選ぶ・動かす。開いている間、1回の描画につき1回（本編の beginFrame で）。
	// 「Drop」「Equip」を選んだら、そのマス（物の中身を持つマス）を返す。取り出して足元に置く・持ち替えるのは本編。
	// inventory: 動かす先。nullptr 不可。equipped: 手に持っている銃の物（素手なら ITEM_NONE。Equip を出すかを決める）。
	Request handleInput( Inventory* inventory, ItemId equipped );
	// 暗くする板とマス目を描く。開いている間、1回の描画につき1回。本編の 3D を描いた後に呼ぶ（上に重ねる）。
	// 中で UiRenderer::draw まで行うので、同じ描画で HUD・一時停止のメニューは描かないこと（UiRenderer は 1 回の描画に 1 回だけ）。
	// equipped: 手に持っている銃の物（素手なら ITEM_NONE。そのマスに「E」の印を出す）。
	void draw( const Inventory& inventory, ItemId equipped );

private:
	enum class Mode{
		Browse, // マスを選んでいる
		Menu,   // 選んだ物のメニュー（Equip / Drop / Move / Cancel のうち、その物に使える物）を出している
		Moving, // 物を持ち上げ、置く所を選んでいる
	};

	// マス目が外から変わった（F6 で行を減らした）ときに、無くなったマス・物を指していたら選び直す・取りやめる。
	void forgetStale( const ItemGrid& grid );
	// 上下左右でマスを選び直す（端で止まる）。
	void moveSelection( bool up, bool down, bool left, bool right, const ItemGrid& grid );
	// 選んでいる物のメニューを出す（その物に使える項目だけを並べる）。
	void openMenu( const ItemGrid& grid, ItemId equipped );
	// メニューの選んだ項目を行う。捨てる・持つなら、本編への答えを返す。
	Request activateMenu( const ItemGrid& grid );
	// 置く所を決めた。動かせたら（取りやめも）Browse へ戻る。入らない所なら何もしない。
	void place( Inventory* inventory );

	// ── 並べる位置（画素。画面上の位置。UiAnchor の中央を足し済み）──
	UiRect cellRect( const ItemGrid& grid, int cell ) const;
	// owner のマスから size マスぶん（横）の範囲。
	UiRect spanRect( const ItemGrid& grid, int owner, int size ) const;
	UiRect menuRect( const ItemGrid& grid, int index ) const;
	// point の下のマス。無ければ -1。
	int cellAt( const ItemGrid& grid, const GameLib::Math::Vector2& point ) const;
	// point の下のメニューの項目。無ければ -1。
	int menuAt( const ItemGrid& grid, const GameLib::Math::Vector2& point ) const;

	// マスの背景 → 物の絵 → 数の順に積む（UiRenderer は絵が変わるたびに描く回数が増えるので、背景と絵を交互に積まない）。
	void drawCells( const ItemGrid& grid );
	// 重ねられる物の数を、マスの右下に積む（読みやすいように、数の下に暗い板を敷く）。
	void drawAmounts( const ItemGrid& grid );
	// 手に持っている銃 equipped のマスの左上に「E」の印を積む（数と同じく暗い板の上）。
	void drawEquippedMark( const ItemGrid& grid, ItemId equipped );
	void drawSelection( const Inventory& inventory );
	void drawMenu( const ItemGrid& grid );
	void drawFooter( const ItemGrid& grid );
	// rect の縁だけを thickness の太さで積む。
	void addFrame( const UiRect& rect, float thickness, unsigned color );

	UiRenderer* mRenderer; // 借りているだけ
	ItemIcons mIcons;      // 物の絵（作った時に読み込みを始める）
	Mode mMode;
	int mSelected;   // 選んでいるマス
	// 出しているメニューの項目（MenuItem の番号を並べた物）と数。項目は物によって変わる（Equip は銃だけ）。
	static const int kMenuCapacity = 4;
	int mMenuItems[ kMenuCapacity ];
	int mMenuCount;
	int mMenuChoice; // メニューで選んでいる項目（mMenuItems の何番目か）
	int mMovingFrom; // 持ち上げている物の中身を持つマス（Moving の間）
	GameLib::Math::Vector2 mPreviousCursor; // 前回のカーソルの位置（動いたかどうかを決めるため）
};

#endif
