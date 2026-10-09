#ifndef INCLUDED_ITEM_CATALOG_H
#define INCLUDED_ITEM_CATALOG_H

// アイテムの種類。増やすときはここに1つ足し、ItemCatalog.cpp の表にも1行足す（ここを見れば全種類が分かる）。
enum ItemId{
	ITEM_FUEL, // ガソリンの携行缶（車へ運ぶ）

	ITEM_NUMBER,
};

// アイテム1種類の決まり。持ち方の決まり（何個まで持てるか）はここにデータで持ち、Inventory の処理には書かない。
// インベントリを作り直しても（マス目・重さなど）、種類ごとの決まりはここに足すだけで済むようにするため。
struct ItemSpec{
	const char* name; // 画面に出す名前（英語）
	int maxCarry;     // 同時に持てる数
};

// 種類 id の決まり。id は ITEM_NUMBER 未満であること。
const ItemSpec& itemSpec( ItemId id );

#endif
