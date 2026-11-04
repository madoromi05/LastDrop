#include "GameSrc/Interaction/FuelTankComponent.h"
#include "GameSrc/Interaction/Inventory.h"
#include "GameSrc/Interaction/ItemCatalog.h"

#include "GameLib/GameLib.h"
#include "GameLib/Profiling/Profiler.h"
#include <string>

FuelTankComponent::FuelTankComponent( int requiredFuel ) :
	mFuelRequired( requiredFuel ),
	mFuelDelivered( 0 ) {
	ASSERT( requiredFuel > 0 && "FuelTankComponent : requiredFuel must be positive." );
}

int FuelTankComponent::fuelDelivered() const {
	return mFuelDelivered;
}

int FuelTankComponent::fuelRequired() const {
	return mFuelRequired;
}

bool FuelTankComponent::isFueled() const {
	return mFuelDelivered >= mFuelRequired;
}

bool FuelTankComponent::canInteract( const Inventory& inventory ) const {
	return !isFueled() && inventory.count( ITEM_FUEL ) > 0;
}

void FuelTankComponent::interact( Inventory* inventory ) {
	ASSERT( inventory && canInteract( *inventory ) && "FuelTankComponent::interact : cannot refuel." );
	PROFILE_EVENT( "Refuel" ); // 計測の印（給油の数でゾンビが増える）
	inventory->remove( ITEM_FUEL );
	++mFuelDelivered;
}
