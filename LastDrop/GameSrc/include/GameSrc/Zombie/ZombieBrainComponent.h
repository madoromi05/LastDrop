#ifndef INCLUDED_ZOMBIE_BRAIN_COMPONENT_H
#define INCLUDED_ZOMBIE_BRAIN_COMPONENT_H

#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix34.h"
#include "GameLib/Math/Random.h"
#include "GameSrc/World/Component.h"
#include "GameSrc/Collision/ICollisionBody.h"
#include "GameSrc/Hit/IDamageable.h"
#include "GameSrc/Hit/BodyPart.h"
#include "GameSrc/Render/SkinnedMesh/BonePoseModifierChain.h"
#include "GameSrc/Zombie/ZombieLocomotion.h"
#include "GameSrc/Zombie/ZombieAttackReach.h"
#include "GameSrc/Zombie/ZombieCrawler.h"
#include <vector>

class ZombieModel;
class HitWorld;
class ZombieCrowd;
class GameObject;
class SkinnedMeshInstance;
class HitboxSet;
class CharacterMover;
class CharacterColliderComponent;
class ZombieDismemberment;
class ZombieFlinch;
class ZombieSway;
class ZombieHealth;
class ZombieStagger;
class ZombieAttack;
class ZombieCorpse;
class ZombieRootMotionSpan;
class ZombieAnimationSet;
struct ChaseTarget;

// ゾンビ 1 体の状態（立っている・倒れている・死体）を持ち、部品どうしをつなぐ部品（前の Zombie クラスの振る舞いの側。★7）。Think の段で動く。
// ゾンビの物の部品：
//   Transform（足元と向き）・SkinnedModel（見た目）・Hitbox（部位の命中）・CharacterCollider（移動の当たり判定と滑る移動）・
//   ZombieHealth（体力・のけぞり・よろけ）・ZombieDismember（部位欠損・体を引きずる腕）・ZombieAttack（攻撃の段階）・
//   ZombieCorpse（死体の時間）・ZombieChase（追う・うろつく・道探し・フラつき）・ZombieRootMotion（アニメと根の動き）
// 立っている間は自キャラに気付くと歩いて追ってくる（どこへ行き・どちらを向いて・どう進むかは ZombieChaseComponent）。
// ここは「考えた後、歩く前」に攻撃を始めるかを見て、歩く・立つのアニメを切り替える（SummaryPlan/個別仕様/ZombieSplit2_Plan.txt）。
// 撃たれると体力が減り、倒れる条件を満たすと死亡アニメで倒れ、しばらく死体として残ってから地面へ沈んで消える（IDamageable）。
// 頭に当たると、確率で頭が弾けて倒れる（クリティカル。確率は ZombieHealth の表、乱数を引くのはここ）。
// 生きている間に撃たれると、1 発ごとに当たった部位がのけぞり、ときどきよろける（よろけのアニメを1回再生して歩きへ戻る）。
// 頭・腕・脚の体力が 0 になると、その部位を切る（見た目から消し、部位の箱を外す）。千切れた部位を飛ばすのは GameWorld。
// 脚を失うと前へ倒れるアニメで倒れ、倒れたまま生きている（Downed）。倒れ終わったら、腕で体を引きずって追ってくる。
// 追いついたら、腕で引っかく攻撃をする。相手に何が起きるかは知らない（「このティックに腕が当たったか」を答えるだけ）。
// 倒れ始めたら移動の名簿から外れる（死体の上を歩ける）。消えるのは持ち主（GameWorld が isGone を見て destroyLater）。
// 移動の当たり判定の形（体を引きずっている間は胸に置く）を答えるのもここ（ICollisionBody。名簿に載るのは CharacterColliderComponent）。
class ZombieBrainComponent : public Component, public ICollisionBody, public IDamageable{
public:
	// 生きているか・倒れているか。並びは時間の順。
	enum class State{
		Alive,   // 立っている（撃たれると体力が減る）
		Downed,  // 脚を失って倒れ、倒れたまま生きている（撃たれると体力が減る。よろけない）
		Falling, // 死亡アニメの途中
		Corpse,  // 倒れ終わって、死体として残っている
		Sinking, // 地面へ沈んでいる（沈み終わったら消えてよい）
	};

