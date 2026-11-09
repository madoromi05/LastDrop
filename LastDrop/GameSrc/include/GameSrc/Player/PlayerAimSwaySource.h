#ifndef INCLUDED_PLAYER_AIM_SWAY_SOURCE_H
#define INCLUDED_PLAYER_AIM_SWAY_SOURCE_H

#include "GameSrc/Player/Camera/AimSway.h"

class GameObject;

// 照準のぶれ（AimSway）を決めるのに要る自キャラの今の状態を、自キャラの部品から集める係
// （SummaryPlan/個別仕様/設計と分割/TickOrderSplit_Plan.txt。2026-11-09 PlayerComponent::aimSwayBody から分けた）。
// 呼ぶのは PlayerFeedback::syncView（自キャラの update の後）。部品は読むだけで、何も書き換えない。
// 読む部品はどれも外せる物として扱う（無ければその分は既定の値＝ぶれを大きくしない側）：
//   Hold（持っているか・覗き具合）・PlayerLocomotion（歩きの速さと位相）・ArmPose（胸元に抱えている割合）・PlayerHealth（Danger か）
// カメラの側（AimSway）は自キャラの部品を知らない。部品の中身を知るのは、この係だけにする。
class PlayerAimSwaySource{
public:
	// player の今の状態。
	static AimSwayBody read( const GameObject& player );
};

#endif
