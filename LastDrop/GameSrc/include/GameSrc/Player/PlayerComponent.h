#ifndef INCLUDED_PLAYER_COMPONENT_H
#define INCLUDED_PLAYER_COMPONENT_H

#include "GameSrc/World/Component.h"
#include "GameSrc/Player/PlayerCommand.h"
#include "GameSrc/Player/Animator.h"
#include "GameLib/Math/Vector3.h"

class InventoryComponent;
class HoldComponent;
class GunComponent;
class PlayerHealthComponent;
class TransformComponent;
class SkinnedModelComponent;
class FirstPersonBodyComponent;
class PlayerLocomotionComponent;
class CharacterColliderComponent;

// 一人称(FPS)で操作する自キャラの「操作を読んで隣の部品へ頼む」部品（画像の PlayerComponent）。Input の段で動く
// 体は常にカメラの水平の向きを向き、WASD はその向き基準の前後・横歩きになる。入力デバイスは読まない（操作は PlayerCommand で受け取る）
// 自キャラの物の部品（PlayerFactory が付ける。付ける順は自由）：
//   必須：Transform（足元と向き）・SkinnedModel（見た目）・FirstPersonBody（一人称の体の見せ方）・
//         PlayerLocomotion（歩き方のアニメと速さ）・CharacterCollider（移動の当たり判定と滑る移動）・Inventory（持ち物）
//   任意（外すとその振る舞いだけ無くなる。使う所ごとに getComponent で引く）：
//         Hold（持つ銃と構え。外すと素手）・PlayerHealth（体力。外すと死なない）・Hitbox（部位の命中。外すと撃たれない）・
//         PlayerGrab（ゾンビにつかまれる。外すと襲われない）・ArmPose（腕の効き具合。外すと腕は体のアニメのまま）・
//         PlayerFlight（確認用に飛ぶ。Debug・Profile だけ付ける）
// どの部品も自分では動かず、ここが 1 ティックの中で決まった順に頼む（順番に意味があるため。update は段ごとの関数を順に呼ぶ）
// 撃つ操作からは「撃てる状態で引き金を引いているか」を決めて銃へ伝える（fire）。連射の間隔・弾の数は銃（GunComponent）が決める
// 弾を出す・銃声を鳴らすのは、それを読む側（GameWorld・PlayerBodySounds）
// 別の部品に任せている物：
//   移動の当たり判定の形 → PlayerFootprintComponent・照準のぶれの元を集める → PlayerAimSwaySource・
//   ゾンビにつかまれている状態 → PlayerGrabComponent（ここは動けないかを読むだけ）
class PlayerComponent : public Component{
public:
	PlayerComponent();

	// 次の update で使う操作。GameWorld が Input の段の前に毎ティック渡す
	// command: この1ティックの操作。cameraYaw / cameraPitch: カメラの水平・上下の向き（度。FirstPersonCamera::yaw / pitch）
	void setInput( const PlayerCommand& command, float cameraYaw, float cameraPitch );

	// Component（必須の部品が要る）
	UpdatePhase updatePhase() const override;
	void start() override;
	void update() override;

private:
	PlayerCommand allowedCommand( const PlayerCommand& command ) const;						// 今できない操作を外した操作（死んだら何もしない・つかまれていたら動かない）
	void faceCamera();																		// 体の向きと上下を、カメラの向きへ即座に合わせる
	void updateHands( HoldComponent* hold, const PlayerCommand& command, bool wantsRun );	// 持つ物（銃の持ち替え・構え・リロード・懐中電灯）を進める。hold: 持つ部品（外されていれば nullptr。何もしない）
	// 撃てる状態で引き金を引いているかを、手に持った銃へ伝え、反動を体へ渡す（画像の PlayerComponent::Fire）
	// gun: 手に持った銃（素手なら nullptr。反動 0 を渡すだけ）
	void fire( GunComponent* gun, bool canFire, bool triggerHeld );
	// next の歩き方で体を進める：腕 → 速さ → アニメ → 滑る移動 → 地面 → 部位の箱。health: 体力の部品（外されていれば nullptr）
	// flightStep: 飛んでいる間のこのティックに進む量（壁と地面を無視してそのまま進む）。飛んでいなければ nullptr
	void moveBody( Animator::State next, const PlayerHealthComponent* health, const GameLib::Math::Vector3* flightStep );

	PlayerCommand mCommand;
	float mCameraYaw;
	float mCameraPitch;
	// ── 同じ物の必須の部品（start で借りる）──
	InventoryComponent* mInventory;	// 持ち物。カバンの銃を持つ・リロードで予備の弾を弾倉へ移すのに、持つ部品へ渡す
	TransformComponent* mTransform;
	SkinnedModelComponent* mModel;
	FirstPersonBodyComponent* mBody;
	PlayerLocomotionComponent* mLocomotion;
	const CharacterColliderComponent* mCollider;
};

#endif
