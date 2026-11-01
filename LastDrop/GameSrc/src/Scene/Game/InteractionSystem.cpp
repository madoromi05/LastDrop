#include "GameSrc/Scene/Game/InteractionSystem.h"
#include "GameSrc/Scene/Game/EffectManager.h"
#include "GameSrc/Component/StaticModelComponent.h"
#include "GameSrc/Component/HitboxComponent.h"
#include "GameSrc/Component/BoxColliderComponent.h"
#include "GameSrc/Interaction/PickupComponent.h"
#include "GameSrc/Interaction/ItemSourceComponent.h"
#include "GameSrc/Interaction/FuelTankComponent.h"
#include "GameSrc/Interaction/CorpseLootComponent.h"
#include "GameSrc/Zombie/ZombieStateComponent.h"
#include "GameSrc/World/GameObjectWorld.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

namespace{
	// 物の「使える」部品（見て E で使う）。拾える物・燃料タンク・置き場のどれか。無ければ nullptr。
	IInteractable* interactableOf( GameObject* object ) {
		if ( PickupComponent* pickup = object->getComponent< PickupComponent >() ) {
			return pickup;
		}
		if ( FuelTankComponent* tank = object->getComponent< FuelTankComponent >() ) {
			return tank;
		}
		return object->getComponent< ItemSourceComponent >();
	}
}

InteractionSystem::InteractionSystem( const HitWorld* hitWorld, GameObjectWorld* objects, EffectManager* effects ) :
	mHitWorld( hitWorld ),
	mObjects( objects ),
	mEffects( effects ) {
	ASSERT( hitWorld && objects && effects && "InteractionSystem : an argument is null." );
}

IInteractable* InteractionSystem::findInteractable( const Vector3& origin, const Vector3& delta, const IHittable* viewer ) {
	// 弾と同じ名簿で一番手前の物を探す（壁の向こうは見えない・死体にも当たる）。自分の体には当てない（目が体の中にあるため）。
	float nearest = 1.f;
	IInteractable* found = nullptr;
	HitWorld::Hit hit;
	if ( mHitWorld->raycast( origin, delta, viewer, &hit ) ) {
		nearest = hit.time;
		found = resolveInteractable( hit );
	}
	// 落ちている物の見る箱は名簿に無いので、ここで調べる。見た目より大きいので、手前に当たった物より近ければ見る箱を選ぶ。
	mObjects->forEach< PickupComponent >( [ &origin, &delta, &nearest, &found ]( PickupComponent& pickup ) {
		float time;
		if ( pickup.lookRaycast( origin, delta, &time ) && time <= nearest ) {
			nearest = time;
			found = &pickup;
		}
	} );
	return found;
}

IInteractable* InteractionSystem::resolveInteractable( const HitWorld::Hit& hit ) const {
	if ( hit.target ) {
		// 死体（倒れ始めた後）なら、その死体が落とした弾。生きているゾンビ・脚を失って這っているゾンビ（まだ落としていない）は使えない。
		const HitboxComponent* hitbox = mObjects->find< HitboxComponent >( hit.target );
		const ZombieStateComponent* zombie = hitbox ? hitbox->getComponent< ZombieStateComponent >() : nullptr;
		if ( !zombie || zombie->isAlive() ) {
			return nullptr;
		}
		const CorpseLootComponent* loot = zombie->getComponent< CorpseLootComponent >();
		GameObject* drop = loot ? loot->drop() : nullptr; // 弾を落とさなかった死体・もう拾われた弾なら nullptr
		return drop ? drop->getComponent< PickupComponent >() : nullptr;
	}
	// 落ちているガソリン・落ちた弾・車・置き場（弾を止める見た目の部品から、その物の使える部品をたどる）。
	if ( StaticModelComponent* model = mObjects->find< StaticModelComponent >( hit.blocker ) ) {
		return interactableOf( model->gameObject() );
	}
	return nullptr;
}

void InteractionSystem::removeTakenPickups() {
	// ガソリンも弾も同じ手順（どちらも PickupComponent を持つ物）。
	mObjects->forEach< PickupComponent >( [ this ]( PickupComponent& pickup ) {
		if ( pickup.isTaken() ) {
			mEffects->detach( pickup.getComponent< StaticModelComponent >() ); // 物が消える前に外す
			mObjects->destroyLater( pickup.gameObject() );
		}
	} );
}

void InteractionSystem::appendPickupColliders( std::vector< BoxCollider >* out ) const {
	// 見せるのはガソリンだけ（落ちた弾は足元に小さく落ちているだけで、運ぶ物ではない）。
	mObjects->forEach< PickupComponent >( [ out ]( const PickupComponent& pickup ) {
		if ( pickup.gameObject()->tag() == GameObjectTag::Fuel && !pickup.isTaken() ) {
			out->push_back( pickup.getComponent< BoxColliderComponent >()->collider() );
		}
	} );
}
