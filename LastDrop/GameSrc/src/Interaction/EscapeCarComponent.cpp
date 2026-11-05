#include "GameSrc/Interaction/EscapeCarComponent.h"
#include "GameSrc/Interaction/ItemCatalog.h"

#include "GameLib/GameLib.h"
#include "GameLib/Profiling/Profiler.h"

EscapeCarComponent::EscapeCarComponent( int requiredFuel ) :
	mFuelRequired( requiredFuel ),
	// 1 行にガソリン 1 個（横 2 マス）。行の数 = 要る数なので、ちょうど要る数だけ入る（FuelPort_Plan.txt）。
	mTank( itemSpec( ITEM_FUEL ).cellSize, requiredFuel ),
	mFueled( false ) {
	ASSERT( requiredFuel > 0 && "EscapeCarComponent : requiredFuel must be positive." );
}

bool EscapeCarComponent::isTankFull() const {
	return mTank.total( ITEM_FUEL ) >= mFuelRequired;
}

void EscapeCarComponent::startEngine() {
	ASSERT( !mFueled && isTankFull() && "EscapeCarComponent::startEngine : the tank is not full." );
	PROFILE_EVENT( "Refuel" ); // 計測の印
	mTank.clearAll(); // 入れたガソリンは使い切る（もう開けないので、取り出せない）
	mFueled = true;
}

bool EscapeCarComponent::isFueled() const {
	return mFueled;
}

const ItemGrid& EscapeCarComponent::tank() const {
	return mTank;
}

bool EscapeCarComponent::canInteract( const Inventory& ) const {
	return !mFueled;
}

void EscapeCarComponent::interact( Inventory* ) {
	ASSERT( false && "EscapeCarComponent::interact : open the fuel port with LootScreen." );
}

bool EscapeCarComponent::offeredStack( ItemStack* ) const {
	return false;
}

void EscapeCarComponent::takeOffered( int ) {
	ASSERT( false && "EscapeCarComponent::takeOffered : not a pickup." );
}

ItemGrid* EscapeCarComponent::lootGrid() {
	return &mTank;
}
