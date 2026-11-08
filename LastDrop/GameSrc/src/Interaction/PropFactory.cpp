#include "GameSrc/Interaction/PropFactory.h"
#include "GameSrc/Render/MeshCache.h"
#include "GameSrc/Component/TransformComponent.h"
#include "GameSrc/Component/StaticModelComponent.h"
#include "GameSrc/Interaction/Item/ItemComponent.h"
#include "GameSrc/Interaction/PickupComponent.h"
#include "GameSrc/Interaction/Loot/LootContainerComponent.h"
#include "GameSrc/Interaction/EscapeCarComponent.h"
#include "GameSrc/World/GameObjectWorld.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

namespace{
	const char* kVehicleFile = "Resource/Model/Prop/uaz.txt"; // tools/configs/uaz_atlas.json → uaz.json で変換（実物大・原点は底面の中心）
	const char* kFuelCanFile = "Resource/Model/Prop/fuelcan.txt"; // tools/configs/fuelcan_atlas.json → fuelcan.json で変換（同上。2026-11-08 に metal_jerrycan へ差し替え）
	const char* kFireHydrantFile = "Resource/Model/Prop/fire_hydrant.txt"; // tools/configs/fire_hydrant_atlas.json → fire_hydrant.json で変換（同上）
	// ゴミ箱・木箱は tools/loot_prop_gen.py で作った FBX を tools/configs/dumpster.json・crate.json で変換（同上。R3_Plan.txt Q2）。
	const char* kDumpsterFile = "Resource/Model/Prop/dumpster.txt";
	const char* kCrateFile = "Resource/Model/Prop/crate.txt";
	// 落ちた物の形のファイルは物の表（ItemCatalog の groundFile）、落ちている銃の形のファイルは銃ごとの表（WeaponCatalog の groundFile）にある。

	// ── 物の名前（確認用。F6 のエディタの物の一覧・デバッグ出力。ゲームの判断はタグで行う）──
	const char* kFuelName = "Fuel";
	const char* kAmmoDropName = "AmmoDrop";
	const char* kItemDropName = "ItemDrop";
	const char* kWeaponDropName = "WeaponDrop";
	const int kOneWeapon = 1; // 落ちている銃は 1 丁
	const char* kEscapeCarName = "EscapeCar";
	const char* kDumpsterName = "Dumpster";
	const char* kCrateName = "Crate";
	const char* kFireHydrantName = "FireHydrant";

	const int kFuelPerCan = 1;
	// 置物の表面の材質（着弾音）。車・缶・弾・ゴミ箱は金属、木箱は木、包帯・売れる物は小さいので金属のまま（ほぼ当たらない）。
	const GroundMaterial kMetalMaterial = GROUND_METAL;
	const GroundMaterial kWoodMaterial = GROUND_WOOD;

	// 箱に必ず入れる物（町のガソリン。入れないなら ITEM_NONE）。
	ItemId guaranteedItem( bool withFuel ) {
		return withFuel ? ITEM_FUEL : ITEM_NONE;
	}
}

PropFactory::PropFactory( MeshCache* meshes ) :
	// 車と缶は粗い形（LOD）も読む（OptimizationC_Plan.txt Q7。弾の箱は小さく、2 画素未満で消えるので段を持たない）。
	mVehicleShapes( StaticMeshLods::request( meshes, kVehicleFile ) ),
	mFuelCanShapes( StaticMeshLods::request( meshes, kFuelCanFile ) ),
	mFireHydrantShapes( StaticMeshLods::request( meshes, kFireHydrantFile ) ),
	mDumpsterShapes( meshes->staticMesh( kDumpsterFile ) ),
	mCrateShapes( meshes->staticMesh( kCrateFile ) ) {
	ASSERT( meshes && "PropFactory : meshes is null." );
	for ( int i = 0; i < ITEM_NUMBER; ++i ) {
		ItemId item = static_cast< ItemId >( i );
		const ItemSpec& spec = itemSpec( item );
		if ( spec.groundFile ) {
			mGroundShapes.emplace( item, StaticMeshLods( meshes->staticMesh( spec.groundFile ) ) );
		}
	}
	for ( int i = 0; i < WEAPON_NUMBER; ++i ) {
		mWeaponDropShapes.emplace_back( meshes->staticMesh( weaponSpec( static_cast< WeaponId >( i ) ).groundFile ) );
	}
}

GameObject* PropFactory::createProp( GameObjectWorld* world, GameObjectTag tag, const char* name, const StaticMeshLods& shapes,
const Vector3& position, float yawDegree, DrawCulling::Target cullTarget, BoxColliderComponent::Mode colliderMode, GroundMaterial material ) const {
	GameObject* obj = world->create( tag, name );
	obj->addComponent< TransformComponent >( position, yawDegree );
	obj->addComponent< StaticModelComponent >( shapes, material, cullTarget );
	obj->addComponent< BoxColliderComponent >( colliderMode ); // 箱は見た目の頂点から求める（見た目は awake で置かれる）
	return obj;
}

