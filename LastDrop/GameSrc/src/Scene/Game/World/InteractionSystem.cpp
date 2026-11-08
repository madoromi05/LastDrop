#include "GameSrc/Scene/Game/World/InteractionSystem.h"
#include "GameSrc/Scene/Game/World/EffectManager.h"
#include "GameSrc/Component/StaticModelComponent.h"
#include "GameSrc/Component/HitboxComponent.h"
#include "GameSrc/Component/BoxColliderComponent.h"
#include "GameSrc/Collision/MeshBoundsBox.h"
#include "GameSrc/Interaction/PickupComponent.h"
#include "GameSrc/Interaction/Loot/LootContainerComponent.h"
#include "GameSrc/Interaction/EscapeCarComponent.h"
#include "GameSrc/Zombie/Component/ZombieStateComponent.h"
#include "GameSrc/World/GameObjectWorld.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

namespace{
	// マークを、物の見た目の箱の上端からどれだけ上に出すか（m。物に重ならず、どの物の上かが分かる高さ）。
	const float kMarkAboveTop = 0.15f;
	// 死体のマークの高さ（足元から m。倒れた体の胸の少し上。死体は見た目の箱を持たないので決め打ち）。
	const float kCorpseMarkHeight = 0.5f;

	// 物の「使える」部品（見て E で使う）。拾える物と、漁れる置物（車・ゴミ箱・木箱。R3_Plan.txt Q1）と、脱出の車。無ければ nullptr。
	IInteractable* interactableOf( GameObject* object ) {
		if ( PickupComponent* pickup = object->getComponent< PickupComponent >() ) {
			return pickup;
		}
		if ( EscapeCarComponent* car = object->getComponent< EscapeCarComponent >() ) {
			return car;
		}
		return object->getComponent< LootContainerComponent >();
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

void InteractionSystem::collectNearby( const Vector3& eye, float radius, std::vector< NearbyInteractable >* out ) const {
	ASSERT( out && "InteractionSystem::collectNearby : out is null." );
	// 使える物はどれも見た目の部品（StaticModelComponent）を持つので、見た目から使える部品をたどる（findInteractable と同じたどり方）。
	mObjects->forEach< StaticModelComponent >( [ this, &eye, radius, out ]( StaticModelComponent& model ) {
		const IInteractable* interactable = interactableOf( model.gameObject() );
		const PickupComponent* pickup = model.getComponent< PickupComponent >();
		if ( !interactable || ( pickup && pickup->isTaken() ) ) {
			return;
		}
		// 箱は見た目の頂点から作る（移動の当たり判定と同じ箱。高さは地面 Y=0 から）。
		BoxCollider box = MeshBoundsBox::make( model.mesh() );
		Vector3 mark( box.center.x, box.height + kMarkAboveTop, box.center.z );
		Vector3 toMark;
		toMark.setSub( mark, eye );
		if ( toMark.length() > radius ) {
			return;
		}
		// 目とマークの間に弾を止める物があれば隠れている。ただし、その物自身に当たったなら見えている（車の屋根の上のマークを横から見たとき）。
		HitWorld::Hit hit;
		if ( mHitWorld->raycastBlockers( eye, toMark, &hit ) && mObjects->find< StaticModelComponent >( hit.blocker ) != &model ) {
			return;
		}
		out->push_back( NearbyInteractable{ interactable, mark } );
	} );
	// 死体（漁れる場所の部品を付けた、倒れたゾンビ）。見た目の部品は骨の付いた形なので、足元から決めた高さに出す。
	mObjects->forEach< LootContainerComponent >( [ this, &eye, radius, out ]( LootContainerComponent& loot ) {
		const ZombieStateComponent* zombie = loot.getComponent< ZombieStateComponent >();
		if ( !zombie ) {
			return; // 置物（上で入れた）
		}
		Vector3 mark = zombie->position();
		mark.y += kCorpseMarkHeight;
		Vector3 toMark;
		toMark.setSub( mark, eye );
		if ( toMark.length() > radius ) {
			return;
		}
		HitWorld::Hit hit;
		if ( mHitWorld->raycastBlockers( eye, toMark, &hit ) ) {
			return;
		}
		out->push_back( NearbyInteractable{ &loot, mark } );
	} );
}

void InteractionSystem::rerollLoot( int seed ) {
	Random random = Random::create( seed );
	mObjects->forEach< LootContainerComponent >( [ &random ]( LootContainerComponent& loot ) {
		loot.reroll( &random );
	} );
}

void InteractionSystem::markAllLootSearched() {
	mObjects->forEach< LootContainerComponent >( []( LootContainerComponent& loot ) {
		loot.markSearched();
	} );
}

LootContainerComponent* InteractionSystem::findContainer( const IInteractable* interactable ) {
	LootContainerComponent* found = nullptr;
	mObjects->forEach< LootContainerComponent >( [ interactable, &found ]( LootContainerComponent& loot ) {
		if ( static_cast< const IInteractable* >( &loot ) == interactable ) {
			found = &loot;
		}
	} );
	return found;
}

EscapeCarComponent* InteractionSystem::findEscapeCar( const IInteractable* interactable ) {
	EscapeCarComponent* found = nullptr;
	mObjects->forEach< EscapeCarComponent >( [ interactable, &found ]( EscapeCarComponent& car ) {
		if ( static_cast< const IInteractable* >( &car ) == interactable ) {
			found = &car;
		}
	} );
	return found;
}

IInteractable* InteractionSystem::resolveInteractable( const HitWorld::Hit& hit ) const {
	if ( hit.target ) {
		// 死体（止めを刺した後）なら、その死体の漁れる場所（R3_Plan.txt Q10）。
		// 生きているゾンビ・脚を失って這っているゾンビ（まだ止めを刺していない）は使えない（部品がまだ付いていない）。
		const HitboxComponent* hitbox = mObjects->find< HitboxComponent >( hit.target );
		const ZombieStateComponent* zombie = hitbox ? hitbox->getComponent< ZombieStateComponent >() : nullptr;
		if ( !zombie || zombie->isAlive() ) {
			return nullptr;
		}
		return zombie->getComponent< LootContainerComponent >();
	}
	// 落ちている物・漁れる置物（弾を止める見た目の部品から、その物の使える部品をたどる）。
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
