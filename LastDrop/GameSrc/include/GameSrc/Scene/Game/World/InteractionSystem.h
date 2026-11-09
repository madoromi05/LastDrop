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
class LootContainerComponent;
class EscapeCarComponent;

// 見て E で使える物を探す・拾われた物を消す係（SummaryPlan/個別仕様/設計と分割/GameWorldSplit_Plan.txt 2-5。2026-11-02 GameWorld から分けた）。
// 使える物：落ちている物（カバンから捨てた物）、漁れる場所（ゴミ箱・木箱。ゾンビの死体は 2026-11-09 から漁れない。
// どれも LootContainerComponent を持つ。R3_Plan.txt Q1・Q10）。「何を見たら何を使うか」の決まりを足すときは、ここだけを直す。
// 何を使うと何が起きるか（持ち物が増える）は、使える部品（IInteractable）が決める。
class InteractionSystem{
public:
	// 近くの使える物 1 つ（collectNearby の答え）。
	struct NearbyInteractable{
		const IInteractable* interactable;     // 借りているだけ（拾われた物は次の世界の update で消えるので、それより後には使わない）
		GameLib::Math::Vector3 markPosition;   // マークを出す所（物の見た目の箱の上端の少し上。ワールド）
	};

	// どれも所有権は受け取らない。nullptr 不可。この係より長く生きていること（どれも GameWorld が持つ）。
	// hitWorld: 視線を撃つ名簿。objects: 当たった物の部品を引く・消す印を付ける。effects: 消える物に付いた弾痕を外す。
	InteractionSystem( const HitWorld* hitWorld, GameObjectWorld* objects, EffectManager* effects );
	InteractionSystem( const InteractionSystem& ) = delete;
	InteractionSystem& operator=( const InteractionSystem& ) = delete;

	// origin から delta だけ進む視線の先の、一番手前の使える物。視線はキャラにも当たる（キャラに当たったら何も使えない）。
	// 落ちている物は一回り大きい見る箱（PickupComponent::lookRaycast）でも当たる。壁などの向こうの物は返さない。
	// viewer: 視線を当てない的（目が体の中にある自キャラ）。nullptr 可。使える物が無ければ nullptr（Interactor の finder に使う）。
	IInteractable* findInteractable( const GameLib::Math::Vector3& origin, const GameLib::Math::Vector3& delta, const IHittable* viewer );
	// 目 eye から radius（m）以内にマークがあり、目とマークの間が壁・建物・ほかの物で隠れていない使える物を、全部 out の後ろへ足す
	// （マークを出すため。InteractMarker_Plan.txt Q1・Q5）。拾われた物は入れない。死体には出さない。
	// 隠れているかは弾を止める物だけで調べる（ゾンビの体では隠さない）。out: nullptr 不可。
	void collectNearby( const GameLib::Math::Vector3& eye, float radius, std::vector< NearbyInteractable >* out ) const;
	// interactable（Interactor::opened が返した物）を、漁れる場所の部品として引き直す（開けている間、消えたかを GameObjectHandle で見るため）。
	// 漁れる場所でなければ nullptr。
	LootContainerComponent* findContainer( const IInteractable* interactable );
	// interactable（Interactor::opened が返した物）を、脱出の車の部品として引き直す（同じ理由）。脱出の車でなければ nullptr。
	EscapeCarComponent* findEscapeCar( const IInteractable* interactable );
	// ── F6 の確認用（Loot の欄。R3_Plan.txt Q13）──
	// 全部の漁れる場所の中身を決め直し、まだ開けていない形に戻す。seed: 乱数の種。
	void rerollLoot( int seed );
	// 全部の漁れる場所を開けたことにする（待たずに中を見られる）。
	void markAllLootSearched();
	// 拾われた物に、消す印を付ける（実際に消えるのは次の update の最後）。物を使った後、1回の描画につき1回呼ぶ。
	void removeTakenPickups();
	// 落ちている物（まだ拾われていないガソリン）の移動の当たり判定の箱を、全部 out の後ろへ足す。
	// 確認用の表示が、物の場所を壁越しに見せるのに使う。out: 出力先。nullptr 不可。
	void appendPickupColliders( std::vector< BoxCollider >* out ) const;

private:
	// 視線が当たった物（findInteractable が一番手前を選ぶ前の 1 つ）が使える物ならそれを返す。キャラなら nullptr。
	IInteractable* resolveInteractable( const HitWorld::Hit& hit ) const;

	const HitWorld* mHitWorld;  // 借りているだけ
	GameObjectWorld* mObjects;  // 借りているだけ
	EffectManager* mEffects;    // 借りているだけ
};

#endif
