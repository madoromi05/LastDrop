#ifndef INCLUDED_MUZZLE_FLASH_COMPONENT_H
#define INCLUDED_MUZZLE_FLASH_COMPONENT_H

#include "GameSrc/World/Component.h"
#include "GameSrc/Effect/MuzzleFlash.h"

class GunComponent;

// 銃（"AK47"）が撃ったティックに、銃口が光る演出（画像の FlipBookFirePlayerComponent）。光り方は MuzzleFlash に任せる。
// Late の段で、同じ物の GunComponent が撃ったかを見て光らせる（撃ったティックの描画が光り始めになる）。
// 銃口の位置と向きは、描くたびに持つ側（HoldComponent::muzzle）から受け取る（銃は手に付いて動くため）。
class MuzzleFlashComponent : public Component{
public:
	MuzzleFlashComponent();

	MuzzleFlash& flash();             // 描く（MuzzleFlash::draw は描くための頂点を書き換える）
	const MuzzleFlash& flash() const; // 光（点光源）を読む

	// Component（GunComponent が要る）
	UpdatePhase updatePhase() const override;
	void start() override;
	void update() override;

private:
	const GunComponent* mGun;
	MuzzleFlash mFlash;
};

#endif
