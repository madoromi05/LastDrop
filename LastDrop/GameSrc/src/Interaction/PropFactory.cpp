#include "GameSrc/Interaction/PropFactory.h"
#include "GameSrc/Render/MeshCache.h"
#include "GameSrc/Component/TransformComponent.h"
#include "GameSrc/Component/StaticModelComponent.h"
#include "GameSrc/Interaction/ItemComponent.h"
#include "GameSrc/Interaction/PickupComponent.h"
#include "GameSrc/World/GameObjectWorld.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

namespace{
	const char* kVehicleFile = "Resource/uaz.txt"; // tools/configs/uaz_atlas.json → uaz.json で変換（実物大・原点は底面の中心）
	const char* kFuelCanFile = "Resource/fuelcan.txt"; // tools/configs/fuelcan_atlas.json → fuelcan.json で変換（同上）
	// 落ちた弾の形のファイルは物の表（ItemCatalog の groundFile）、落ちている銃の形のファイルは銃ごとの表（WeaponCatalog の groundFile）にある。

	// ── 物の名前（確認用。F6 のエディタの物の一覧・デバッグ出力。ゲームの判断はタグで行う）──
	const char* kFuelName = "Fuel";
	const char* kAmmoDropName = "AmmoDrop";
	const char* kWeaponDropName = "WeaponDrop";
	const int kOneWeapon = 1; // 落ちている銃は 1 丁
	const char* kParkedCarName = "ParkedCar";

	const int kFuelPerCan = 1;
	const int kAmmoDropRounds = 10;		//落とす球数
	// 置物（車・ガソリン缶・落ちた弾）の表面の材質（着弾音）。どれも金属。木箱などを置くときは、物ごとに変える。
	const GroundMaterial kPropMaterial = GROUND_METAL;
}

PropFactory::PropFactory( MeshCache* meshes ) :
	// 車と缶は粗い形（LOD）も読む（OptimizationC_Plan.txt Q7。弾の箱は小さく、2 画素未満で消えるので段を持たない）。
	mVehicleShapes( StaticMeshLods::request( meshes, kVehicleFile ) ),
	mFuelCanShapes( StaticMeshLods::request( meshes, kFuelCanFile ) ) {
	ASSERT( meshes && "PropFactory : meshes is null." );
	for ( int i = 0; i < ITEM_NUMBER; ++i ) {
		ItemId item = static_cast< ItemId >( i );
		const ItemSpec& spec = itemSpec( item );
		if ( spec.kind == ITEM_KIND_AMMO ) {
			ASSERT( spec.groundFile && "PropFactory : ammo needs groundFile." );
			mAmmoDropShapes.emplace( item, StaticMeshLods( meshes->staticMesh( spec.groundFile ) ) );
		}
	}
	for ( int i = 0; i < WEAPON_NUMBER; ++i ) {
		mWeaponDropShapes.emplace_back( meshes->staticMesh( weaponSpec( static_cast< WeaponId >( i ) ).groundFile ) );
	}
}

GameObject* PropFactory::createProp( GameObjectWorld* world, GameObjectTag tag, const char* name, const StaticMeshLods& shapes,
const Vector3& position, float yawDegree, DrawCulling::Target cullTarget, BoxColliderComponent::Mode colliderMode ) const {
	GameObject* obj = world->create( tag, name );
	obj->addComponent< TransformComponent >( position, yawDegree );
	obj->addComponent< StaticModelComponent >( shapes, kPropMaterial, cullTarget );
	obj->addComponent< BoxColliderComponent >( colliderMode ); // 箱は見た目の頂点から求める（見た目は awake で置かれる）
	return obj;
}

GameObject* PropFactory::spawnParkedCar( GameObjectWorld* world, const Vector3& position, float yawDegree ) const {
	// 箱の大きさを頂点から求めるので、構築に失敗したデータは置かない。
	if ( !mVehicleShapes.full()->isReady() ) {
		return nullptr;
	}
	GameObject* obj = createProp( world, GameObjectTag::ParkedCar, kParkedCarName, mVehicleShapes, position, yawDegree,
		DrawCulling::Target::Large, BoxColliderComponent::Mode::Fixed );
	world->spawn( obj );
	return obj;
}

