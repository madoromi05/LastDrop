#ifndef INCLUDED_ITEM_SOURCE_COMPONENT_H
#define INCLUDED_ITEM_SOURCE_COMPONENT_H

#include "GameSrc/World/Component.h"
#include "GameSrc/Interaction/IInteractable.h"

class ItemComponent;

// 何度でも拾える物の置き場（車の横のガソリン缶など）の「渡す」部品。見て操作キーで、同じ物の ItemComponent のアイテムが持ち物に 1 つ増える。
// 拾っても減らず、消えない（PickupComponent との違い）。一度に持てる数は変えない（持ち物の上限は Inventory が決める）。
class ItemSourceComponent : public Component, public IInteractable{
public:
	ItemSourceComponent();

	// Component（ItemComponent が要る）
	virtual void start();
	// IInteractable
	virtual bool canInteract( const Inventory& inventory ) const;
	virtual void interact( Inventory* inventory );

private:
	const ItemComponent* mItem;
};

#endif
