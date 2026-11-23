#include "GameSrc/Interaction/PropFactory.h"
#include "GameSrc/Render/MeshCache.h"
#include "GameSrc/Component/TransformComponent.h"
#include "GameSrc/Component/StaticModelComponent.h"
#include "GameSrc/Component/LightComponent.h"
#include "GameSrc/Interaction/Item/ItemComponent.h"
#include "GameSrc/Interaction/PickupComponent.h"
#include "GameSrc/Interaction/Loot/LootContainerComponent.h"
#include "GameSrc/Interaction/EscapeCarComponent.h"
#include "GameSrc/Weapon/GunComponent.h"
#include "GameSrc/World/GameObjectWorld.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

namespace{
	const char* kVehicleFile = "Resource/Model/Prop/uaz.txt"; // tools/configs/uaz_atlas.json → uaz.json で変換（実物大・原点は底面の中心）
	const char* kFuelCanFile = "Resource/Model/Prop/fuelcan.txt"; // tools/configs/fuelcan_atlas.json → fuelcan.json で変換（同上）
	const char* kFireHydrantFile = "Resource/Model/Prop/fire_hydrant.txt"; // tools/configs/fire_hydrant_atlas.json → fire_hydrant.json で変換（同上）
	// 街灯は tools/configs/streetlamp.json・streetlamp_off.json で変換（同上。画像は使わず色だけ。ガラスは別の部品で発光だけ違う）
	const char* kStreetLampFile = "Resource/Model/Prop/streetlamp.txt";
	const char* kStreetLampOffFile = "Resource/Model/Prop/streetlamp_off.txt";
	// ゴミ箱・木箱は tools/modelgen/loot_prop_gen.py で作った FBX を tools/configs/dumpster.json・crate.json で変換（同上）
	const char* kDumpsterFile = "Resource/Model/Prop/dumpster.txt";
	const char* kCrateFile = "Resource/Model/Prop/crate.txt";
	// 落ちた物の形のファイルは物の表（ItemCatalog の groundFile）、落ちている銃の形のファイルは銃ごとの表（WeaponCatalog の groundFile）にある

	// ── 物の名前（確認用。F6 のエディタの物の一覧・デバッグ出力。ゲームの判断はタグで行う）──
	const char* kFuelName = "Fuel";
	const char* kAmmoDropName = "AmmoDrop";
	const char* kItemDropName = "ItemDrop";
	const int kOneWeapon = 1; // 地面に置いた銃は 1 丁
	const char* kEscapeCarName = "EscapeCar";
	const char* kDumpsterName = "Dumpster";
	const char* kCrateName = "Crate";
	const char* kFireHydrantName = "FireHydrant";
	const char* kStreetLampName = "StreetLamp";
	const char* kBuildingPreviewName = "BuildingPreview";

	const int kFuelPerCan = 1;
	// 置物の表面の材質（着弾音）。車・缶・弾・ゴミ箱は金属、木箱は木、包帯は小さいので金属のまま（ほぼ当たらない）
	const GroundMaterial kMetalMaterial = GROUND_METAL;
	const GroundMaterial kWoodMaterial = GROUND_WOOD;
}

PropFactory::PropFactory( MeshCache* meshes ) :
	// 車と缶は粗い形（LOD）も読む（弾の箱は小さく、2 画素未満で消えるので段を持たない）
	mVehicleShapes( StaticMeshLods::request( meshes, kVehicleFile ) ),
	mFuelCanShapes( StaticMeshLods::request( meshes, kFuelCanFile ) ),
	mFireHydrantShapes( StaticMeshLods::request( meshes, kFireHydrantFile ) ),
	mStreetLampShapes( StaticMeshLods::request( meshes, kStreetLampFile ) ),
	mStreetLampOffShapes( StaticMeshLods::request( meshes, kStreetLampOffFile ) ),
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
		mWeaponGroundShapes.emplace_back( meshes->staticMesh( weaponSpec( static_cast< WeaponId >( i ) ).groundFile ) );
	}
}

GameObject* PropFactory::createProp( GameObjectWorld* world, GameObjectTag tag, const char* name, const StaticMeshLods& shapes,
const Vector3& position, float yawDegree, DrawCulling::Target cullTarget, BoxColliderComponent::Mode colliderMode, GroundMaterial material ) const {
	GameObject* obj = world->create( tag, name );
	addPropParts( obj, shapes, position, yawDegree, cullTarget, colliderMode, material );
	return obj;
}

void PropFactory::addPropParts( GameObject* obj, const StaticMeshLods& shapes, const Vector3& position, float yawDegree,
DrawCulling::Target cullTarget, BoxColliderComponent::Mode colliderMode, GroundMaterial material ) const {
	obj->addComponent< TransformComponent >( position, yawDegree );
	obj->addComponent< StaticModelComponent >( shapes, material, cullTarget );
	obj->addComponent< BoxColliderComponent >( colliderMode ); // 箱は見た目の頂点から求める（見た目は awake で置かれる）
}

