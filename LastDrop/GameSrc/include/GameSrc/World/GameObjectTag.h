#ifndef INCLUDED_GAME_OBJECT_TAG_H
#define INCLUDED_GAME_OBJECT_TAG_H

// 物（GameObject）が何の物か。同じ部品のクラスを、違う物として見分けるのに使う
// （落ちているガソリンとゾンビが落とした弾は、どちらも部品は Pickup。数の上限・確認用の表示は物ごとに分ける）。
// このゲームに出てくる物の種類の一覧なので、物の種類を足すときはここにも足す。
enum class GameObjectTag{
	Zombie,   // ゾンビ（部品は Zombie）
	Fuel,     // 落ちているガソリン（部品は Pickup。拾うと消える）
	AmmoDrop, // ゾンビが落とした予備の弾（部品は Pickup。拾うと消える）
};

#endif
