#ifndef INCLUDED_MUZZLE_FLASH_COMPONENT_H
#define INCLUDED_MUZZLE_FLASH_COMPONENT_H

#include "GameSrc/World/Component.h"
#include "GameSrc/Effect/MuzzleFlash.h"

class GunComponent;
class ILightSocket;
struct LightSource;

// 銃（"AK47"）が撃ったティックに、銃口が光る演出（画像の FlipBookFirePlayerComponent）。光り方は MuzzleFlash に任せる
// Late の段で、同じ物の GunComponent が撃ったかを見て光らせる（撃ったティックの描画が光り始めになる）
// 銃は手に付いて動くので、銃口の位置と向きは持つ側に聞く
// 板を描くときは描く側（WorldRenderer）が HoldComponent::muzzle から渡し、点光源は持つ側が渡した相手（ILightSocket）に聞く
class MuzzleFlashComponent : public Component{
public:
	MuzzleFlashComponent();

	MuzzleFlash& flash();	// 描く（MuzzleFlash::draw は描くための頂点を書き換える）
	// 銃口を答える相手を渡す（手に持った側が呼ぶ）。nullptr で外す。所有権は受け取らない。外すまで生きていること
	void setSocket( const ILightSocket* socket );
	// 撃った瞬間に銃口の前に灯す点光源。光っていない・手に持たれていなければ false（out は変えない）。out: nullptr 不可
	// シーン（SceneLights）がライトの一覧を作るときに、全部の MuzzleFlashComponent をたどって読む
	bool lightSource( LightSource* out ) const;

	// Component（GunComponent が要る）
	UpdatePhase updatePhase() const override;
	void start() override;
	void update() override;

private:
	const GunComponent* mGun;
	const ILightSocket* mSocket;	// 借りているだけ。nullptr なら手に持たれていない（光を出さない）
	MuzzleFlash mFlash;
};

#endif
