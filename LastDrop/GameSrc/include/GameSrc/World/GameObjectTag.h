#ifndef INCLUDED_GAME_OBJECT_TAG_H
#define INCLUDED_GAME_OBJECT_TAG_H

// 物（GameObject）が何の物か。ゲームの判断（数の上限・確認用の表示を物ごとに分ける）はこれで行い、名前（文字列）では行わない（★6）。
// 同じ部品の組み合わせを、違う物として見分けるのにも使う
// （落ちているガソリンとゾンビが落とした弾は、どちらも部品は StaticModel・BoxCollider・Item・Pickup）。
// このゲームに出てくる物の種類の一覧なので、物の種類を足すときはここにも足す。
enum class GameObjectTag{
	Player,      // 自キャラ
	Weapon,      // 自キャラが持てる銃（拳銃・ライフル。捨てても消えず、弾倉の中身を持ち続ける）
	Zombie,      // ゾンビ
	Fuel,        // 落ちているガソリン（拾うと消える）
	AmmoDrop,    // 地面に落ちている予備の弾（カバンから捨てた弾。拾うと消える。2026-11-05 からゾンビは落とさず、死体を漁る）
	WeaponDrop,  // 地面に落ちている銃（カバンから捨てた銃。拾うと消える）
	ItemDrop,    // 地面に落ちている包帯・売れる物（カバンから捨てた物。拾うと消える。R3_Plan.txt）
	LootContainer, // 漁れる置物（ゴミ箱・木箱。動かない。R3_Plan.txt Q1）
	EscapeCar,   // 今日の脱出の車（ガソリンを入れる。漁れない。FuelEscapeLoading_Plan.txt）
	FireHydrant, // 消火栓（見た目の置物。動かない箱で移動を止める。使えない）
};

#endif
