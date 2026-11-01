#ifndef INCLUDED_INTERACTION_SYSTEM_H
#define INCLUDED_INTERACTION_SYSTEM_H

#include "GameSrc/Hit/HitWorld.h"
#include "GameSrc/Collision/BoxCollider.h"
#include "GameLib/Math/Vector3.h"
#include <vector>

class GameObjectWorld;
class EffectManager;
class IInteractable;
class IHittable;

// 見て E で使える物を探す・拾われた物を消す係（SummaryPlan/個別仕様/GameWorldSplit_Plan.txt 2-5。2026-11-02 GameWorld から分けた）。
// 使える物：落ちている物（ガソリン・ゾンビが落とした弾）・何度でも拾える置き場・脱出用の車、それと死体（その死体が落とした弾。
// 死体の物に付いた CorpseLootComponent からたどる）。「何を見たら何を使うか」の決まりを足すときは、ここだけを直す。
// 何を使うと何が起きるか（持ち物が増える・給油する）は、使える部品（IInteractable）が決める。
class InteractionSystem{
public:
	// どれも所有権は受け取らない。nullptr 不可。この係より長く生きていること（どれも GameWorld が持つ）。
	// hitWorld: 視線を撃つ名簿。objects: 当たった物の部品を引く・消す印を付ける。effects: 消える物に付いた弾痕を外す。
	InteractionSystem( const HitWorld* hitWorld, GameObjectWorld* objects, EffectManager* effects );
	InteractionSystem( const InteractionSystem& ) = delete;
	InteractionSystem& operator=( const InteractionSystem& ) = delete;

	// origin から delta だけ進む視線の先の、一番手前の使える物。視線はキャラにも当たる（死体なら、その死体が落としてまだ残っている弾）。
	// 落ちている物は一回り大きい見る箱（PickupComponent::lookRaycast）でも当たる。壁などの向こうの物は返さない。
	// viewer: 視線を当てない的（目が体の中にある自キャラ）。nullptr 可。使える物が無ければ nullptr（Interactor の finder に使う）。
	IInteractable* findInteractable( const GameLib::Math::Vector3& origin, const GameLib::Math::Vector3& delta, const IHittable* viewer );
	// 拾われた物に、消す印を付ける（実際に消えるのは次の update の最後）。物を使った後、1回の描画につき1回呼ぶ。
	void removeTakenPickups();
	// 落ちている物（まだ拾われていないガソリン）の移動の当たり判定の箱を、全部 out の後ろへ足す。
	// 確認用の表示が、物の場所を壁越しに見せるのに使う。out: 出力先。nullptr 不可。
	void appendPickupColliders( std::vector< BoxCollider >* out ) const;

private:
	// 視線が当たった物（findInteractable が一番手前を選ぶ前の 1 つ）が使える物ならそれを返す。死体なら、その死体が落とした弾。
	IInteractable* resolveInteractable( const HitWorld::Hit& hit ) const;

	const HitWorld* mHitWorld;  // 借りているだけ
	GameObjectWorld* mObjects;  // 借りているだけ
	EffectManager* mEffects;    // 借りているだけ
};

#endif