	// 全員の Think の段で共有する、このティックの周り（GameWorld が Think の段の前に作り直す）。
	struct TickContext{
		const ChaseTarget* target = nullptr; // 追う先。nullptr なら追わない（自キャラが居ない）。attackTaken は各体が決め直す
		const HitWorld* hitWorld = nullptr;  // 見通しを聞く名簿
		const ZombieCrowd* crowd = nullptr;  // 立って歩けるゾンビの位置の並び（近くの仲間を避ける。nullptr なら避けない）
		// 攻撃に行っている体（前のティックから続いている 1 体）。攻撃を始めた体がここへ入り、後の体は攻撃を始めない
		// （同じティックに 2 体が攻撃を始めないように）。
		const GameObject* attacker = nullptr;
	};

	// model: 形・アニメと当たり判定の設計図。context: このティックの周り。どちらも所有権は受け取らない。nullptr 不可。
	//        この部品より長く生きていること（持ち主の GameWorld が、ゾンビより後に破棄する）。
	// random: 体ごとの乱数（頭のクリティカルを引く）。
	ZombieBrainComponent( const ZombieModel* model, TickContext* context, GameLib::Math::Random random );

	// position で銃声がした（届くかは持ち主が決める）。立っている（か、体を引きずっている）、よろけ・攻撃の最中でなければ、
	// 少し遅れてからそこへ探しに来る（SummaryPlan/個別仕様/ZombieHearing_Plan.txt）。
	void hearNoise( const GameLib::Math::Vector3& position );
	// 最後の applyDamage で切れた部位（無ければ空。次の applyDamage で入れ替わる）。持ち主が千切れた部位を飛ばすのに読む。
	const std::vector< BodyPart >& severedByLastDamage() const;
	// 今見えている骨の姿勢（ワールド。のけぞり・部位欠損の書き換え後）を、骨の番号の順に out へ書く（前の中身は捨てる）。
	// applyDamage の直後なら、切れた部位はまだ見えている（消えるのは次の update）。out: nullptr 不可。
	void shownWorldPose( std::vector< GameLib::Math::Matrix34 >* out ) const;

	State state() const;
	// 足元の位置（ワールド）。攻撃・よろけ・倒れる間は腰に付いて動く（倒れた後は死体の腰の下）。
	const GameLib::Math::Vector3& position() const;
	bool isAlive() const;    // 生きているか（Alive か Downed）
	// 脚を失って倒れ終わり、腕で体を引きずっているか（Downed の間だけ true になりうる）。
	bool isCrawling() const;
	// 体の中心（ワールド）。立っている間は足元、体を引きずっている間は胸。追う・回り込む・壁に当たるのはこの位置。
	// 攻撃・よろけ・倒れるアニメの間は、始めた所（根の動きのアンカー。腕が届く距離もここから測る）。
	GameLib::Math::Vector3 bodyCenter() const;
	// 腕を振っている間か。持ち主は、この間ほかのゾンビに攻撃を始めさせない（TickContext::attacker）。
	bool isAttacking() const;
	bool hitTargetThisTick() const; // 直前の update で相手に腕が当たったか（ダメージの量は ZombieAttack::hitDamage）
	// 頭の位置（ワールド。頭の部位の箱の中心）。攻撃の音を鳴らす所。頭を失っていれば目の高さ。
	GameLib::Math::Vector3 headPosition() const;
	// 沈み終わって、消してよいか（持ち主が付いている物に GameObjectWorld::destroyLater する）。
	bool isGone() const;
	// 消えるまでの残り秒（死体・沈んでいる間だけ。それ以外は 0）。確認用の表示に使う。
	float secondsUntilGone() const;

