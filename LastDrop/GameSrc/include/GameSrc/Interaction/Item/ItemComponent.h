#ifndef INCLUDED_ITEM_COMPONENT_H
#define INCLUDED_ITEM_COMPONENT_H

#include "GameSrc/World/Component.h"
#include "GameSrc/Interaction/Item/ItemCatalog.h"
#include "GameLib/GameLib.h"

// この物が何のアイテムで、いくつあるか（落ちているガソリン・落ちた弾・落ちている銃）
// どう渡すか（拾うと減って消える）は、同じ物の PickupComponent が決める
class ItemComponent : public Component{
public:
	// item: 何のアイテムか。amount: いくつあるか（1 以上）
	ItemComponent( ItemId item, int amount ) :
		mItem( item ),
		mAmount( amount ){
		ASSERT( amount > 0 && "ItemComponent : amount must be positive." );
	}

	ItemId item() const { return mItem; }
	int amount() const { return mAmount; }
	// 1 つ渡した（拾われた）。0 まで減ったら、もう渡せない
	void takeOne(){
		ASSERT( mAmount > 0 && "ItemComponent : nothing left." );
		--mAmount;
	}

private:
	ItemId mItem;
	int mAmount;
};

#endif
