#include "GameSrc/Scene/Game/InteractionSystem.h"
#include "GameSrc/Scene/Game/EffectManager.h"
#include "GameSrc/Component/StaticModelComponent.h"
#include "GameSrc/Component/HitboxComponent.h"
#include "GameSrc/Component/BoxColliderComponent.h"
#include "GameSrc/Collision/MeshBoundsBox.h"
#include "GameSrc/Interaction/PickupComponent.h"
#include "GameSrc/Interaction/CorpseLootComponent.h"
#include "GameSrc/Zombie/ZombieStateComponent.h"
#include "GameSrc/World/GameObjectWorld.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

namespace{
	// マークを、物の見た目の箱の上端からどれだけ上に出すか（m。物に重ならず、どの物の上かが分かる高さ）。
	const float kMarkAboveTop = 0.15f;

	// 物の「使える」部品（見て E で使う）。今は拾える物だけ（2026-11-05 に車の燃料タンクと何度でも拾える置き場を消した。R2_Plan.txt Q15）。
	// 無ければ nullptr。
	IInteractable* interactableOf( GameObject* object ) {
		return object->getComponent< PickupComponent >();
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
	// 落ちているガソリン・落ちた弾（弾を止める見た目の部品から、その物の使える部品をたどる）。
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
