#ifndef INCLUDED_PLAYER_COMPONENT_H
#define INCLUDED_PLAYER_COMPONENT_H

#include "GameLib/Math/Vector3.h"
#include "GameSrc/World/Component.h"
#include "GameSrc/Collision/ICollisionBody.h"
#include "GameSrc/Collision/BodyFootprint.h"
#include "GameSrc/Player/PlayerCommand.h"
#include "GameSrc/Player/AimSway.h"
#include <memory>

class Inventory;
class TransformComponent;
class SkinnedModelComponent;
class FirstPersonBodyComponent;
class PlayerLocomotionComponent;
class CharacterColliderComponent;

// 一人称(FPS)で操作する自キャラの「操作を読んで隣の部品へ頼む」部品（画像の PlayerComponent）。Input の段で動く。
// 体は常にカメラの水平の向きを向き、WASD はその向き基準の前後・横歩きになる。入力デバイスは読まない（操作は PlayerCommand で受け取る）。
// 自キャラの物の部品（GameWorld::placePlayer が付ける。付ける順は自由。SummaryPlan/個別仕様/ComponentSystem3_Plan.txt）：
//   必須：Transform（足元と向き）・SkinnedModel（見た目）・FirstPersonBody（一人称の体の見せ方）・
//         PlayerLocomotion（歩き方のアニメと速さ）・CharacterCollider（移動の当たり判定と滑る移動）
//   任意（外すとその振る舞いだけ無くなる。使う所ごとに getComponent で引く）：
//         Hold（持つ銃と構え。外すと素手）・PlayerHealth（体力。外すと死なない）・Hitbox（部位の命中。外すと撃たれない）
// どの部品も自分では動かず、ここが 1 ティックの中で決まった順に頼む（順番に意味があるため。下の update の中のコメント）。
// 撃つ操作からは「撃てる状態で引き金を引いているか」を決めて銃へ伝える（fire）。連射の間隔・弾の数は銃（GunComponent）が決める。
// 弾を出す・銃声を鳴らすのは、それを読む側（GameWorld・PlayerBodySounds）。
// 移動の当たり判定は体を真上から囲む長方形（BodyFootprint）で、体の向きに合わせて回る。その形を答えるのもここ（ICollisionBody。
// 名簿に載るのは CharacterColliderComponent の方）。前の辺は今の目の位置の少し先まで伸ばし（目が壁を越えて建物の中が見えないように）、
// 銃を持っている間は、手に持った銃を囲む箱も含める（銃口が壁に入らないように）。
class PlayerComponent : public Component, public ICollisionBody{
public:
	PlayerComponent();

	// 次の update で使う操作。GameWorld が Input の段の前に毎ティック渡す。
	// command: この1ティックの操作。cameraYaw / cameraPitch: カメラの水平・上下の向き（度。FirstPersonCamera::yaw / pitch）。
	// inventory: 予備の弾（ITEM_AMMO）を持つ持ち物。リロードを込め終えたら、ここから減らす。所有権は受け取らない。nullptr 不可。
	void setInput( const PlayerCommand& command, float cameraYaw, float cameraPitch, Inventory* inventory );
	// ぶれを決めるのに要る今の状態（持っているか・覗き具合・歩きの速さと位相・抱えている割合・体力が Danger か）。update の後に呼ぶ。
	AimSwayBody aimSwayBody() const;

	// Component（必須の部品が要る。awake で見た目の形から移動の当たり判定の大きさを求める）
	virtual UpdatePhase updatePhase() const;
	virtual void awake();
	virtual void start();
	virtual void update();
	// ICollisionBody（移動の当たり判定の形）
	virtual BoxCollider collider() const;

private:
	// 今できない操作を外した操作（死んだら何もしない）。
	PlayerCommand allowedCommand( const PlayerCommand& command ) const;
	// 撃てる状態で引き金を引いているかを、手に持った銃へ伝える（画像の PlayerComponent::Fire。持つ部品はここで引く）。
	void fire( bool canFire, bool triggerHeld );

	std::unique_ptr< BodyFootprint > mFootprint; // 移動の当たり判定の大きさ（見た目の形の骨から awake で求める）
	PlayerCommand mCommand;
	float mCameraYaw;
	float mCameraPitch;
	Inventory* mInventory;
	// ── 同じ物の必須の部品（start で借りる）──
	TransformComponent* mTransform;
	SkinnedModelComponent* mModel;
	FirstPersonBodyComponent* mBody;
	PlayerLocomotionComponent* mLocomotion;
	const CharacterColliderComponent* mCollider;
};

#endif