GameObject* PropFactory::spawnFuel( GameObjectWorld* world, const Vector3& position, float yawDegree ) const {
	if ( !mFuelCanShapes.full()->isReady() ) {
		return nullptr;
	}
	GameObject* obj = createProp( world, GameObjectTag::Fuel, kFuelName, mFuelCanShapes, position, yawDegree,
		DrawCulling::Target::Small, BoxColliderComponent::Mode::Removable );
	obj->addComponent< ItemComponent >( ITEM_FUEL, kFuelPerCan );
	obj->addComponent< PickupComponent >();
	world->spawn( obj );
	return obj;
}

GameObject* PropFactory::spawnAmmoDrop( GameObjectWorld* world, ItemId ammo, const Vector3& position, float yawDegree ) const {
	return spawnAmmo( world, ammo, position, yawDegree, kAmmoDropRounds );
}

GameObject* PropFactory::spawnWeaponDrop( GameObjectWorld* world, WeaponId id, const Vector3& position, float yawDegree ) const {
	ASSERT( id >= 0 && id < WEAPON_NUMBER && "PropFactory::spawnWeaponDrop : bad WeaponId." );
	const StaticMeshLods& shapes = mWeaponDropShapes[ id ];
	if ( !shapes.full()->isReady() ) {
		return nullptr;
	}
	GameObject* obj = createProp( world, GameObjectTag::WeaponDrop, kWeaponDropName, shapes, position, yawDegree,
		DrawCulling::Target::Small, BoxColliderComponent::Mode::WalkThrough );
	obj->addComponent< ItemComponent >( weaponSpec( id ).item, kOneWeapon );
	obj->addComponent< PickupComponent >();
	world->spawn( obj );
	return obj;
}

GameObject* PropFactory::spawnDroppedItem( GameObjectWorld* world, const ItemStack& stack, const Vector3& position, float yawDegree ) const {
	// 物の種類ではなく分類で分ける（弾や銃の種類を足しても、ここは変えずに表へ 1 行足すだけで済むように）。
	switch ( itemSpec( stack.item ).kind ) {
	case ITEM_KIND_FUEL:
		ASSERT( stack.amount == kFuelPerCan && "PropFactory::spawnDroppedItem : fuel is one can." );
		return spawnFuel( world, position, yawDegree );
	case ITEM_KIND_AMMO:
		return spawnAmmo( world, stack.item, position, yawDegree, stack.amount );
	case ITEM_KIND_WEAPON:
		ASSERT( stack.amount == kOneWeapon && "PropFactory::spawnDroppedItem : a gun is one." );
		return spawnWeaponDrop( world, weaponForItem( stack.item ), position, yawDegree );
	default:
		ASSERT( false && "PropFactory::spawnDroppedItem : no shape for this item." );
		return nullptr;
	}
}

const StaticMeshLods& PropFactory::ammoShapes( ItemId ammo ) const {
	auto found = mAmmoDropShapes.find( ammo );
	ASSERT( found != mAmmoDropShapes.end() && "PropFactory::ammoShapes : not ammo." );
	return found->second;
}

GameObject* PropFactory::spawnAmmo( GameObjectWorld* world, ItemId ammo, const Vector3& position, float yawDegree, int rounds ) const {
	const StaticMeshLods& shapes = ammoShapes( ammo );
	if ( !shapes.full()->isReady() ) {
		return nullptr;
	}
	GameObject* obj = createProp( world, GameObjectTag::AmmoDrop, kAmmoDropName, shapes, position, yawDegree,
		DrawCulling::Target::Small, BoxColliderComponent::Mode::WalkThrough );
	obj->addComponent< ItemComponent >( ammo, rounds );
	obj->addComponent< PickupComponent >();
	world->spawn( obj );
	return obj;
}
