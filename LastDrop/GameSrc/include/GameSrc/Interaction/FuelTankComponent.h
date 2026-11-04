#ifndef INCLUDED_FUEL_TANK_COMPONENT_H
#define INCLUDED_FUEL_TANK_COMPONENT_H

#include "GameSrc/World/Component.h"
#include "GameSrc/Interaction/IInteractable.h"

// 脱出用の車の燃料タンク。ガソリンを決まった数だけ入れると走れる（isFueled）。見て操作キーで、持っているガソリンを1缶ずつ入れる。
// 見た目と箱は同じ物の StaticModelComponent・BoxColliderComponent。
// 走れるようになった後どうするか（クリア画面へ進む）は知らない（シーンが isFueled を見て決める）。
class FuelTankComponent : public Component, public IInteractable{
public:
	// requiredFuel: 走れるようになるまでに入れるガソリンの数（1 以上）。
	explicit FuelTankComponent( int requiredFuel );

	int fuelDelivered() const;
	int fuelRequired() const;
	// ガソリンが決まった数だけ入ったか。
	bool isFueled() const;

	// IInteractable
	virtual bool canInteract( const Inventory& inventory ) const;
	virtual void interact( Inventory* inventory );

private:
	int mFuelRequired;
	int mFuelDelivered;
};

#endif
