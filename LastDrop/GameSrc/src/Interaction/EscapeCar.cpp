#include "GameSrc/Interaction/EscapeCar.h"
#include "GameSrc/Interaction/Inventory.h"
#include "GameSrc/Hit/HitWorld.h"

#include "GameLib/GameLib.h"
#include "GameLib/Profiling/Profiler.h"
#include <string>

using namespace GameLib::Math;

namespace{
	const GroundMaterial kBodyMaterial = GROUND_METAL; // 車体（弾が当たると金属の音）
}

EscapeCar::EscapeCar( const StaticMeshData* data, const Vector3& position, float angleY, int requiredFuel ) :
	mBody( data, position, angleY, kBodyMaterial ),
	mFuelRequired( requiredFuel ),
	mFuelDelivered( 0 ) {
	ASSERT( requiredFuel > 0 && "EscapeCar : requiredFuel must be positive." );
}

void EscapeCar::addTo( CollisionWorld* collisionWorld, HitWorld* hitWorld ) const {
	ASSERT( collisionWorld && hitWorld && "EscapeCar : world is null." );
	mBody.addColliderTo( collisionWorld );
	hitWorld->addBlocker( &mBody );
}

void EscapeCar::draw( const ViewFrustum& frustum ) const {
	mBody.draw( frustum );
}

bool EscapeCar::isBody( const IRayBlocker* blocker ) const {
	return blocker == &mBody;
}

const BoxCollider& EscapeCar::collider() const {
	return mBody.collider();
}

int EscapeCar::fuelDelivered() const {
	return mFuelDelivered;
}

int EscapeCar::fuelRequired() const {
	return mFuelRequired;
}

bool EscapeCar::isFueled() const {
	return mFuelDelivered >= mFuelRequired;
}

bool EscapeCar::canInteract( const Inventory& inventory ) const {
	return !isFueled() && inventory.count( ITEM_FUEL ) > 0;
}

std::string EscapeCar::prompt( const Inventory& inventory ) const {
	std::string tank = std::to_string( mFuelDelivered ) + "/" + std::to_string( mFuelRequired );
	if ( isFueled() ) {
		return "Fuel tank full";
	}
	if ( inventory.count( ITEM_FUEL ) == 0 ) {
		return "Need " + std::string( itemSpec( ITEM_FUEL ).name ) + " (" + tank + ")";
	}
	return "[E] Refuel (" + tank + ")";
}

void EscapeCar::interact( Inventory* inventory ) {
	ASSERT( inventory && canInteract( *inventory ) && "EscapeCar::interact : cannot refuel." );
	PROFILE_EVENT( "Refuel" ); // 計測の印（給油の数でゾンビが増える）
	inventory->remove( ITEM_FUEL );
	++mFuelDelivered;
}
