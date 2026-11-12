#ifndef INCLUDED_PLAYER_FOOTPRINT_COMPONENT_H
#define INCLUDED_PLAYER_FOOTPRINT_COMPONENT_H

#include "GameSrc/World/Component.h"
#include "GameSrc/Collision/ICollisionBody.h"
#include "GameSrc/Collision/BodyFootprint.h"
#include <memory>

class TransformComponent;
class FirstPersonBodyComponent;

// 自キャラの移動の当たり判定の形を答える部品（SummaryPlan/個別仕様/設計と分割/TickOrderSplit_Plan.txt）
// 形は体を真上から囲む長方形（BodyFootprint）で、体の向きに合わせて回る。前の辺は今の目の位置の少し先まで伸ばし
// （目が壁を越えて建物の中が見えないように）、銃を持っている間は、手に持った銃を囲む箱も含める（銃口が壁に入らないように）
// 持ち主：自キャラの物（PlayerFactory が付ける）。読むのは CharacterColliderComponent（start で setShape して形を覚えさせる
// 名簿に載るのは CharacterCollider の方）
// 自分では動かない（None）。形を作るだけで、体を動かすのは PlayerComponent
// 必須：Transform（足元と向き）・SkinnedModel（骨から大きさを求める）・FirstPersonBody（目と銃の位置）・CharacterCollider（形を渡す先）
// 任意：Hold（外すと素手。銃の箱を含めない）
class PlayerFootprintComponent : public Component, public ICollisionBody{
public:
	PlayerFootprintComponent();

	// Component（awake で見た目の形から大きさを求め、start で CharacterCollider へ自分を形として渡す）
	void awake() override;
	void start() override;
	BoxCollider collider() const override;	// ICollisionBody（移動の当たり判定の形）

private:
	std::unique_ptr< BodyFootprint > mFootprint; // 移動の当たり判定の大きさ（見た目の形の骨から awake で求める）
	// ── 同じ物の必須の部品（start で借りる）──
	const TransformComponent* mTransform;
	const FirstPersonBodyComponent* mBody;
};

#endif
