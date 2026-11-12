#ifndef INCLUDED_COMBAT_RESOLVER_H
#define INCLUDED_COMBAT_RESOLVER_H

#include "GameSrc/Weapon/Bullet/BulletList.h"
#include "GameSrc/Scene/Game/World/EffectManager.h"
#include "GameSrc/Hit/BodyPart.h"
#include "GameSrc/Interaction/Item/ItemCatalog.h"
#include "GameSrc/Audio/GroundMaterial.h"
#include "GameLib/Math/Vector3.h"
#include <vector>

class GameObject;
class GameObjectWorld;
class HitWorld;

// 当たった後に何が起きるかを決める係
// ・弾：物に当たったら弾痕と砂ぼこり、ゾンビに当たったらダメージを渡して血しぶき・後ろの壁と足元の血の跡・千切れた部位・
//   倒した数を数える（死体は漁れない・物も落とさない）
// ・ゾンビの攻撃：つかんだら出来事を残し、噛んだ（這う体は腕が当たった）ら体力を減らす
// 起きたことは出来事の並び（ZombieHit・SurfaceHit・ZombieAttackEvent）に残し、音・画面の赤み・揺れの係（シーン）が読む
// 相手の体力へは IDamageable（ZombieHealthComponent::applyDamage）・PlayerHealthComponent::applyZombieHit の入口だけを通す
// （部位の倍率・倒れるかは受ける側が決める）。武器・ダメージ・部位欠損の決まりを変えるときは、ここだけを直す
class CombatResolver{
public:
	// ゾンビに弾が当たった出来事（ヒット音を鳴らすなど、世界の外の係が読む）。死体に当たったときも入る
	struct ZombieHit{
		// 命中の重さ（HitFeel_Plan.txt）。血・画面の揺れの係が読む。決めるのは CombatResolver.cpp の chooseHitWeight だけ
		enum class Weight{
			Normal,    // 胴・腕・脚に当たっただけ（死体に当たったときも）
			Severed,   // 腕・脚が千切れた（脚を失って倒れた Downed も）
			Killed,    // 止めを刺した（部位は飛んでいない・頭ではない）
			HeadBurst, // 頭への 1 発で倒した（頭が飛ぶ）
		};
		GameLib::Math::Vector3 point; // 当たった位置（ワールド座標）
		BodyPart part;                // 当たった部位
		Weight weight;
		bool killed;                  // この 1 発で倒した（重さとは別。脚を飛ばして倒したときも true。倒れる音に使う）
	};
	// 弾が物（建物・車・地面・拾える物）に当たった出来事（着弾音を鳴らす係が読む）
	struct SurfaceHit{
		GameLib::Math::Vector3 point; // 当たった位置（ワールド座標）
		GroundMaterial material;      // 当たった物の、その位置の表面の材質
	};
	// ゾンビが自キャラをつかんだ・噛んだ出来事（視点を向ける・音・画面の赤み・揺れの係が読む）
	struct ZombieAttackEvent{
		enum class Kind{
			Grab, // つかんだ（体力は減らない）
			Hit,  // 噛んだ・這う体の腕が当たった（自キャラの体力はもう減らしてある）
		};
		Kind kind;
		GameLib::Math::Vector3 point; // ゾンビの頭の位置（ワールド座標。顔を向ける先・音を鳴らす所）
	};

	// どれも所有権は受け取らない。nullptr 不可。この係より長く生きていること（どれも GameWorld が持つ）
	// objects: 当たった物の部品を引く。effects: 弾痕・血しぶき・血の跡・千切れた部位を出す
	// hitWorld: 血の跡を付ける面（撃たれたゾンビの後ろ・足元）を聞く
	CombatResolver( GameObjectWorld* objects, EffectManager* effects, const HitWorld* hitWorld );
	CombatResolver( const CombatResolver& ) = delete;
	CombatResolver& operator=( const CombatResolver& ) = delete;

	void beginTick();																// このティックの出来事を空にする。ティックの頭（弾を進める前）に 1 回呼ぶ
	void resolveBulletImpacts( const std::vector< BulletList::Impact >& impacts );	// 弾が当たった所を当たった順に解決する（BulletList::update の後）
	// このティックにつかんだ・噛んだゾンビの分だけ出来事を残し、噛んだ分は自キャラの体力を減らす（ゾンビの Think の段の後）
	// player: nullptr 可。自キャラが居なければ何も残さない。体力の部品を外した自キャラなら、噛まれた出来事も残さない（つかまれた方は残す）
	void resolveZombieAttacks( GameObject* player );

	const std::vector< ZombieHit >& zombieHitsThisTick() const;				// 最後の update でゾンビに当たった出来事（当たった順）。次の update で入れ替わる
	const std::vector< SurfaceHit >& surfaceHitsThisTick() const;			// 最後の update で弾が物に当たった出来事（当たった順）。次の update で入れ替わる
	const std::vector< ZombieAttackEvent >& zombieAttacksThisTick() const;	// 最後の update でゾンビが自キャラをつかんだ・噛んだ出来事（起きた順）。次の update で入れ替わる
	int zombiesKilledThisTick() const;										// 最後の update で倒したゾンビの数（弾で止めを刺した数。脚を失って倒れただけ〈Downed〉は数えない）。1 日の倒した数・スコアに使う

private:
	void hitZombie( const BulletList::Impact& impact );	// 弾がキャラ（impact.hit.target）に当たったとき、ゾンビならダメージを渡し、血しぶきを出し、出来事を覚える。部位が切れたら飛ばす
	// 弾が当たったゾンビ（impact.hit.target）の後ろ（弾の向きへ）と真下を聞き、物の面に血の跡を付ける
	// 後ろは、先に別のゾンビに当たったら付けない（後ろのゾンビにも、その奥の壁にも）
	void stainAroundZombie( const BulletList::Impact& impact, EffectManager::BloodStainSize size );

	GameObjectWorld* mObjects;  // 借りているだけ
	EffectManager* mEffects;    // 借りているだけ
	const HitWorld* mHitWorld;  // 借りているだけ
	std::vector< ZombieHit > mZombieHits;           // このティックの出来事（毎回確保し直さないために持つ）
	std::vector< SurfaceHit > mSurfaceHits;         // 同上
	std::vector< ZombieAttackEvent > mZombieAttacks; // 同上
	int mZombiesKilled; // このティックに倒した数
};

#endif