GameObject* PropFactory::spawnLootContainer( GameObjectWorld* world, LootTableId table, const Vector3& position, float yawDegree,
Random* random, const std::vector< ItemStack >& guaranteed ) const {
	ASSERT( ( table == LootTableId::Dumpster || table == LootTableId::Crate ) && "PropFactory::spawnLootContainer : not a box." );
	bool isCrate = ( table == LootTableId::Crate );
	const StaticMeshLods& shapes = isCrate ? mCrateShapes : mDumpsterShapes;
	if ( !shapes.full()->isReady() ) {
		return nullptr;
	}
	GameObject* obj = createProp( world, GameObjectTag::LootContainer, isCrate ? kCrateName : kDumpsterName, shapes, position, yawDegree,
		DrawCulling::Target::Large, BoxColliderComponent::Mode::Fixed, isCrate ? kWoodMaterial : kMetalMaterial );
	obj->addComponent< LootContainerComponent >( table, random, guaranteed );
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
	// 背丈 0.8m・幅 0.3m ほどの細い物なので、小さすぎれば描かない側（Small）で間引く
	GameObject* obj = createProp( world, GameObjectTag::FireHydrant, kFireHydrantName, mFireHydrantShapes, position, yawDegree,
		DrawCulling::Target::Small, BoxColliderComponent::Mode::Fixed, kMetalMaterial );
	world->spawn( obj );
	return obj;
}

GameObject* PropFactory::spawnStreetLamp( GameObjectWorld* world, const Vector3& position, float yawDegree, bool lit ) const {
	const StaticMeshLods& shapes = streetLampShapes( lit );
	if ( !shapes.full()->isReady() ) {
		return nullptr;
	}
	// 背丈 3.9m あり遠くからも見える（光るガラスが目印になる）ので、大きい物の側（Large）で間引く
	GameObject* obj = createProp( world, GameObjectTag::StreetLamp, kStreetLampName, shapes, position, yawDegree,
		DrawCulling::Target::Large, BoxColliderComponent::Mode::Fixed, kMetalMaterial );
	obj->addComponent< LightComponent >( LightPreset::StreetLamp ); // 柱の頭から照らす。停電の日は、ライトの一覧を作る側が明るさを 0 にする
	world->spawn( obj );
	return obj;
}

void PropFactory::relightStreetLamps( GameObjectWorld* world, bool lit ) const {
	ASSERT( world && "PropFactory::relightStreetLamps : world is null." );
	const StaticMeshLods& shapes = streetLampShapes( lit );
	if ( !shapes.full()->isReady() ) {
		return;
	}
	// 光る形と消えた形は同じ頂点（ガラスの発光だけ違う）なので、描く形だけを替える
	world->forEach< StaticModelComponent >( [ &shapes ]( StaticModelComponent& model ) {
		if ( model.gameObject()->tag() == GameObjectTag::StreetLamp ) {
			model.setShapes( shapes );
		}
	} );
}

const StaticMeshLods& PropFactory::streetLampShapes( bool lit ) const {
	return lit ? mStreetLampShapes : mStreetLampOffShapes;
}

GameObject* PropFactory::spawnFuel( GameObjectWorld* world, const Vector3& position, float yawDegree ) const {
	if ( !mFuelCanShapes.full()->isReady() ) {
		return nullptr;
	}
	GameObject* obj = createProp( world, GameObjectTag::Fuel, kFuelName, mFuelCanShapes, position, yawDegree,
		DrawCulling::Target::Small, BoxColliderComponent::Mode::Removable, kMetalMaterial );
	obj->addComponent< ItemComponent >( ITEM_FUEL, kFuelPerCan );
	obj->addComponent< PickupComponent >( PickupComponent::AfterTaken::DestroyObject );
	world->spawn( obj );
	return obj;
}

GameObject* PropFactory::layWeaponOnGround( GameObjectWorld* world, WeaponId id, const Vector3& position, float yawDegree ) const {
	ASSERT( world && id >= 0 && id < WEAPON_NUMBER && "PropFactory::layWeaponOnGround : bad argument." );
	const StaticMeshLods& shapes = mWeaponGroundShapes[ id ];
	if ( !shapes.full()->isReady() ) {
		return nullptr;
	}
	GunComponent* gun = GunComponent::find( *world, weaponSpec( id ).item );
	if ( !gun ) {
		return nullptr; // 手に持つ形の構築に失敗して、銃の物が作られていない
	}
	GameObject* weapon = gun->gameObject();
	if ( weapon->getComponent< TransformComponent >() ) {
		return nullptr; // もう地面にある（外す印が付いて、まだ外れていない間も）
	}
	// 捨てた銃は新しい物を作らず、銃の物へ地面に置くための部品を足す（拾われたら liftFromGround で外す）
	// 足元の小さな物でつまずかないよう、移動の名簿には載せない（上を歩ける）
	addPropParts( weapon, shapes, position, yawDegree, DrawCulling::Target::Small, BoxColliderComponent::Mode::WalkThrough, kMetalMaterial );
	weapon->addComponent< ItemComponent >( weaponSpec( id ).item, kOneWeapon );
	weapon->addComponent< PickupComponent >( PickupComponent::AfterTaken::LeaveObject ); // ItemComponent と見た目を start で借りるので最後に付ける
	return weapon;
}

