#ifndef INCLUDED_PLAYER_COMPONENT_H
#define INCLUDED_PLAYER_COMPONENT_H

#include "GameLib/Math/Vector3.h"
#include "GameSrc/World/Component.h"
#include "GameSrc/Collision/ICollisionBody.h"
#include "GameSrc/Collision/BodyFootprint.h"
#include "GameSrc/Player/PlayerCommand.h"
#include "GameSrc/Player/AimSway.h"

class SkinnedMeshData;
class Inventory;
class TransformComponent;
class SkinnedModelComponent;
class FirstPersonBodyComponent;
class PlayerLocomotionComponent;
class HoldComponent;
class PlayerHealthComponent;
class HitboxComponent;
class CharacterColliderComponent;

// 一人称(FPS)で操作する自キャラの「操作を読んで隣の部品へ頼む」部品（画像の PlayerComponent）。Input の段で動く。
// 体は常にカメラの水平の向きを向き、WASD はその向き基準の前後・横歩きになる。入力デバイスは読まない（操作は PlayerCommand で受け取る）。
// 自キャラの物の部品：
//   Transform（足元と向き）・SkinnedModel（見た目）・FirstPersonBody（一人称の体の見せ方）・Hold（持つ銃と構え）・
//   PlayerLocomotion（歩き方のアニメと速さ）・PlayerHealth（体力）・Hitbox（部位の命中）・CharacterCollider（移動の当たり判定と滑る移動）
// どの部品も自分では動かず、ここが 1 ティックの中で決まった順に頼む（順番に意味があるため。下の update の中のコメント）。
// 撃つ操作からは「撃てる状態で引き金を引いているか」を決めて銃へ伝える（fire）。連射の間隔・弾の数は銃（GunComponent）が決める。
// 弾を出す・銃声を鳴らすのは、それを読む側（GameWorld・PlayerBodySounds）。
// 移動の当たり判定は体を真上から囲む長方形（BodyFootprint）で、体の向きに合わせて回る。その形を答えるのもここ（ICollisionBody。
// 名簿に載るのは CharacterColliderComponent の方）。前の辺は今の目の位置の少し先まで伸ばし（目が壁を越えて建物の中が見えないように）、
// 銃を持っている間は、手に持った銃を囲む箱も含める（銃口が壁に入らないように）。
class PlayerComponent : public Component, public ICollisionBody{
public:
	// data: 形（移動の当たり判定の大きさを骨から求める）。所有権は受け取らない。nullptr 不可。
	explicit PlayerComponent( const SkinnedMeshData* data );

	// 次の update で使う操作。GameWorld が Input の段の前に毎ティック渡す。
	// command: この1ティックの操作。cameraYaw / cameraPitch: カメラの水平・上下の向き（度。FirstPersonCamera::yaw / pitch）。
	// inventory: 予備の弾（ITEM_AMMO）を持つ持ち物。リロードを込め終えたら、ここから減らす。所有権は受け取らない。nullptr 不可。
	void setInput( const PlayerCommand& command, float cameraYaw, float cameraPitch, Inventory* inventory );
	// ぶれを決めるのに要る今の状態（持っているか・覗き具合・歩きの速さと位相・抱えている割合・体力が Danger か）。update の後に呼ぶ。
	AimSwayBody aimSwayBody() const;

	// Component（自キャラの物の部品が全部要る）
	virtual UpdatePhase updatePhase() const;
	virtual void start();
	virtual void update();
	// ICollisionBody（移動の当たり判定の形）
	virtual BoxCollider collider() const;

private:
	// 今できない操作を外した操作（死んだら何もしない）。
	PlayerCommand allowedCommand( const PlayerCommand& command ) const;
	// 撃てる状態で引き金を引いているかを、手に持った銃へ伝える（画像の PlayerComponent::Fire）。
	void fire( bool canFire, bool triggerHeld );

	BodyFootprint mFootprint; // 移動の当たり判定の大きさ
	PlayerCommand mCommand;
	float mCameraYaw;
	float mCameraPitch;
	Inventory* mInventory;
	TransformComponent* mTransform;
	SkinnedModelComponent* mModel;
	FirstPersonBodyComponent* mBody;
	PlayerLocomotionComponent* mLocomotion;
	HoldComponent* mHold;
	const PlayerHealthComponent* mHealth;
	HitboxComponent* mHitbox;
	const CharacterColliderComponent* mCollider;
};

#endif