	// Component（ゾンビの物の部品が全部要る）
	virtual UpdatePhase updatePhase() const;
	virtual void start();
	// 1ティック進める。立っていてよろけていなければ、追う先を考えて向きを変え・歩き、
	// アニメを進め、倒れた後の時間を進め、部位の当たり判定をそのポーズへ置き直す。頂点の変形は描くときに行う。
	virtual void update();
	// 生きている間だけ体力を減らし、のけぞらせ、当たった部位が 0 になったら切り、条件を満たせばよろけさせる。
	// 脚を失ったら倒れて Downed に、倒れる条件を満たしたら死亡アニメへ繋いで倒し始める（IDamageable）。
	virtual void applyDamage( const DamageInfo& damage );
	// 移動の当たり判定（体を真上から囲む長方形。体の向きに合わせて回る。体を引きずっている間は胸に置く）。
	virtual BoxCollider collider() const;

private:
	void startFalling();      // 死亡アニメへ繋ぎ（Downed なら倒れる途中のまま続け）、Falling にする
	void startDowned();       // 前へ倒れるアニメ（無ければ死亡アニメ）へ繋ぎ、Downed にする（倒れたまま生きている）
	// 倒れ始めた（脚を失って倒れたときも）。移動の名簿から外れ、死体・倒れた体の上を歩けるようにする。
	void stopBlocking();
	// ── 体を引きずる（Downed の間。SummaryPlan/個別仕様/ZombieCrawl_Plan.txt）──
	void updateDowned( const ChaseTarget* target, const HitWorld& hitWorld );
	bool canStartCrawling() const; // 前へ倒れるアニメで倒れ終わったか（引きずれる骨がそろっているか）
	void startCrawling();
	ZombieCrawler::Effort crawlChase( const ChaseTarget* target, const HitWorld& hitWorld, bool* moving );
	// part が 0 になっていて、まだあれば切る（見た目から消して切れた部位を覚えるのは ZombieDismemberment、部位の箱を外すのはここ）。
	void severIfBroken( BodyPart part );
	void updateAfterDeath();  // Falling → Corpse → Sinking と進め、沈む分だけ見た目を下げる
	bool canStagger( BodyPart part ) const;
	void startStagger();      // よろけのアニメへ繋ぐ
	void updateStagger();     // よろけが終わったら歩く・立つのアニメへ戻し、間あけを数える
	// 追う先を考えて向きを変え、追いついたら攻撃を始め、そうでなければ歩く・横へよける（立っていて、よろけていなくて、攻撃していないときだけ）。
	void chase( const ChaseTarget* target, const HitWorld& hitWorld, const ZombieCrowd* crowd );
	// 今揺らしたい強さ（0〜1）。立って自キャラを追っていて（よろけ・攻撃の最中でない）、近いほど強い。それ以外は 0。
	float swayStrength( const ChaseTarget* target ) const;
	bool canStartAttack( const ChaseTarget* target ) const;
	void startAttack();       // 攻撃のアニメへ繋ぐ
	void approachDuringAttack( const ChaseTarget* target );
	void updateAttack( const ChaseTarget* target );
	bool isTargetInReach( const ChaseTarget* target ) const;
	ZombieAttackReach::Origin attackOrigin() const;
	float flatDistanceTo( const GameLib::Math::Vector3& target ) const;
	ZombieLocomotion::Posture posture() const;
	bool canChase() const;    // 追ってくる体か（歩くアニメがある＝T ポーズの確認用ではない）

	const ZombieModel* mModel; // 借りているだけ
	TickContext* mContext;     // 借りているだけ
	GameLib::Math::Random mRandom; // 体ごとの乱数（頭のクリティカルを引く）
	// 描く直前の姿勢の書き換え。引きずる腕 → のけぞり → 失った部位を潰す、の順に掛ける
	// （撃たれた腕は引きずる形から押され、のけぞった肩の位置で切り口を作るため）。掛ける順は mPoseModifiers へ足す順で決まる。
	BonePoseModifierChain mPoseModifiers;
	State mState;
	// ── 同じ物の部品（start で借りる）──
	GameLib::Math::Vector3* mPosition; // 足元の位置（TransformComponent。沈んでも変えない。沈む分は見た目の位置だけ下げる）
	float* mAngleY;                    // 向き（度。TransformComponent）
	SkinnedMeshInstance* mMesh;
	HitboxSet* mHitboxes;
	CharacterColliderComponent* mCollider;
	const CharacterMover* mMover;      // 壁・他のキャラに沿って滑る移動
	ZombieDismemberment* mDismemberment;
	ZombieCrawler* mCrawler;
	ZombieFlinch* mFlinch;
	ZombieHealth* mHealth;
	ZombieStagger* mStagger;
	ZombieAttack* mAttack;
	ZombieCorpse* mCorpse;
	ZombieSway* mSway;
	ZombieLocomotion* mLocomotion;
	ZombieRootMotionSpan* mRootSpan;
	ZombieAnimationSet* mAnimations;
};

#endif
