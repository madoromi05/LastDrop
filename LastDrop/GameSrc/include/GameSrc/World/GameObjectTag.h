#ifndef INCLUDED_GAME_OBJECT_TAG_H
#define INCLUDED_GAME_OBJECT_TAG_H

// 物（GameObject）が何の物か。ゲームの判断（数の上限・確認用の表示を物ごとに分ける）はこれで行い、名前（文字列）では行わない（★6）。
// 同じ部品の組み合わせを、違う物として見分けるのにも使う
// （落ちているガソリンとゾンビが落とした弾は、どちらも部品は StaticModel・BoxCollider・Item・Pickup）。
// このゲームに出てくる物の種類の一覧なので、物の種類を足すときはここにも足す。
enum class GameObjectTag{
	Player,      // 自キャラ
	Weapon,      // 自キャラが持つ銃（"AK47"）
	Zombie,      // ゾンビ
	Fuel,        // 落ちているガソリン（拾うと消える）
	AmmoDrop,    // ゾンビが落とした予備の弾（拾うと消える）
	EscapeCar,   // 脱出用の車
	FuelStation, // 車の近くの、何度でも拾えるガソリンの置き場
};

#endif
