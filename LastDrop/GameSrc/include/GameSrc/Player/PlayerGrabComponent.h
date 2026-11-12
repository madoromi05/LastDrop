#ifndef INCLUDED_PLAYER_GRAB_COMPONENT_H
#define INCLUDED_PLAYER_GRAB_COMPONENT_H

#include "GameSrc/World/Component.h"

// 自キャラが「ゾンビにつかまれる」部品（SummaryPlan/個別仕様/ゾンビ/ZombieAttack_Plan.txt の 4 章）
// つかまれているか・離されてから、またつかまれるようになるまでのクールダウンを持つ。自分では動かない（Update の段なし）
//   書く：ZombieDirector（ゾンビの Think の段の後に、つかんでいる体がいるかを 1 ティックに 1 回 setHeld）
//   読む：ZombieDirector（canBeGrabbed。つかんでよいか）・PlayerComponent（isHeld。つかまれている間は動けない）
// 外してよい部品：外すと、ゾンビは自キャラを襲わない（攻撃を始めない。Hitbox を外すと撃たれない、と同じ形）
class PlayerGrabComponent : public Component{
public:
	PlayerGrabComponent();

	// つかまれているか。次の update から動けなくなる・動けるようになる
	// つかまれている間も、撃つ・覗く・視点を回すはできる（撃って振りほどく）
	void setHeld( bool held );
	bool isHeld() const;
	// つかんでよいか（つかまれていない・離されてから少したった）。続けてつかまれて抜け出せなくならないように。生きているかは呼ぶ側が見る
	bool canBeGrabbed() const;

private:
	bool mHeld;             // ゾンビにつかまれているか（setHeld）
	int mGrabCooldownTicks; // 離されてから、またつかまれるようになるまでの残り（ティック）
};

#endif
