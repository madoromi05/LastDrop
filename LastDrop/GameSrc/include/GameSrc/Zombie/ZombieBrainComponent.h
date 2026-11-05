#ifndef INCLUDED_ZOMBIE_BRAIN_COMPONENT_H
#define INCLUDED_ZOMBIE_BRAIN_COMPONENT_H

#include "GameLib/Math/Vector3.h"
#include "GameSrc/World/Component.h"
#include "GameSrc/Render/SkinnedMesh/BonePoseModifierChain.h"
#include "GameSrc/Zombie/AnimationThrottle.h"

class HitWorld;
class ZombieCrowd;
class GameObject;
class SkinnedMeshInstance;
class HitboxSet;
class CharacterMover;
class ZombieLocomotion;
class ZombieRootMotionSpan;
class ZombieStateComponent;
class ZombieChaseComponent;
class ZombieAttackComponent;
class ZombieHealthComponent;
class ZombieDismemberComponent;
struct ChaseTarget;

// ゾンビ 1 体の部品を、1 ティックの中で呼ぶ順番を決める部品（Think の段で動く。SummaryPlan/個別仕様/ZombieBrainSplit_Plan.txt）。
// 判断はそれぞれの部品が持ち、ここは順番だけを書く（順番を間違えると 1 ティック遅れの揺れが出るので、1 か所で見えるようにする）。
// ゾンビの物の部品（ZombieFactory が付ける。付ける順は自由。SummaryPlan/個別仕様/ComponentSystem3_Plan.txt）：
//   必須：Transform（足元と向き）・ZombieModel（設計図）・SkinnedModel（見た目）・Hitbox（部位の命中）・
//         CharacterCollider（移動の当たり判定と滑る移動）・ZombieDismember（部位欠損・引きずる腕）・ZombieRootMotion（アニメと根の動き）・
//         ZombieState（状態と体の位置）・ZombieChase（立って追う・うろつく・道探し・フラつき）
//   任意（外すとその振る舞いだけ無くなる。使う所ごとに getComponent で引く）：
//         ZombieAttack（攻撃しない）・ZombieHealth（撃っても死なない・のけぞらない）・
//         ZombieCrawl（脚を失ったら這わずに倒れたまま）・ZombieCorpse（死体が消えない）
// 姿勢の書き換え（引きずる腕・のけぞり・失った部位）を掛ける順もここで決める。
// 銃声を聞くかどうか（倒れている・よろけている・攻撃している体は聞かない）は、全部の部品の様子を見て決めるのでここ。
class ZombieBrainComponent : public Component{
public:
	// 全員の Think の段で共有する、このティックの周り（GameWorld が Think の段の前に作り直す）。
	// GameWorld が共有の棚へ置き、この部品が start で引く（GameObjectWorld::provide）。
	struct TickContext{
		const ChaseTarget* target = nullptr; // 追う先。nullptr なら追わない（自キャラが居ない）。attackTaken は各体が決め直す
		const HitWorld* hitWorld = nullptr;  // 見通しを聞く名簿
		const ZombieCrowd* crowd = nullptr;  // 立って歩けるゾンビの位置の並び（近くの仲間を避ける。nullptr なら避けない）
		// 攻撃に行っている体（前のティックから続いている 1 体）。攻撃を始めた体がここへ入り、後の体は攻撃を始めない
		// （同じティックに 2 体が攻撃を始めないように）。
		const GameObject* attacker = nullptr;
		// 前のフレームで絵を描いたカメラ。アニメを間引く段（AnimationThrottle）を決めるのに使う。nullptr なら間引かない。
		const AnimationView* view = nullptr;
	};

	ZombieBrainComponent();

	// position で銃声がした（届くかは持ち主が決める）。立っている（か、体を引きずっている）、よろけ・攻撃の最中でなければ、
	// 少し遅れてからそこへ探しに来る（SummaryPlan/個別仕様/ZombieHearing_Plan.txt）。
	void hearNoise( const GameLib::Math::Vector3& position );

	// Component（必須の部品と、共有の棚の TickContext が要る）
	UpdatePhase updatePhase() const override;
	void start() override;
	// 1ティック進める。立っていてよろけていなければ、追う先を考えて向きを変え・歩き（追いついたら攻撃し）、倒れていれば這い、
	// アニメを進め、倒れた後の時間を進め、部位の当たり判定をそのポーズへ置き直す。頂点の変形は描くときに行う。
	void update() override;

	// 最後のティックのアニメの段（F3 の頭の上の表示に使う）。
	AnimationThrottle::Level animationLevel() const;

private:
	// 立っている体が、追う先を考えて向きを変え、追いついたら攻撃を始め、そうでなければ歩く・横へよける。
	// attack: 攻撃の部品（外されていれば nullptr）。
	void chase( const ChaseTarget* target, const HitWorld& hitWorld, const ZombieCrowd* crowd, ZombieAttackComponent* attack );
	// 描く直前の姿勢の書き換えを組み直す。引きずる腕 → のけぞり → 失った部位を潰す、の順に掛ける
	// （撃たれた腕は引きずる形から押され、のけぞった肩の位置で切り口を作るため）。
	// のけぞりは外せる体力の部品が持つので毎ティック組み直す（外された部品の書き換えを指したまま姿勢を求めないため）。
	// health: 体力の部品（外されていれば nullptr）。
	void rebuildPoseModifiers( ZombieHealthComponent* health );
	// アニメを間引いてはいけない間か（攻撃・根の動き・よろけ・のけぞり・体を引きずる間。仕様書 9 章）。
	// 根の動きを置き場所へ移す処理や、アニメの何フレーム目で何かを起こす処理が、飛んだ姿勢だとずれるため。
	// attack / health: 外せる部品（外されていれば nullptr）。
	bool mustPoseEveryTick( const ZombieAttackComponent* attack, const ZombieHealthComponent* health ) const;

	TickContext* mContext; // 借りているだけ（共有の棚から start で引く）
	BonePoseModifierChain mPoseModifiers; // 掛ける順は mPoseModifiers へ足す順で決まる
	AnimationThrottle mAnimationThrottle; // 姿勢を求めるティックを間引く（体ごとのずれは start で物の番号から決め直す）
	// ── 同じ物の必須の部品（start で借りる）──
	GameLib::Math::Vector3* mPosition; // 足元の位置（TransformComponent）
	SkinnedMeshInstance* mMesh;
	HitboxSet* mHitboxes;
	const CharacterMover* mMover;      // 足元を地面（歩道の段差）の高さへ合わせる
	ZombieDismemberComponent* mDismember;
	ZombieLocomotion* mLocomotion;
	ZombieRootMotionSpan* mRootSpan;
	const ZombieStateComponent* mState;
	ZombieChaseComponent* mChase;
};

#endif
