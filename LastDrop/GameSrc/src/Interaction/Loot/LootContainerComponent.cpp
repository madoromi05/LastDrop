#include "GameSrc/Interaction/Loot/LootContainerComponent.h"

#include "GameLib/GameLib.h"

LootContainerComponent::LootContainerComponent( LootTableId table, GameLib::Math::Random* random, const std::vector< ItemStack >& guaranteed ) :
	mTable( table ),
	mGuaranteed( guaranteed ),
	mContents( LootTable::kColumns, LootTable::kRows ),
	mSearched( false ) {
	LootTable::roll( table, random, mGuaranteed, &mContents );
}

LootTableId LootContainerComponent::table() const {
	return mTable;
}

bool LootContainerComponent::isSearched() const {
	return mSearched;
}

void LootContainerComponent::markSearched() {
	mSearched = true;
}

float LootContainerComponent::searchSeconds() const {
	return mSearched ? 0.f : LootTable::searchSeconds( mTable );
}

const ItemGrid& LootContainerComponent::contents() const {
	return mContents;
}

void LootContainerComponent::reroll( GameLib::Math::Random* random ) {
	LootTable::roll( mTable, random, mGuaranteed, &mContents );
	mSearched = false;
}

bool LootContainerComponent::canInteract( const Inventory& ) const {
	return !mSearched || !mContents.stacks().empty();
}

void LootContainerComponent::interact( Inventory* ) {
	ASSERT( false && "LootContainerComponent::interact : open it with LootSearch / LootScreen." );
}

bool LootContainerComponent::offeredStack( ItemStack* ) const {
	return false;
}

void LootContainerComponent::takeOffered( int ) {
	ASSERT( false && "LootContainerComponent::takeOffered : not a pickup." );
}

ItemGrid* LootContainerComponent::lootGrid() {
	return &mContents;
}
