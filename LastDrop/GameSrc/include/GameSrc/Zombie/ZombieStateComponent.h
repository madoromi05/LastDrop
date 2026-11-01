#ifndef INCLUDED_ZOMBIE_STATE_COMPONENT_H
#define INCLUDED_ZOMBIE_STATE_COMPONENT_H

#include "GameLib/Math/Vector3.h"
#include "GameSrc/World/Component.h"
#include "GameSrc/Collision/ICollisionBody.h"
#include "GameSrc/Zombie/ZombieLocomotion.h"

class ZombieModel;
class HitboxSet;
class ZombieCrawler;
class ZombieRootMotionSpan;
class ZombieAnimationSet;

// ゾンビ 1 体が今どの状態か（立っている・倒れている・死体）と、体のどこに居るか（足元・体の中心・頭・目）を答える部品
// （SummaryPlan/個別仕様/ZombieBrainSplit_Plan.txt）。ゾンビの判断の部品（Chase・Attack・Health・Crawl・Corpse）はみなここを読む。
// ここはほかの判断の部品を知らない（読むのは Transform・Hitbox・ZombieDismember・ZombieRootMotion だけ）。
// 状態を変えるのは ZombieHealthComponent（Alive → Downed / Falling）と ZombieCorpseComponent（Falling → Corpse → Sinking）だけ。
// 移動の当たり判定の形（体を引きずっている間は胸に置く）を答えるのもここ（ICollisionBody。名簿に載るのは CharacterColliderComponent）。
class ZombieStateComponent : public Component, public ICollisionBody{
public:
	// 生きているか・倒れているか。並びは時間の順。
	enum class State{
		Alive,   // 立っている（撃たれると体力が減る）
		Downed,  // 脚を失って倒れ、倒れたまま生きている（撃たれると体力が減る。よろけない）
		Falling, // 死亡アニメの途中
		Corpse,  // 倒れ終わって、死体として残っている
		Sinking, // 地面へ沈んでいる（沈み終わったら消えてよい）
	};

	ZombieStateComponent();

	State state() const;
	void setState( State state );
	bool isAlive() const;    // 生きているか（Alive か Downed）
	// 脚を失って倒れ終わり、腕で体を引きずっているか（Downed の間だけ true になりうる）。
	bool isCrawling() const;
	// 追ってくる体か（歩くアニメがある＝T ポーズの確認用ではない）。
	bool canChase() const;
	ZombieLocomotion::Posture posture() const;
	// 足元の位置（ワールド）。攻撃・よろけ・倒れる間は腰に付いて動く（倒れた後は死体の腰の下）。
	const GameLib::Math::Vector3& position() const;
	// 体の中心（ワールド）。立っている間は足元、体を引きずっている間は胸。追う・回り込む・壁に当たるのはこの位置。
	// 攻撃・よろけ・倒れるアニメの間は、始めた所（根の動きのアンカー。腕が届く距離もここから測る）。
	GameLib::Math::Vector3 bodyCenter() const;
	// 立っているときの目の位置（足元から目の高さ）。ここから自キャラの目が見えているかを調べる。
	GameLib::Math::Vector3 eyePosition() const;
	// 頭の位置（ワールド。頭の部位の箱の中心）。攻撃の音を鳴らす所。頭を失っていれば目の位置。
	GameLib::Math::Vector3 headPosition() const;

	// Component（ZombieModel・Transform・Hitbox・CharacterCollider・ZombieDismember・ZombieRootMotion が要る）
	virtual void start();
	// 移動の当たり判定（体を真上から囲む長方形。体の向きに合わせて回る。体を引きずっている間は胸に置く）。
	virtual BoxCollider collider() const;

private:
	State mState;
	// ── 同じ物の部品（start で借りる）──
	const ZombieModel* mModel;               // 移動の当たり判定の形（ZombieModelComponent）
	const GameLib::Math::Vector3* mPosition; // 足元の位置（TransformComponent）
	const float* mAngleY;                    // 向き（度。TransformComponent）
	HitboxSet* mHitboxes;
	ZombieCrawler* mCrawler;
	ZombieRootMotionSpan* mRootSpan;
	ZombieAnimationSet* mAnimations;
};

#endif
