#ifndef INCLUDED_HOLD_COMPONENT_H
#define INCLUDED_HOLD_COMPONENT_H

#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix34.h"
#include "GameSrc/World/Component.h"
#include "GameSrc/World/GameObjectHandle.h"
#include "GameSrc/Weapon/WeaponStance.h"
#include "GameSrc/Player/Animator.h"
#include "GameSrc/Render/SkinnedMesh/AnimationPoseSampler.h"
#include <memory>
#include <vector>

class SkinnedMeshData;
class GameObject;
class Inventory;
class HeldItem;
class HeldMagazine;
class GunComponent;
class LightComponent;
class WeaponModelComponent;
class SkinnedModelComponent;
class FirstPersonBodyComponent;

// 自キャラが手に持っている物（画像の HoldComponent。今は銃 "AK47" だけ）と、武器の構えの段階（しまう／持つ／覗く。WeaponStance）。
// 持つ物は別の GameObject（銃の部品は GunComponent・WeaponModelComponent・LightComponent・MuzzleFlashComponent）で、ここは借りるだけ（★4）。
// 銃は右手の骨に付いて動く（HeldItem）。弾倉は支える手（左手）で抜いて挿す（HeldMagazine）。
// 両手持ち・走って胸元に抱える・リロードで腕をアニメへ差し替える効き具合を決め、一人称の体（FirstPersonBodyComponent）へ渡す。
// しまった武器は描かない（背中に背負った見た目は、一人称では見えないので持たない）。出し入れのアニメが無いので、切り替えた瞬間に手へ現れる・消える。
// 撃つ・リロードを始めるかを決めるのは PlayerComponent（ここは「持っている銃」と「構え」を答え、頼まれたら銃へ伝える）。
class HoldComponent : public Component{
public:
	// held: 持たせる物（銃）。nullptr なら素手。所有権は受け取らない（持ち主は GameObjectWorld）。この部品より長く生きていること。
	// 持つ側の形（右手・左手の骨を引き、持ち方を合わせたアニメ・リロードのアニメ "rifle_reload" を読む）は、
	// 同じ物の SkinnedModelComponent から awake で借りる。
	explicit HoldComponent( const GameObject* held );
	~HoldComponent();

	// 手に持っている銃（持っていない・手の骨が無くて持てなかったら nullptr）。しまっていても返す。
	GunComponent* gun() const;
	// 手に持っている銃のライト（無ければ nullptr）。
	LightComponent* light() const;
	// 手に持っている物（無ければ nullptr）。銃口の光（MuzzleFlashComponent）など、物の部品を読むのに使う。
	GameObject* heldObject() const;

	// ── 構え（WeaponStance）──
	void updateStance( bool holsterPressed, bool aimHeld, bool runHeld );
	bool isDrawn() const;   // 武器を出しているか（銃を持っていなくても構えの段階は進む）
	bool isAiming() const;  // 覗いているか（覗き始め・終わりの途中も true）
	float aimWeight() const; // 覗き具合（0〜1）
	// 銃を手に持って出しているか（撃てる・ぶれる・移動の当たり判定を銃の先まで伸ばす）。
	bool isWeaponInHand() const;

	// R でリロードを始め・しまったらやめ、時間割を進め、込め終えたら予備（inventory）の弾を弾倉へ移す（構えの段階を決めた後）。
	void updateReload( bool reloadPressed, Inventory* inventory );
	// 腕の効き具合（リロードで差し替える割合・両手持ち・胸元への寄せ・覗き具合）を決めて体へ渡す。next: このティックのアニメの状態。
	void updateArms( Animator::State next );
	// 弾倉を抜く合図のティックなら、弾倉を銃から外して左手に持たせる（このティックのポーズ・置き場所が決まった後に呼ぶ）。
	void takeOutMagazineIfCued();
	float carryWeight() const; // 走って胸元に抱えている割合（0〜1）

	// 銃口の今の位置（ワールド）と向き（長さ1）。銃を持っていない・しまっている・まだポーズが無ければ false（out は変えない）。
	bool muzzle( GameLib::Math::Vector3* position, GameLib::Math::Vector3* direction ) const;
	// 手に持って出している銃と弾倉を描く（持つ側の見た目を描いた後に呼ぶ）。
	void draw() const;

	// Component（SkinnedModelComponent・FirstPersonBodyComponent が要る）
	// awake：持つ側の形を借り、リロードの腕のポーズを求める物を作る。start：銃を手に持たせる（一人称の体へ持ち方を渡す）
	virtual void awake();
	virtual void start();

private:
	// 銃の今の置き場所（銃のモデル座標 → ワールド）。持っていない・まだポーズが無ければ false。
	bool weaponPlacement( GameLib::Math::Matrix34* out ) const;

	const SkinnedMeshData* mHolderData;
	GameObjectHandle mHeld;
	GunComponent* mGun;                        // 手に持てた銃（start で決まる）
	LightComponent* mLight;
	const WeaponModelComponent* mWeaponModel;
	const SkinnedModelComponent* mModel;
	FirstPersonBodyComponent* mBody;
	std::unique_ptr< HeldItem > mHeldItem;     // 銃を右手に付ける（持っていなければ空）
	std::unique_ptr< HeldMagazine > mMagazine; // 弾倉の見た目。銃・弾倉・支える手のどれかが無ければ空
	WeaponStance mStance;
	std::unique_ptr< AnimationPoseSampler > mReloadArms; // リロードのアニメのポーズを求める（腕の差し替えに使う。形を借りるので awake で作る）
	std::vector< GameLib::Math::Matrix34 > mReloadPose; // 求めたポーズ（FirstPersonBody が次の姿勢の計算まで借りる）
	float mHoldWeight;                         // 両手持ちの効き具合（FirstPersonBody::setHoldWeight へ渡す）
	float mCarryWeight;                        // 胸元への寄せ具合（FirstPersonBody::setCarryWeight へ渡す）
};

#endif
