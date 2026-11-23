#ifndef INCLUDED_GAME_OBJECT_TAG_H
#define INCLUDED_GAME_OBJECT_TAG_H

// 物（GameObject）が何の物か。ゲームの判断（数の上限・確認用の表示を物ごとに分ける）はこれで行い、名前（文字列）では行わない
// 同じ部品の組み合わせを、違う物として見分けるのにも使う
// （落ちているガソリンとゾンビが落とした弾は、どちらも部品は StaticModel・BoxCollider・Item・Pickup）
// このゲームに出てくる物の種類の一覧なので、物の種類を足すときはここにも足す
enum class GameObjectTag{
	Player,      // 自キャラ
	Weapon,      // 自キャラが持てる銃（拳銃・ライフル。捨てても消えず、弾倉の中身を持ち続ける。捨てている間は、同じ物に地面に置くための部品が付く）
	Zombie,      // ゾンビ
	Fuel,        // 落ちているガソリン（拾うと消える）
	AmmoDrop,    // 地面に落ちている予備の弾（カバンから捨てた弾。拾うと消える）
	ItemDrop,    // 地面に落ちている包帯（カバンから捨てた物。拾うと消える）
	LootContainer, // 漁れる置物（ゴミ箱・木箱。動かない）
	EscapeCar,   // 今日の脱出の車（ガソリンを入れる。漁れない）
	FireHydrant, // 消火栓（見た目の置物。動かない箱で移動を止める。使えない）
	StreetLamp,  // 街灯の柱（動かない箱。LightComponent が柱の頭から照らす）
	SeveredLimb, // ゾンビから千切れた部位（飛んで地面で止まり、沈み終えたら自分で消える。同時に出しておける数に上限がある）
	BuildingPreview, // F6 の Town タブで足した建物を、出撃を読み直すまで見せる物（建物と同じ形。上を通れる。読み直すと本物の建物になる）
};

#endif
