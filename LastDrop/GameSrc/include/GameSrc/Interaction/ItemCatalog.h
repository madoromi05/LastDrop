#ifndef INCLUDED_ITEM_CATALOG_H
#define INCLUDED_ITEM_CATALOG_H

// アイテムの種類。増やすときはここに1つ足し、ItemCatalog.cpp の表にも1行足す（ここを見れば全種類が分かる）。
enum ItemId{
	ITEM_FUEL,     // ガソリンの携行缶（車へ運ぶ）
	ITEM_AMMO_762, // ライフルの予備の弾（7.62 NATO。1 つが 1 発。R で弾倉へ込める）
	// 銃もカバンの 1 マスを使う（RE:2 と同じ。手に持っている銃も入っている）。どの銃かは WeaponCatalog が銃の物の種類から引く。
	// 銃は 1 種類 1 丁だけ（弾倉の中身は銃の物〈GunComponent〉が持ち続けるので、同じ銃を 2 丁持つ形は無い）。SummaryPlan/個別仕様/Pistol_Plan.txt。
	ITEM_PISTOL,
	ITEM_RIFLE,
	ITEM_AMMO_9MM, // 拳銃の予備の弾（9mm。1 つが 1 発）

	ITEM_NUMBER,
	ITEM_NONE = -1, // 物が無い（手に持っている銃が無い＝素手、など）
};

// アイテムの分類。地面に落ちたときにどの物になるか（PropFactory::spawnDroppedItem）を、物の種類ではなく分類で決めるため
// （弾や銃の種類を足しても、落とす処理を書き換えずに済むように）。
enum ItemKind{
	ITEM_KIND_FUEL,   // ガソリン（落ちているガソリンになる）
	ITEM_KIND_AMMO,   // 予備の弾（落ちた弾になる。形は ItemSpec::groundFile）
	ITEM_KIND_WEAPON, // 銃（落ちている銃になる。どの銃かは WeaponCatalog の weaponForItem）
};

// アイテム1種類の決まり。持ち方の決まり（何マス使うか・1 マスに何個重ねるか）はここにデータで持ち、Inventory・ItemGrid の処理には書かない。
// 種類を足しても、マス目の処理は変えずにこの表へ 1 行足すだけで済むようにするため。
// 何個まで持てるかは決めない（カバンの空きのマス目が上限）。SummaryPlan/個別仕様/GridInventory_Plan.txt。
struct ItemSpec{
	const char* name; // 画面に出す名前（日本語。F6 の ImGui は日本語の字を持たないので、そちらは saveName を出す）
	// 保存のファイル（save.xml）に書く名前（英小文字）。ItemId の番号で書くと、並びを変えたときに古いファイルが別の物になるため。
	// 一度決めたら変えない（変えると、その名前で保存した物が読めなくなる）。R2_Plan.txt 7 章。
	const char* saveName;
	ItemKind kind;    // 分類
	// 地面に落ちた形のファイル（ITEM_KIND_AMMO だけが使う。それ以外は nullptr）。
	// ガソリンは携行缶の形（粗い形の段付き）を PropFactory が持ち、銃は銃ごとの表（WeaponSpec::groundFile）にある。
	const char* groundFile;
	// カバンの画面に出す絵のファイル（tools/item_icon_gen.py で物の 3D モデルを撮って作る。設定は tools/configs/item_icons.json）。
	// 1 マスの物は正方形、2 マスの物は横 2 : 縦 1 の絵。SummaryPlan/個別仕様/ItemIcon_Plan.txt。
	const char* icon;
	int cellSize;     // 使うマスの数（1 か 2。2 は横に並んだ 2 マス）
	int stackSize;    // 1 マスに重ねられる数（1 なら重ねない）
	// カバンに何個まで入れられるか（0 は決めない＝空きが上限）。銃は 1（弾倉は銃の物 GunComponent が 1 つだけ持つため。R2_Plan.txt Q14）。
	// 倉庫には決まりが無い（何丁でも置ける）。始めに持っている数は持たない（始めの倉庫は RunRules の表。Q14）。
	int carryLimit;
	int scoreValue;   // 1 つあたりの点（スコアの「持ち帰った物」。弾は 1 発あたり。R2_Plan.txt Q4）
	// カバンの画面で物のマスを塗る色（0〜1）。絵が読めなかった（ファイルが無い）ときに、絵の代わりにマスを塗って物を見分けるため。
	float red;
	float green;
	float blue;
};

// 種類 id の決まり。id は ITEM_NUMBER 未満であること。
const ItemSpec& itemSpec( ItemId id );
// 保存のファイルの名前 saveName の種類（知らない名前なら ITEM_NONE）。
ItemId itemFromSaveName( const char* saveName );

#endif
