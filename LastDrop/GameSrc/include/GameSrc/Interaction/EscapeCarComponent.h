#ifndef INCLUDED_ESCAPE_CAR_COMPONENT_H
#define INCLUDED_ESCAPE_CAR_COMPONENT_H

#include "GameSrc/World/Component.h"
#include "GameSrc/Interaction/IInteractable.h"

// 今日の脱出の車の燃料タンク（FuelEscapeLoading_Plan.txt 3 章。2026-11-05 に脱出点の円から変えた）。
// 見て E を押すと、決まった秒（RaidParams::refuelSeconds）待ってから、カバンのガソリンを 1 個入れる（待つのは呼ぶ側。漁るのと同じ取りやめ）。
// 入れた数は車ごとに数える（Q6）。要る数に届いたら isFueled。届いた後にどうなるか（そばに居て脱出）は知らない
// （シーンが isFueled を見て RaidSession::openEscape を呼ぶ）。見た目と箱は同じ物の StaticModelComponent・BoxColliderComponent。
class EscapeCarComponent : public Component, public IInteractable{
public:
	// requiredFuel: 走れるようになるまでに入れるガソリンの数（1 以上。RaidParams::requiredFuel）。
	explicit EscapeCarComponent( int requiredFuel );

	int fuelDelivered() const;
	int fuelRequired() const;
	bool isFueled() const; // 要る数のガソリンが入ったか

	// IInteractable
	bool canInteract( const Inventory& inventory ) const override; // 要る数に届いていない かつ カバンにガソリンがある
	void interact( Inventory* inventory ) override;                // 待ち終えた後に呼ぶ。カバンのガソリンを 1 個入れる
	bool offeredStack( ItemStack* out ) const override;            // 拾う物ではないので false
	void takeOffered( int count ) override;                        // 呼ばれない
	ItemGrid* lootGrid() override;                                  // 漁れないので nullptr
	float useSeconds() const override;                             // RaidParams::refuelSeconds

private:
	int mFuelRequired;
	int mFuelDelivered;
};

#endif