void PropFactory::liftFromGround( GameObject* weapon ) {
	ASSERT( weapon && "PropFactory::liftFromGround : weapon is null." );
	// 5 つは互いに借り合っているので、全部に印を付ける（同じ flush で一緒に外れる。片方だけでは外せない）
	weapon->removeComponentLater< PickupComponent >();
	weapon->removeComponentLater< ItemComponent >();
	weapon->removeComponentLater< BoxColliderComponent >();
	weapon->removeComponentLater< StaticModelComponent >();
	weapon->removeComponentLater< TransformComponent >();
}

GameObject* PropFactory::spawnDroppedItem( GameObjectWorld* world, const ItemStack& stack, const Vector3& position, float yawDegree ) const {
	// 物の種類ではなく分類で分ける（弾や銃の種類を足しても、ここは変えずに表へ 1 行足すだけで済むように）
	switch ( itemSpec( stack.item ).kind ) {
	case ITEM_KIND_FUEL:
		ASSERT( stack.amount == kFuelPerCan && "PropFactory::spawnDroppedItem : fuel is one can." );
		return spawnFuel( world, position, yawDegree );
	case ITEM_KIND_AMMO:
	case ITEM_KIND_MEDICAL:
		return spawnGroundItem( world, stack, position, yawDegree );
	case ITEM_KIND_WEAPON:
		ASSERT( stack.amount == kOneWeapon && "PropFactory::spawnDroppedItem : a gun is one." );
		return layWeaponOnGround( world, weaponForItem( stack.item ), position, yawDegree );
	default:
		ASSERT( false && "PropFactory::spawnDroppedItem : no shape for this item." );
		return nullptr;
	}
}

GameObject* PropFactory::spawnEditPreview( GameObjectWorld* world, TownEditKind kind, const Vector3& position, float yawDegree, bool lampLit ) const {
	ASSERT( world && "PropFactory::spawnEditPreview : world is null." );
	// 形・間引き方・材質は、町に置くときの spawn と同じ
	struct Look{
		GameObjectTag tag;
		const char* name;
		const StaticMeshLods* shapes;
		DrawCulling::Target cullTarget;
		GroundMaterial material;
	};
	Look look;
	switch ( kind ) {
		case TownEditKind::Dumpster:	look = { GameObjectTag::LootContainer, kDumpsterName, &mDumpsterShapes, DrawCulling::Target::Large, kMetalMaterial }; break;
		case TownEditKind::Crate:		look = { GameObjectTag::LootContainer, kCrateName, &mCrateShapes, DrawCulling::Target::Large, kWoodMaterial }; break;
		case TownEditKind::FireHydrant:	look = { GameObjectTag::FireHydrant, kFireHydrantName, &mFireHydrantShapes, DrawCulling::Target::Small, kMetalMaterial }; break;
		case TownEditKind::StreetLight:	look = { GameObjectTag::StreetLamp, kStreetLampName, &streetLampShapes( lampLit ), DrawCulling::Target::Large, kMetalMaterial }; break;
		default:						return nullptr;
	}
	if ( !look.shapes->full()->isReady() ) {
		return nullptr;
	}
	GameObject* obj = createProp( world, look.tag, look.name, *look.shapes, position, yawDegree, look.cullTarget,
		BoxColliderComponent::Mode::WalkThrough, look.material );
	if ( kind == TownEditKind::StreetLight ) {
		obj->addComponent< LightComponent >( LightPreset::StreetLamp );
	}
	world->spawn( obj );
	return obj;
}

GameObject* PropFactory::spawnBuildingPreview( GameObjectWorld* world, const StaticMeshLods& shapes, GroundMaterial material,
const Vector3& position, float yawDegree ) const {
	ASSERT( world && "PropFactory::spawnBuildingPreview : world is null." );
	if ( !shapes.full()->isReady() ) {
		return nullptr;
	}
	GameObject* obj = createProp( world, GameObjectTag::BuildingPreview, kBuildingPreviewName, shapes, position, yawDegree,
		DrawCulling::Target::Large, BoxColliderComponent::Mode::WalkThrough, material );	// 間引き方は町の建物と同じ
	world->spawn( obj );
	return obj;
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
	// 足元の小さな物でつまずかないよう、移動の名簿には載せない（上を歩ける）
	GameObject* obj = createProp( world, isAmmo ? GameObjectTag::AmmoDrop : GameObjectTag::ItemDrop, isAmmo ? kAmmoDropName : kItemDropName,
		shapes, position, yawDegree, DrawCulling::Target::Small, BoxColliderComponent::Mode::WalkThrough, kMetalMaterial );
	obj->addComponent< ItemComponent >( stack.item, stack.amount );
	obj->addComponent< PickupComponent >( PickupComponent::AfterTaken::DestroyObject );
	world->spawn( obj );
	return obj;
}
