#ifndef INCLUDED_HOLD_COMPONENT_H
#define INCLUDED_HOLD_COMPONENT_H

#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix34.h"
#include "GameSrc/World/Component.h"
#include "GameSrc/World/GameObjectHandle.h"
#include "GameSrc/Weapon/WeaponStance.h"
#include "GameSrc/Interaction/Item/ItemCatalog.h"
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

// 自キャラが手に持っている物（画像の HoldComponent。銃）と、武器の構えの段階（しまう／持つ／覗く。WeaponStance）。
// 持てる銃（拳銃・ライフル）は作る時に全部受け取り、カバンに入っている銃のうち 1 丁を手に持つ（RE:2 と同じく、持っている銃もカバンの 1 マスを使う）。
// どれを持つかはカバンの画面の Equip（equip）。持っている銃をカバンから捨てたら素手になり、素手のときカバンに銃があればそれを持つ（syncWithInventory）。
// 銃の物（GunComponent）は捨てても消えないので、弾倉の中身は銃ごとに残る（拾い直すと前の弾数のまま）。SummaryPlan/個別仕様/射撃と手応え/Pistol_Plan.txt。
// 持つ物は別の GameObject（銃の部品は GunComponent・WeaponModelComponent・LightComponent・MuzzleFlashComponent）で、ここは借りるだけ（★4）。
// 銃は右手の骨に付いて動く（HeldItem）。弾倉は支える手（左手）で抜いて挿す（HeldMagazine）。
// 持ち方（手の骨・持ち方・サイト・支える手・銃の大きさ）を一人称の体（FirstPersonBodyComponent）へ渡す。
// 腕のポーズ（両手持ち・胸元への寄せ・リロードで腕をアニメへ差し替える効き具合）は ArmPoseComponent
// （2026-11-09 にここから分けた。HighPrioritySplit_Plan.txt 4 章。あちらがこの部品を読み、この部品はあちらを知らない）。
// しまった武器は描かない（背中に背負った見た目は、一人称では見えないので持たない）。出し入れのアニメが無いので、切り替えた瞬間に手へ現れる・消える。
// 撃つ・リロードを始めるかを決めるのは PlayerComponent（ここは「持っている銃」と「構え」を答え、頼まれたら銃へ伝える）。
class HoldComponent : public Component{
public:
	// weapons: 持てる銃（GunComponent・WeaponModelComponent の付いた物。1 種類 1 丁）。nullptr は飛ばす。所有権は受け取らない
	// （持ち主は GameObjectWorld）。この部品より長く生きていること。始めは素手で、最初の syncWithInventory でカバンの銃を持つ。
	// 持つ側の形（右手・左手の骨を引き、持ち方を合わせたアニメ〈銃ごとの表 WeaponSpec の名前〉を読む）は、
	// 同じ物の SkinnedModelComponent から awake で借りる。
	explicit HoldComponent( const std::vector< const GameObject* >& weapons );
	~HoldComponent();

	// 手に持っている銃（持っていない・手の骨が無くて持てなかったら nullptr）。しまっていても返す。
	GunComponent* gun() const;
	// holder（自キャラなど）が手に持っている銃。holder が nullptr・この部品が付いていない（外せる部品）・素手なら nullptr。
	// 物の外から銃を読む所（GameScene・GameWorld・CombatResolver・PlayerBodySounds）が、同じ 3 行を書かずに済むようにする。
	static GunComponent* heldGun( const GameObject* holder );
	// 手に持っている銃のライト（無ければ nullptr）。
	LightComponent* light() const;
	// 手に持っている物（無ければ nullptr）。銃口の光（MuzzleFlashComponent）など、物の部品を読むのに使う。
	GameObject* heldObject() const;

	// ── 持ち替え ──
	// カバンの銃 item を手に持つ（持っていた銃のリロードはやめる）。item の銃が無い・もう持っていたら何もしない。持てたら true。
	bool equip( ItemId item );
	// カバンと持っている銃を合わせる（毎ティック、構えより前に呼ぶ）。持っている銃がカバンに無くなった（捨てた）ら素手にし、
	// 素手でカバンに銃があれば、表の順（WeaponCatalog）で最初の銃を持つ。
	void syncWithInventory( const Inventory& inventory );

	// ── 構え（WeaponStance）──
	void updateStance( bool holsterPressed, bool aimHeld, bool runHeld );
	bool isDrawn() const;   // 武器を出しているか（銃を持っていなくても構えの段階は進む）
	bool isAiming() const;  // 覗いているか（覗き始め・終わりの途中も true）
	float aimWeight() const; // 覗き具合（0〜1）
	// 銃を手に持って出しているか（撃てる・ぶれる・移動の当たり判定を銃の先まで伸ばす）。
	bool isWeaponInHand() const;

	// R でリロードを始め・しまったらやめ、時間割を進め、込め終えたら予備（inventory）の弾を弾倉へ移す（構えの段階を決めた後）。
	void updateReload( bool reloadPressed, Inventory* inventory );
	// 弾倉を抜く合図のティックなら、弾倉を銃から外して左手に持たせる（このティックのポーズ・置き場所が決まった後に呼ぶ）。
	void takeOutMagazineIfCued();
	// 銃を持たせる手の骨（start で決まる。見つからなければ -1）。腕のポーズ（ArmPoseComponent）が右手を基準にするのに読む。
	int handBone() const;

	// 銃口の今の位置（ワールド）と向き（長さ1）。銃を持っていない・しまっている・まだポーズが無ければ false（out は変えない）。
	bool muzzle( GameLib::Math::Vector3* position, GameLib::Math::Vector3* direction ) const;
	// 手に持って出している銃と弾倉を描く（持つ側の見た目を描いた後に呼ぶ）。
	void draw() const;

	// Component（SkinnedModelComponent・FirstPersonBodyComponent が要る）
	// awake：持つ側の形を借りる。start：手の骨を引く（銃を持たせるのは最初の syncWithInventory）
	void awake() override;
	void start() override;

private:
	// held を手に持たせる（nullptr なら素手）。銃の部品を借り直し、一人称の体へ持ち方を渡す。
	void bindWeapon( GameObject* held );
	// 持てる銃のうち、カバンの物 item の銃（無ければ nullptr）。
	GameObject* findWeapon( ItemId item ) const;
	// 銃の今の置き場所（銃のモデル座標 → ワールド）。持っていない・まだポーズが無ければ false。
	bool weaponPlacement( GameLib::Math::Matrix34* out ) const;

	const SkinnedMeshData* mHolderData;
	std::vector< GameObjectHandle > mWeapons; // 持てる銃（借りているだけ）
	GameObjectHandle mHeld;                   // 今手に持っている銃（素手なら何も指さない）
	int mHandBone;                            // 銃を持たせる手の骨（start で決まる。見つからなければ -1）
	int mSupportHandBone;                     // 銃を支える手の骨（見つからなければ -1）
	GunComponent* mGun;                        // 手に持てた銃（start で決まる）
	LightComponent* mLight;
	const WeaponModelComponent* mWeaponModel;
	const SkinnedModelComponent* mModel;
	FirstPersonBodyComponent* mBody;
	std::unique_ptr< HeldItem > mHeldItem;     // 銃を右手に付ける（持っていなければ空）
	std::unique_ptr< HeldMagazine > mMagazine; // 弾倉の見た目。銃・弾倉・支える手のどれかが無ければ空
	WeaponStance mStance;
};

#endif
