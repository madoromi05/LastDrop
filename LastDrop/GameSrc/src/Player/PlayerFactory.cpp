#include "GameSrc/Player/PlayerFactory.h"
#include "GameSrc/Render/MeshCache.h"
#include "GameSrc/Component/TransformComponent.h"
#include "GameSrc/Component/SkinnedModelComponent.h"
#include "GameSrc/Component/HitboxComponent.h"
#include "GameSrc/Component/CharacterColliderComponent.h"
#include "GameSrc/Component/LightComponent.h"
#include "GameSrc/Player/PlayerComponent.h"
#include "GameSrc/Player/PlayerFootprintComponent.h"
#include "GameSrc/Player/PlayerNoiseComponent.h"
#include "GameSrc/Player/PlayerGrabComponent.h"
#include "GameSrc/Player/Body/FirstPersonBodyComponent.h"
#include "GameSrc/Player/Body/HoldComponent.h"
#include "GameSrc/Player/Body/ArmPoseComponent.h"
#include "GameSrc/Player/PlayerLocomotionComponent.h"
#include "GameSrc/Player/PlayerHealthComponent.h"
#include "GameSrc/Interaction/Item/InventoryComponent.h"
#include "GameSrc/Interaction/InteractorComponent.h"
#include "GameSrc/Weapon/GunComponent.h"
#include "GameSrc/Weapon/WeaponModelComponent.h"
#include "GameSrc/Effect/MuzzleFlashComponent.h"
#include "GameSrc/World/GameObjectWorld.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

namespace{
	// ── 自キャラ
	const char* kPlayerFile = "Resource/Model/Character/player.txt";
	// 銃の形のファイルは銃ごとの表（WeaponCatalog）にある
	// 骨の名前の頭の共通部分（部位ごとの当たり判定で骨を引くのに使う）
	const char* kBoneNamePrefix = "mixamorig:";
	const float kInteractReach = 2.5f; // 見ている物を使うときに手が届く距離（m）。足元の缶を立ったまま拾える長さ

	// ── 物の名前（確認用。F6 のエディタの物の一覧・デバッグ出力。ゲームの判断はタグで行う）──
	const char* kPlayerName = "Player";
}

PlayerFactory::PlayerFactory( MeshCache* meshes ) :
	mPlayerData( nullptr ) {
	ASSERT( meshes && "PlayerFactory : meshes is null." );
	mPlayerData = meshes->skinnedMesh( kPlayerFile );
	for ( int i = 0; i < WEAPON_NUMBER; ++i ) {
		const WeaponSpec& spec = weaponSpec( static_cast< WeaponId >( i ) );
		mWeaponData[ i ] = meshes->staticMesh( spec.modelFile );
		mMagazineData[ i ] = meshes->staticMesh( spec.magazineFile );
	}
}

bool PlayerFactory::isReady() const {
	return mPlayerData->isReady();
}

GameObject* PlayerFactory::spawn( GameObjectWorld* world, const Vector3& position, float yawDegree, const ItemGrid& startBag ) const {
	// 外せる部品（Hold・ArmPose・PlayerHealth・Hitbox・PlayerGrab・PlayerNoise・Interactor）は、行を消せばその振る舞いだけ無くなる
	// Input の段で動く 2 つ（PlayerComponent → PlayerNoise）は、この順に付ける（同じ段の中は付けた順に動く
	// 音は、動いた後の足元・撃った後の銃を読むため）
	std::vector< const GameObject* > weapons;
	for ( int i = 0; i < WEAPON_NUMBER; ++i ) {
		weapons.push_back( spawnWeapon( world, static_cast< WeaponId >( i ) ) ); // 作れなかった銃（nullptr）は HoldComponent が飛ばす
	}
	GameObject* obj = world->create( GameObjectTag::Player, kPlayerName );
	obj->addComponent< TransformComponent >( position, yawDegree );
	obj->addComponent< SkinnedModelComponent >( mPlayerData );
	obj->addComponent< FirstPersonBodyComponent >();
	obj->addComponent< HoldComponent >( weapons );
	obj->addComponent< ArmPoseComponent >();
	obj->addComponent< PlayerLocomotionComponent >();
	obj->addComponent< PlayerHealthComponent >();
	obj->addComponent< InventoryComponent >( startBag ); // 持ち物
	obj->addComponent< InteractorComponent >( kInteractReach ); // 見ている物を E で使う
	obj->addComponent< HitboxComponent >( mPlayerData, kBoneNamePrefix );
	obj->addComponent< CharacterColliderComponent >();
	obj->addComponent< PlayerFootprintComponent >(); // 移動の当たり判定の形
	obj->addComponent< PlayerGrabComponent >();
	obj->addComponent< PlayerComponent >();
	obj->addComponent< PlayerNoiseComponent >(); 
	world->spawn( obj );
	return obj;
}

GameObject* PlayerFactory::spawnWeapon( GameObjectWorld* world, WeaponId id ) const {
	const WeaponSpec& spec = weaponSpec( id );
	// 構築に失敗していたら置かない（自キャラは素手で出す。キャラまで出さないと操作の確認ができないため）
	if ( !mWeaponData[ id ]->isReady() ) {
		return nullptr;
	}
	const StaticMeshData* magazine = mMagazineData[ id ]->isReady() ? mMagazineData[ id ] : nullptr; // 失敗したら弾倉の無い銃になるだけ
	GameObject* obj = world->create( GameObjectTag::Weapon, spec.name ); // 画像の obj_manager.Register( "AK47" )
	obj->addComponent< WeaponModelComponent >( mWeaponData[ id ], magazine, spec.grip );
	obj->addComponent< GunComponent >( spec );
	obj->addComponent< LightComponent >( LightPreset::Flashlight );
	obj->addComponent< MuzzleFlashComponent >();
	world->spawn( obj );
	return obj;
}
