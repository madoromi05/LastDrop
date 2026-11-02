#ifndef INCLUDED_COMBAT_RESOLVER_H
#define INCLUDED_COMBAT_RESOLVER_H

#include "GameSrc/Weapon/BulletList.h"
#include "GameSrc/Scene/Game/EffectManager.h"
#include "GameSrc/Hit/BodyPart.h"
#include "GameSrc/Audio/GroundMaterial.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Random.h"
#include <vector>

class GameObject;
class GameObjectWorld;
class HitWorld;
class PropFactory;
class ZombieStateComponent;

// 当たった後に何が起きるかを決める係（SummaryPlan/個別仕様/GameWorldSplit_Plan.txt 2-4。2026-11-02 GameWorld から分けた）。
// ・弾：物に当たったら弾痕と砂ぼこり、ゾンビに当たったらダメージを渡して血しぶき・後ろの壁と足元の血の跡・千切れた部位・倒したら弾を落とす
// ・ゾンビの腕（★3）：自キャラに当たったら体力を減らす
// 起きたことは出来事の並び（ZombieHit・SurfaceHit・ZombieAttackEvent）に残し、音・画面の赤み・揺れの係（シーン）が読む。
// 相手の体力へは IDamageable（ZombieHealthComponent::applyDamage）・PlayerHealthComponent::applyZombieHit の入口だけを通す
// （部位の倍率・倒れるかは受ける側が決める）。武器・ダメージ・部位欠損・落とす物の決まりを変えるときは、ここだけを直す。
class CombatResolver{
public:
	// ゾンビに弾が当たった出来事（ヒット音を鳴らすなど、世界の外の係が読む）。死体に当たったときも入る。
	struct ZombieHit{
		GameLib::Math::Vector3 point; // 当たった位置（ワールド座標）
		BodyPart part;                // 当たった部位
	};
	// 弾が物（建物・車・地面・拾える物）に当たった出来事（着弾音を鳴らす係が読む）。
	struct SurfaceHit{
		GameLib::Math::Vector3 point; // 当たった位置（ワールド座標）
		GroundMaterial material;      // 当たった物の、その位置の表面の材質
	};
	// ゾンビの腕が自キャラに当たった出来事（音・画面の赤み・揺れの係が読む）。自キャラの体力はもう減らしてある。
	struct ZombieAttackEvent{
		GameLib::Math::Vector3 point; // ゾンビの頭の位置（ワールド座標。音を鳴らす所）
	};

	// どれも所有権は受け取らない。nullptr 不可。この係より長く生きていること（どれも GameWorld が持つ）。
	// objects: 当たった物の部品を引く・落とした弾を置く。effects: 弾痕・血しぶき・血の跡・千切れた部位を出す。props: 落とす弾を組み立てる。
	// hitWorld: 血の跡を付ける面（撃たれたゾンビの後ろ・足元）を聞く。
	CombatResolver( GameObjectWorld* objects, EffectManager* effects, const PropFactory* props, const HitWorld* hitWorld );
	CombatResolver( const CombatResolver& ) = delete;
	CombatResolver& operator=( const CombatResolver& ) = delete;

	// このティックの出来事を空にする。ティックの頭（弾を進める前）に 1 回呼ぶ。
	void beginTick();
	// 弾が当たった所を当たった順に解決する（BulletList::update の後）。
	// player: 自キャラの物（落とした弾を自キャラ側へ寄せるのに使う）。nullptr 可（寄せない）。
	void resolveBulletImpacts( const std::vector< BulletList::Impact >& impacts, const GameObject* player );
	// このティックに腕が当たったゾンビの分だけ、自キャラの体力を減らして出来事を残す（ゾンビの Think の段の後）。
	// player: nullptr 可。自キャラが居ない・体力の部品を外した自キャラなら、減らさず出来事も残さない。
	void resolveZombieAttacks( GameObject* player );

	// 最後の update でゾンビに当たった出来事（当たった順）。次の update で入れ替わる。
	const std::vector< ZombieHit >& zombieHitsThisTick() const;
	// 最後の update で弾が物に当たった出来事（当たった順）。次の update で入れ替わる。
	const std::vector< SurfaceHit >& surfaceHitsThisTick() const;
	// 最後の update でゾンビの腕が自キャラに当たった出来事（起きた順）。次の update で入れ替わる。
	const std::vector< ZombieAttackEvent >& zombieAttacksThisTick() const;

private:
	// 弾がキャラ（impact.hit.target）に当たったとき、ゾンビならダメージを渡し、血しぶきを出し、出来事を覚える。部位が切れたら飛ばす。
	void hitZombie( const BulletList::Impact& impact, const GameObject* player );
	// 弾が当たったゾンビ（impact.hit.target）の後ろ（弾の向きへ）と真下を聞き、物の面に血の跡を付ける（BloodStain_Plan.txt 3 章）。
	// 後ろは、先に別のゾンビに当たったら付けない（後ろのゾンビにも、その奥の壁にも）。
	void stainAroundZombie( const BulletList::Impact& impact, EffectManager::BloodStainSize size );
	// zombie が今倒れたので、決まった割合で足元（自キャラ側へ少し寄せた所）に予備の弾を落とし、死体から弾をたどれるようにする。
	// 落ちている弾（タグ AmmoDrop）が多すぎたら、一番古い物を消す。数の上限があるのは弾だけ（ガソリンは数が決まっている）。
	void dropAmmo( const ZombieStateComponent& zombie, const GameObject* player );

	GameObjectWorld* mObjects;  // 借りているだけ
	EffectManager* mEffects;    // 借りているだけ
	const PropFactory* mProps;  // 借りているだけ
	const HitWorld* mHitWorld;  // 借りているだけ
	GameLib::Math::Random mRandom; // 弾を落とすか・落とした弾の向きを選ぶ（遊ぶたびに変わるよう、起動からの時間を種にする）
	std::vector< ZombieHit > mZombieHits;           // このティックの出来事（毎回確保し直さないために持つ）
	std::vector< SurfaceHit > mSurfaceHits;         // 同上
	std::vector< ZombieAttackEvent > mZombieAttacks; // 同上
};

#endif
