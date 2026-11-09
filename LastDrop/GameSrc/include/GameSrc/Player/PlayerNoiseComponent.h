#ifndef INCLUDED_PLAYER_NOISE_COMPONENT_H
#define INCLUDED_PLAYER_NOISE_COMPONENT_H

#include "GameSrc/World/Component.h"

class TransformComponent;
class PlayerLocomotionComponent;
class ZombieHearing;

// 自キャラが立てる音（足音・銃声）を、ゾンビへ知らせる部品（SummaryPlan/個別仕様/設計と分割/HighPrioritySplit_Plan.txt 3-4）。
// 知らせる先は共有の棚の ZombieHearing（何体のゾンビに届くかは、そちらが決める）。
// Input の段で、PlayerComponent の後に動く（同じ段の中は付けた順に動くので、PlayerFactory で PlayerComponent の後に付ける）。
// こうすると、このティックに動いた後の足元・撃った後の銃を読める（弾とゾンビの Think より前なので、同じティックに届く）。
// 外してよい部品（外すと、ゾンビが自キャラの音を聞かない。確認用）。
class PlayerNoiseComponent : public Component{
public:
	PlayerNoiseComponent();

	// Component（TransformComponent・PlayerLocomotionComponent と、共有の棚の ZombieHearing が要る）
	UpdatePhase updatePhase() const override;
	void start() override;
	void update() override;

private:
	// 持っている銃がこのティックに撃ったら、足元で銃声を鳴らす。
	void emitGunshot();
	// 0.3 秒ごとに、歩き方で決まる距離（歩き 5m・走り 18m）の足音を鳴らす。
	void emitFootsteps();

	const TransformComponent* mTransform;
	const PlayerLocomotionComponent* mLocomotion;
	ZombieHearing* mHearing; // 共有の棚から借りる
	int mFootstepTicks;      // 次に足音を鳴らすまでのティック
};

#endif
