#ifndef INCLUDED_INVENTORY_CURSOR_H
#define INCLUDED_INVENTORY_CURSOR_H

#include "GameSrc/Interaction/Item/ItemCatalog.h"
#include "GameSrc/Interaction/Item/ItemGrid.h"

class Inventory;

// カバンの画面の「今どこを選んで、何をしている途中か」（SummaryPlan/個別仕様/設計と分割/MidPrioritySplit_Plan.txt 7 章）。
//   Browse  … マスを選んでいる
//   Menu    … 選んだ物のメニュー（装備 / 使う / 捨てる / 動かす / やめる のうち、その物に使える物）を出している
//   Moving  … 物を持ち上げ、置く所を選んでいる
//   Placing … 拾った物（マス目の外）を持ち、入れる所を選んでいる
// 描かない・入力デバイスを読まない（画面の InventoryScreen が入力を読み、マウスの下のマス・項目を言ってくる）。
// 捨てる・持ち替える・使う・拾った物を入れるは本編への頼み（Request）で返す。動かす（並べ替え）はカバンの中だけで済むので、ここから Inventory に頼む。
class InventoryCursor{
public:
	// 本編（BagController）への頼み。
	struct Request{
		enum Kind{
			NONE,  // 何もしない
			DROP,  // cell の物を取り出して足元に置く
			EQUIP, // cell の銃を手に持つ
			USE,   // cell の体力を戻す物（包帯）を 1 つ使う
			PLACE, // 拾った物を cell を指した所へ入れる（入る数は本編が Inventory::countPlaceableInto で決める）
			QUICK_PLACE, // 拾った物をおまかせで入れる
		};
		Kind kind;
		int cell; // 物の中身を持つマス（NONE なら使わない）
	};
	enum class Mode{
		Browse,
		Menu,
		Moving,
		Placing,
	};
	// メニューの項目（全部）。物ごとに、使える物だけを上からこの順に並べる（openMenu）。文字は画面が持つ。
	enum MenuItem{
		MENU_EQUIP, // 銃を手に持つ（手に持っていない銃だけ）
		MENU_USE,   // 包帯を使う（体力が減っているときだけ）
		MENU_DROP,
		MENU_MOVE,
		MENU_CANCEL,
		MENU_ITEM_NUMBER,
	};
	static const int kNoCell = -1;

	InventoryCursor();

	// ── 開く・戻る ──
	// 左上のマスを選んだ状態から始める（カバンを開いたとき）。
	void open();
	// 拾った物 incoming を持った状態で始める。入る所があれば、その最初のマスを選んでおく。
	void openForPlacing( const Inventory& inventory, const ItemStack& incoming );
	// ESC：拾った物を持っている・メニュー・動かしている途中なら取りやめて true。何もしていなければ false（本編がカバンを閉じる）。
	bool back();
	// マス目が外から変わった（F6 で行を減らした）ときに、無くなったマス・物を指していたら選び直す・取りやめる。
	void forgetStale( const ItemGrid& grid );
	// 拾う物が無くなった（全部入れた）ら、拾った物を持っている状態をやめる。incoming: 拾って持っている物（無ければ nullptr）。
	void endPlacingIfEmpty( const ItemStack* incoming );

	// ── マスを選ぶ（Browse・Moving・Placing）──
	// cell を選ぶ（マウスの下のマス）。kNoCell なら何もしない。
	void selectCell( int cell );
	// 上下左右でマスを選び直す（端で止まる）。
	void moveSelection( bool up, bool down, bool left, bool right, const ItemGrid& grid );
	// 選んだマスで決定した。Placing なら入れる頼み（入らない所なら何もしない）、Moving なら置く、Browse なら物のメニューを出す。
	// equipped: 手に持っている銃の物（Equip を出すか）。injured: 体力が減っているか（Use を出すか）。incoming: 拾って持っている物（Placing の間）。
	Request decide( Inventory* inventory, ItemId equipped, bool injured, const ItemStack* incoming );

	// ── メニュー（Menu）──
	void selectMenu( int index );   // マウスの下の項目を選ぶ
	void menuUp();
	void menuDown();
	void closeMenu();               // メニューの外を押した
	// 選んでいる項目を行う。捨てる・持つ・使うなら、本編への頼みを返す。
	Request activateMenu( const ItemGrid& grid );

	// ── 読み口（描く・当たりを決めるのに使う）──
	Mode mode() const;
	bool isPlacing() const;
	int selected() const;      // 選んでいるマス
	int movingFrom() const;    // 持ち上げている物の中身を持つマス（Moving の間。それ以外は kNoCell）
	int menuCount() const;
	MenuItem menuItem( int index ) const;
	int menuChoice() const;    // メニューで選んでいる項目（何番目か）

private:
	// 選んでいる物のメニューを出す（その物に使える項目だけを並べる）。
	void openMenu( const ItemGrid& grid, ItemId equipped, bool injured );
	// 置く所を決めた。動かせたら（取りやめも）Browse へ戻る。入らない所なら何もしない。
	void place( Inventory* inventory );

	Mode mMode;
	int mSelected;
	static const int kMenuCapacity = MENU_ITEM_NUMBER;
	MenuItem mMenuItems[ kMenuCapacity ]; // 出しているメニューの項目
	int mMenuCount;
	int mMenuChoice;
	int mMovingFrom;
};

#endif
