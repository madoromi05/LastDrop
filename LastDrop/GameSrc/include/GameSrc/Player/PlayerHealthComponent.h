#ifndef INCLUDED_PLAYER_HEALTH_COMPONENT_H
#define INCLUDED_PLAYER_HEALTH_COMPONENT_H

#include "GameSrc/World/Component.h"
#include "GameSrc/Player/PlayerHealth.h"

// 自キャラの体力（体全体で 1 つ。バイオハザード式。中身は PlayerHealth）と、デバッグ用の無敵。
// ゾンビに噛まれると減る（applyZombieHit。どのゾンビが攻撃したかは CombatResolver が決める）。
// 傷つくほど歩く・走るのが遅く（walkSpeedScale）、0 で死に、それからは操作を受け付けない（PlayerComponent が止める）。
class PlayerHealthComponent : public Component{
public:
	PlayerHealthComponent() : mInvincible( false ){}

	// ゾンビに噛まれた・這うゾンビの腕が当たった（どちらも同じダメージ）。damage: ダメージの量。死んだ後・無敵の間は何もしない。
	void applyZombieHit( float damage ){
		if ( mInvincible ){
			return;
		}
		mHealth.takeDamage( damage );
	}
	// 包帯などで amount だけ戻す（最大で止める・死んだ後は戻らない。R3_Plan.txt Q11）。
	void heal( float amount ){ mHealth.heal( amount ); }
	bool canBeAttacked() const { return !isDead(); } // ゾンビが攻撃してよいか（生きている）
	bool isDead() const { return mHealth.isDead(); }
	const PlayerHealth& health() const { return mHealth; }
	void toggleInvincible(){ mInvincible = !mInvincible; } // デバッグ用（F5）。無敵の間は体力が減らない
	bool isInvincible() const { return mInvincible; }     // F6 のエディタに出すため

private:
	PlayerHealth mHealth;
	bool mInvincible;
};

#endif
