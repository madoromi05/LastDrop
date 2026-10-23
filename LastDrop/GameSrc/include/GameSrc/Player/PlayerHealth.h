#ifndef INCLUDED_PLAYER_HEALTH_H
#define INCLUDED_PLAYER_HEALTH_H

#include "GameSrc/Player/PlayerBodyZone.h"

// 自キャラの体力。タルコフと同じく 7 部位（PlayerBodyZone）ごとに持ち、全体の体力は持たない
// （SummaryPlan/個別仕様/ZombieAttack_Plan.txt の 2 章）。
// ・頭か胸が 0 になったら死ぬ
// ・ほかの部位は 0 で「壊れた」になる。壊れた部位へのダメージ（0 を超えた分も）は減らしてから、壊れていない残りの部位へ同じ量ずつ分ける
//   （散った分で頭・胸が 0 になれば死ぬ）
// ・脚が片方でも壊れたら、走れず、歩くのも遅くなる（読むのは PlayerCharacter）
// 回復はしない（回復アイテムを作るときに入口を足す）。部位ごとの最大の体力・0 で死ぬか・噛まれる所の割合の表は、この .cpp だけが知っている。
class PlayerHealth{
public:
	PlayerHealth(); // 全部満タンで始める

	// zone に amount のダメージを受ける。死んだ後は何もしない。
	void takeDamage( PlayerBodyZone zone, float amount );
	// ゾンビに噛まれる所を選ぶ。roll: 0 以上 1 未満の乱数（呼ぶ側が引く。体力は乱数を持たない）。
	static PlayerBodyZone biteZone( float roll );
	// 脚を失って体を引きずるゾンビに足元から噛まれる所（左脚か右脚を半々）。roll は biteZone と同じ。
	static PlayerBodyZone legBiteZone( float roll );

	bool isDead() const;                  // 頭か胸が 0 になったか
	float part( PlayerBodyZone zone ) const;    // 部位の残り（0 以上）
	float maxPart( PlayerBodyZone zone ) const; // 部位の最大
	bool isBroken( PlayerBodyZone zone ) const; // 部位が 0 になったか（頭・胸も、0 なら true）
	bool canRun() const;                  // 走れるか（脚がどちらも壊れていない）
	float walkSpeedScale() const;         // 歩く・走る速さに掛ける倍率（脚が壊れていれば 1 より小さい）
private:
	// zone から amount を引く。0 を超えた分（引ききれなかった分）を返す。
	float subtract( PlayerBodyZone zone, float amount );
	// 壊れた部位が受けた amount を減らして、壊れていない部位へ同じ量ずつ分ける。
	void spread( float amount );
	bool isLegBroken() const;

	float mParts[ static_cast< int >( PlayerBodyZone::Count ) ]; // PlayerBodyZone の番号で引く
};

#endif