GameObject* PropFactory::spawnLootContainer( GameObjectWorld* world, LootTableId table, const Vector3& position, float yawDegree,
Random* random, bool withFuel ) const {
	ASSERT( ( table == LootTableId::Dumpster || table == LootTableId::Crate ) && "PropFactory::spawnLootContainer : not a box." );
	bool isCrate = ( table == LootTableId::Crate );
	const StaticMeshLods& shapes = isCrate ? mCrateShapes : mDumpsterShapes;
	if ( !shapes.full()->isReady() ) {
		return nullptr;
	}
	GameObject* obj = createProp( world, GameObjectTag::LootContainer, isCrate ? kCrateName : kDumpsterName, shapes, position, yawDegree,
		DrawCulling::Target::Large, BoxColliderComponent::Mode::Fixed, isCrate ? kWoodMaterial : kMetalMaterial );
	obj->addComponent< LootContainerComponent >( table, random, guaranteedItem( withFuel ) );
	world->spawn( obj );
	return obj;
}

GameObject* PropFactory::spawnEscapeCar( GameObjectWorld* world, const Vector3& position, float yawDegree, int requiredFuel ) const {
	if ( !mVehicleShapes.full()->isReady() ) {
		return nullptr;
	}
	GameObject* obj = createProp( world, GameObjectTag::EscapeCar, kEscapeCarName, mVehicleShapes, position, yawDegree,
		DrawCulling::Target::Large, BoxColliderComponent::Mode::Fixed, kMetalMaterial );
	obj->addComponent< EscapeCarComponent >( requiredFuel ); // 車全体を見れば給油できる（給油口は作らない）
	world->spawn( obj );
	return obj;
}

GameObject* PropFactory::spawnFireHydrant( GameObjectWorld* world, const Vector3& position, float yawDegree ) const {
	if ( !mFireHydrantShapes.full()->isReady() ) {
		return nullptr;
	}
	// 背丈 0.8m・幅 0.3m ほどの細い物なので、小さすぎれば描かない側（Small）で間引く。
	GameObject* obj = createProp( world, GameObjectTag::FireHydrant, kFireHydrantName, mFireHydrantShapes, position, yawDegree,
		DrawCulling::Target::Small, BoxColliderComponent::Mode::Fixed, kMetalMaterial );
	world->spawn( obj );
	return obj;
}

GameObject* PropFactory::spawnFuel( GameObjectWorld* world, const Vector3& position, float yawDegree ) const {
	if ( !mFuelCanShapes.full()->isReady() ) {
		return nullptr;
	}
	GameObject* obj = createProp( world, GameObjectTag::Fuel, kFuelName, mFuelCanShapes, position, yawDegree,
		DrawCulling::Target::Small, BoxColliderComponent::Mode::Removable, kMetalMaterial );
	obj->addComponent< ItemComponent >( ITEM_FUEL, kFuelPerCan );
	obj->addComponent< PickupComponent >();
	world->spawn( obj );
	return obj;
}

GameObject* PropFactory::spawnWeaponDrop( GameObjectWorld* world, WeaponId id, const Vector3& position, float yawDegree ) const {
	ASSERT( id >= 0 && id < WEAPON_NUMBER && "PropFactory::spawnWeaponDrop : bad WeaponId." );
	const StaticMeshLods& shapes = mWeaponDropShapes[ id ];
	if ( !shapes.full()->isReady() ) {
		return nullptr;
	}
	GameObject* obj = createProp( world, GameObjectTag::WeaponDrop, kWeaponDropName, shapes, position, yawDegree,
		DrawCulling::Target::Small, BoxColliderComponent::Mode::WalkThrough, kMetalMaterial );
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
	case ITEM_KIND_MEDICAL:
	case ITEM_KIND_VALUABLE:
		return spawnGroundItem( world, stack, position, yawDegree );
	case ITEM_KIND_WEAPON:
		ASSERT( stack.amount == kOneWeapon && "PropFactory::spawnDroppedItem : a gun is one." );
		return spawnWeaponDrop( world, weaponForItem( stack.item ), position, yawDegree );
	default:
		ASSERT( false && "PropFactory::spawnDroppedItem : no shape for this item." );
		return nullptr;
	}
}

const StaticMeshLods& PropFactory::groundShapes( ItemId item ) const {
	auto found = mGroundShapes.find( item );
	ASSERT( found != mGroundShapes.end() && "PropFactory::groundShapes : no groundFile." );
	return found->second;
}

GameObject* PropFactory::spawnGroundItem( GameObjectWorld* world, const ItemStack& stack, const Vector3& position, float yawDegree ) const {
	const StaticMeshLods& shapes = groundShapes( stack.item );
	if ( !shapes.full()->isReady() ) {
		return nullptr;
	}
	bool isAmmo = ( itemSpec( stack.item ).kind == ITEM_KIND_AMMO );
	// 足元の小さな物でつまずかないよう、移動の名簿には載せない（上を歩ける）。
	GameObject* obj = createProp( world, isAmmo ? GameObjectTag::AmmoDrop : GameObjectTag::ItemDrop, isAmmo ? kAmmoDropName : kItemDropName,
		shapes, position, yawDegree, DrawCulling::Target::Small, BoxColliderComponent::Mode::WalkThrough, kMetalMaterial );
	obj->addComponent< ItemComponent >( stack.item, stack.amount );
	obj->addComponent< PickupComponent >();
	world->spawn( obj );
	return obj;
}
