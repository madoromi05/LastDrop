#include "GameSrc/Interaction/EscapeCarComponent.h"
#include "GameSrc/Interaction/Inventory.h"
#include "GameSrc/Interaction/ItemCatalog.h"
#include "GameSrc/Raid/RaidParams.h"

#include "GameLib/GameLib.h"
#include "GameLib/Profiling/Profiler.h"

EscapeCarComponent::EscapeCarComponent( int requiredFuel ) :
	mFuelRequired( requiredFuel ),
	mFuelDelivered( 0 ) {
	ASSERT( requiredFuel > 0 && "EscapeCarComponent : requiredFuel must be positive." );
}

int EscapeCarComponent::fuelDelivered() const {
	return mFuelDelivered;
}

int EscapeCarComponent::fuelRequired() const {
	return mFuelRequired;
}

bool EscapeCarComponent::isFueled() const {
	return mFuelDelivered >= mFuelRequired;
}

bool EscapeCarComponent::canInteract( const Inventory& inventory ) const {
	return !isFueled() && inventory.count( ITEM_FUEL ) > 0;
}

void EscapeCarComponent::interact( Inventory* inventory ) {
	ASSERT( inventory && canInteract( *inventory ) && "EscapeCarComponent::interact : cannot refuel." );
	PROFILE_EVENT( "Refuel" ); // 計測の印
	inventory->remove( ITEM_FUEL ); // 同じ物なら、どのマスから減っても同じ
	++mFuelDelivered;
}

bool EscapeCarComponent::offeredStack( ItemStack* ) const {
	return false;
}

void EscapeCarComponent::takeOffered( int ) {
	ASSERT( false && "EscapeCarComponent::takeOffered : not a pickup." );
}

ItemGrid* EscapeCarComponent::lootGrid() {
	return nullptr;
}

float EscapeCarComponent::useSeconds() const {
	return raidParams().refuelSeconds;
}
