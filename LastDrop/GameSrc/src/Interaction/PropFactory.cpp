#include "GameSrc/Interaction/PropFactory.h"
#include "GameSrc/Render/MeshCache.h"
#include "GameSrc/Component/TransformComponent.h"
#include "GameSrc/Component/StaticModelComponent.h"
#include "GameSrc/Interaction/ItemComponent.h"
#include "GameSrc/Interaction/PickupComponent.h"
#include "GameSrc/Interaction/ItemSourceComponent.h"
#include "GameSrc/Interaction/FuelTankComponent.h"
#include "GameSrc/World/GameObjectWorld.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

namespace{
	const char* kVehicleFile = "Resource/uaz.txt"; // tools/configs/uaz_atlas.json → uaz.json で変換（実物大・原点は底面の中心）
	const char* kFuelCanFile = "Resource/fuelcan.txt"; // tools/configs/fuelcan_atlas.json → fuelcan.json で変換（同上）
	const char* kAmmoDropFile = "Resource/ammo_drop.txt";

	// ── 物の名前（確認用。F6 のエディタの物の一覧・デバッグ出力。ゲームの判断はタグで行う）──
	const char* kFuelName = "Fuel";
	const char* kAmmoDropName = "AmmoDrop";
	const char* kEscapeCarName = "EscapeCar";
	const char* kFuelStationName = "FuelStation";

	const int kFuelPerCan = 1;
	const int kAmmoDropRounds = 10;		//落とす球数
	// 置物（車・ガソリン缶・落ちた弾）の表面の材質（着弾音）。どれも金属。木箱などを置くときは、物ごとに変える。
	const GroundMaterial kPropMaterial = GROUND_METAL;
}

PropFactory::PropFactory( MeshCache* meshes ) :
	// 車と缶は粗い形（LOD）も読む（OptimizationC_Plan.txt Q7。弾の箱は小さく、2 画素未満で消えるので段を持たない）。
	mVehicleShapes( StaticMeshLods::request( meshes, kVehicleFile ) ),
	mFuelCanShapes( StaticMeshLods::request( meshes, kFuelCanFile ) ),
	mAmmoDropShapes( meshes->staticMesh( kAmmoDropFile ) ) {
	ASSERT( meshes && "PropFactory : meshes is null." );
}

GameObject* PropFactory::createProp( GameObjectWorld* world, GameObjectTag tag, const char* name, const StaticMeshLods& shapes,
const Vector3& position, float yawDegree, DrawCulling::Target cullTarget, BoxColliderComponent::Mode colliderMode ) const {
	GameObject* obj = world->create( tag, name );
	obj->addComponent< TransformComponent >( position, yawDegree );
	obj->addComponent< StaticModelComponent >( shapes, kPropMaterial, cullTarget );
	obj->addComponent< BoxColliderComponent >( colliderMode ); // 箱は見た目の頂点から求める（見た目は awake で置かれる）
	return obj;
}

GameObject* PropFactory::spawnEscapeCar( GameObjectWorld* world, const Vector3& position, float yawDegree, int requiredFuel ) const {
	// 箱の大きさを頂点から求めるので、構築に失敗したデータは置かない。
	if ( !mVehicleShapes.full()->isReady() ) {
		return nullptr;
	}
	GameObject* obj = createProp( world, GameObjectTag::EscapeCar, kEscapeCarName, mVehicleShapes, position, yawDegree,
		DrawCulling::Target::Large, BoxColliderComponent::Mode::Fixed );
	obj->addComponent< FuelTankComponent >( requiredFuel );
	world->spawn( obj );
	return obj;
}

GameObject* PropFactory::spawnFuelStation( GameObjectWorld* world, const Vector3& position, float yawDegree ) const {
	if ( !mFuelCanShapes.full()->isReady() ) {
		return nullptr;
	}
	GameObject* obj = createProp( world, GameObjectTag::FuelStation, kFuelStationName, mFuelCanShapes, position, yawDegree,
		DrawCulling::Target::Small, BoxColliderComponent::Mode::Fixed );
	obj->addComponent< ItemComponent >( ITEM_FUEL, kFuelPerCan );
	obj->addComponent< ItemSourceComponent >();
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

GameObject* PropFactory::spawnAmmoDrop( GameObjectWorld* world, const Vector3& position, float yawDegree ) const {
	if ( !mAmmoDropShapes.full()->isReady() ) {
		return nullptr;
	}
	GameObject* obj = createProp( world, GameObjectTag::AmmoDrop, kAmmoDropName, mAmmoDropShapes, position, yawDegree,
		DrawCulling::Target::Small, BoxColliderComponent::Mode::WalkThrough );
	obj->addComponent< ItemComponent >( ITEM_AMMO, kAmmoDropRounds );
	obj->addComponent< PickupComponent >();
	world->spawn( obj );
	return obj;
}
