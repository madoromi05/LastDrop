#ifndef INCLUDED_ZOMBIE_HEALTH_COMPONENT_H
#define INCLUDED_ZOMBIE_HEALTH_COMPONENT_H

#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Random.h"
#include "GameSrc/World/Component.h"
#include "GameSrc/Hit/IDamageable.h"
#include "GameSrc/Hit/BodyPart.h"
#include "GameSrc/Zombie/ZombieHealth.h"
#include "GameSrc/Zombie/ZombieFlinch.h"
#include "GameSrc/Zombie/ZombieStagger.h"
#include <memory>

class ZombieModel;
class SkinnedMeshInstance;
class HitboxSet;
class CharacterColliderComponent;
class ZombieDismemberment;
class ZombieStateComponent;
class ZombieLocomotion;
class ZombieAnimationSet;

// ゾンビの体力と、撃たれたときの反応（IDamageable）。体力の最大は体ごとに違う（作るときに丈夫さを選ぶ。
// SummaryPlan/個別仕様/ZombieUncertainty_Plan.txt）。
// 生きている間に撃たれると体力が減り、当たった部位がのけぞり（ZombieFlinch）、部位が 0 になったら切り、ときどきよろける（ZombieStagger）。
// 頭に当たると、確率で頭が弾けて倒れる（クリティカル。確率は ZombieHealth の表、乱数を引くのはここ）。
// 脚を失うと前へ倒れるアニメで倒れ、倒れたまま生きている（Downed）。倒れる条件を満たすと死亡アニメで倒れ始める（Falling）。
// どう反応するかを選ぶのは ZombieDamageReaction、ここはそれを動かす。倒れ始めたら移動の名簿から外れる（死体の上を歩ける）。
class ZombieHealthComponent : public Component, public IDamageable{
public:
	// toughness: 丈夫さ（体力の最大に掛ける）。random: 体ごとの乱数（頭のクリティカルを引く）。
	// のけぞる骨・切り口の設計図は ZombieModelComponent から借りる。
	ZombieHealthComponent( float toughness, GameLib::Math::Random random );
	~ZombieHealthComponent();

	const ZombieHealth& health() const { return mHealth; }
	ZombieFlinch& flinch(){ return *mFlinch; } // awake で作る
	const ZombieFlinch& flinch() const { return *mFlinch; }
	// よろけのアニメの最中か（生きている間だけ true になりうる）。
	bool isStaggering() const { return mStagger.isActive(); }
	// よろけが終わったら歩く・立つのアニメへ戻し、次のよろけまでの間あけを数える（立っている間、毎ティック）。
	void updateStagger();

	// Component（ZombieModel・Transform・SkinnedModel・Hitbox・CharacterCollider・ZombieDismember・ZombieState・
	// ZombieChase・ZombieRootMotion が要る。ZombieAttack は任意＝付いていれば、倒れる・よろける時に攻撃をやめさせる）
	void awake() override;
	void start() override;
	// 生きている間だけ体力を減らし、のけぞらせ、当たった部位が 0 になったら切り、条件を満たせばよろけさせる。
	// 脚を失ったら倒れて Downed に、倒れる条件を満たしたら死亡アニメへ繋いで倒し始める（IDamageable）。
	void applyDamage( const DamageInfo& damage ) override;

private:
	void startFalling();      // 死亡アニメへ繋ぎ（Downed なら倒れる途中のまま続け）、Falling にする
	void startDowned();       // 前へ倒れるアニメ（無ければ死亡アニメ）へ繋ぎ、Downed にする（倒れたまま生きている）
	// 倒れ始めた（脚を失って倒れたときも）。腕を振っていたらやめ、移動の名簿から外れる。
	void stopActing();
	// part が 0 になっていて、まだあれば切る（見た目から消して切れた部位を覚えるのは ZombieDismemberment、部位の箱を外すのはここ）。
	void severIfBroken( BodyPart part );
	bool canStagger( BodyPart part ) const;
	void startStagger();      // よろけのアニメへ繋ぐ

	GameLib::Math::Random mRandom;
	ZombieHealth mHealth;
	std::unique_ptr< ZombieFlinch > mFlinch; // 設計図を借りるので awake で作る
	ZombieStagger mStagger; // よろけの最中か・次のよろけまでの間あけ
	// ── 同じ物の部品（start で借りる）──
	const ZombieModel* mModel;         // 切り口の設計図（ZombieModelComponent）
	GameLib::Math::Vector3* mPosition; // 足元の位置（TransformComponent）
	SkinnedMeshInstance* mMesh;
	HitboxSet* mHitboxes;
	CharacterColliderComponent* mCollider;
	ZombieDismemberment* mDismemberment;
	ZombieStateComponent* mState;
	ZombieLocomotion* mLocomotion;
	ZombieAnimationSet* mAnimations;
};

#endif
