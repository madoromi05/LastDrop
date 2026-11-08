#ifndef INCLUDED_ESCAPE_CAR_COMPONENT_H
#define INCLUDED_ESCAPE_CAR_COMPONENT_H

#include "GameSrc/World/Component.h"
#include "GameSrc/Interaction/IInteractable.h"
#include "GameSrc/Interaction/Item/ItemGrid.h"

// 今日の脱出の車の給油口（FuelEscapeLoading_Plan.txt 3 章・FuelPort_Plan.txt。2026-11-06 にマス目へ入れる形へ変えた）。
// 見て E を押すと、漁れる場所と同じ画面で給油口のマス目（ガソリン 1 個分 × 要る数）を見せる（開けるのは呼ぶ側）。
// 給油口に要る数のガソリンが入った状態で画面を閉じると、呼ぶ側が決まった秒（RaidParams::refuelSeconds）待ってから startEngine を呼ぶ。
// 給油が済んだら（isFueled）もう開けない。済んだ後にどうなるか（そばに居て脱出）は知らない
// （シーンが isFueled を見て RaidSession::openEscape を呼ぶ）。見た目と箱は同じ物の StaticModelComponent・BoxColliderComponent。
class EscapeCarComponent : public Component, public IInteractable{
public:
	// requiredFuel: 走れるようになるまでに入れるガソリンの数（1 以上。RaidParams::requiredFuel）。給油口はこの数だけ行を持つ。
	explicit EscapeCarComponent( int requiredFuel );

	// 給油口に要る数のガソリンが入っているか（閉じたときに給油を始めてよいか）。
	bool isTankFull() const;
	// 給油口のガソリンを使い切って走れるようにする。isTankFull のときだけ呼ぶこと。
	void startEngine();
	bool isFueled() const; // 給油が済んだか
	// 給油口のマス目（見るだけ。画面に描く）。
	const ItemGrid& tank() const;

	// IInteractable
	bool canInteract( const Inventory& inventory ) const override; // 給油が済むまではいつでも開ける（ガソリンが無くても、要る数が見える）
	void interact( Inventory* inventory ) override;                // 呼ばれない（Interactor は lootGrid のある物を opened へ回す）
	bool offeredStack( ItemStack* out ) const override;            // 拾う物ではないので false
	void takeOffered( int count ) override;                        // 呼ばれない
	ItemGrid* lootGrid() override;                                  // 給油口のマス目

private:
	int mFuelRequired;
	ItemGrid mTank;
	bool mFueled;
};

#endif
