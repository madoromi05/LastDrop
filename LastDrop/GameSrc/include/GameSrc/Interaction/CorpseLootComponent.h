#ifndef INCLUDED_CORPSE_LOOT_COMPONENT_H
#define INCLUDED_CORPSE_LOOT_COMPONENT_H

#include "GameSrc/World/Component.h"
#include "GameSrc/World/GameObjectHandle.h"

// 倒れたゾンビの物に後から付ける部品。そのゾンビが足元に落とした弾を指す（2026-10-31 ユーザー指定：死体を見ても弾を拾える。
// SummaryPlan/個別仕様/GameWorldSplit_Plan.txt の ★1）。
// 付けるのは CombatResolver（弾を落としたとき）、読むのは InteractionSystem（見た死体からたどる）だけ。
// ゾンビの部品はこの部品を知らない（ゾンビは自分が何を落としたかを知らずに済む）。
// 弾が拾われて消えたら drop は nullptr を返す。死体が消えれば、この部品も一緒に消える（並べて掃除する物が要らない）。
class CorpseLootComponent : public Component{
public:
	// drop: 落とした弾の物。nullptr 不可。
	explicit CorpseLootComponent( const GameObject* drop ) : mDrop( drop ){}

	// 落とした弾の物（拾われて消えたら nullptr）。
	GameObject* drop() const { return mDrop.get(); }

private:
	GameObjectHandle mDrop;
};

#endif
