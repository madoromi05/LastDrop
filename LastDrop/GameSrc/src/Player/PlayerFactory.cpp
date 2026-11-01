#include "GameSrc/Player/PlayerFactory.h"
#include "GameSrc/Render/MeshCache.h"
#include "GameSrc/Component/TransformComponent.h"
#include "GameSrc/Component/SkinnedModelComponent.h"
#include "GameSrc/Component/HitboxComponent.h"
#include "GameSrc/Component/CharacterColliderComponent.h"
#include "GameSrc/Component/LightComponent.h"
#include "GameSrc/Player/PlayerComponent.h"
#include "GameSrc/Player/FirstPersonBodyComponent.h"
#include "GameSrc/Player/HoldComponent.h"
#include "GameSrc/Player/PlayerLocomotionComponent.h"
#include "GameSrc/Player/PlayerHealthComponent.h"
#include "GameSrc/Weapon/GunComponent.h"
#include "GameSrc/Weapon/WeaponModelComponent.h"
#include "GameSrc/Effect/MuzzleFlashComponent.h"
#include "GameSrc/World/GameObjectWorld.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

namespace{
	// ── 自キャラ（Mixamo Ch15。身長約1.8）──
	const char* kPlayerFile = "Resource/player.txt"; // tools/configs/player.json で変換
	const char* kWeaponFile = "Resource/rifle.txt"; // tools/configs/rifle.json で変換（AKM。弾倉を除いた形）
	const char* kMagazineFile = "Resource/rifle_mag.txt"; // tools/configs/rifle_mag.json で変換（AKM の弾倉だけ。銃と同じ座標）
	// 骨の名前の頭の共通部分（部位ごとの当たり判定で骨を引くのに使う）。
	const char* kBoneNamePrefix = "mixamorig:";

	// ── 物の名前（確認用。F6 のエディタの物の一覧・デバッグ出力。ゲームの判断はタグで行う）──
	const char* kPlayerName = "Player";
	const char* kWeaponName = "AK47";
}

PlayerFactory::PlayerFactory( MeshCache* meshes ) :
	mPlayerData( nullptr ),
	mWeaponData( nullptr ),
	mMagazineData( nullptr ) {
	ASSERT( meshes && "PlayerFactory : meshes is null." );
	mPlayerData = meshes->skinnedMesh( kPlayerFile );
	mWeaponData = meshes->staticMesh( kWeaponFile );
	mMagazineData = meshes->staticMesh( kMagazineFile );
}

bool PlayerFactory::isReady() const {
	return mPlayerData->isReady();
}

GameObject* PlayerFactory::spawn( GameObjectWorld* world, const Vector3& position, float yawDegree ) const {
	// HoldComponent が銃を手に持たせ、PlayerComponent が操作を読んで部品へ頼む。付ける順は自由
	// （部品は awake で自分の物を作り、start で隣の部品を借りる。SummaryPlan/個別仕様/ComponentSystem3_Plan.txt）。
	// 外せる部品（Hold・PlayerHealth・Hitbox）は、行を消せばその振る舞いだけ無くなる（素手・死なない・撃たれない）。
	GameObject* weapon = spawnWeapon( world );
	GameObject* obj = world->create( GameObjectTag::Player, kPlayerName );
	obj->addComponent< TransformComponent >( position, yawDegree );
	obj->addComponent< SkinnedModelComponent >( mPlayerData );
	obj->addComponent< FirstPersonBodyComponent >();
	obj->addComponent< HoldComponent >( weapon );
	obj->addComponent< PlayerLocomotionComponent >();
	obj->addComponent< PlayerHealthComponent >();
	obj->addComponent< HitboxComponent >( mPlayerData, kBoneNamePrefix );
	obj->addComponent< CharacterColliderComponent >();
	obj->addComponent< PlayerComponent >();
	world->spawn( obj );
	return obj;
}

GameObject* PlayerFactory::spawnWeapon( GameObjectWorld* world ) const {
	// 構築に失敗していたら置かない（自キャラは素手で出す。キャラまで出さないと操作の確認ができないため）。
	if ( !mWeaponData->isReady() ) {
		return nullptr;
	}
	const StaticMeshData* magazine = mMagazineData->isReady() ? mMagazineData : nullptr; // 失敗したら弾倉の無い銃になるだけ
	GameObject* obj = world->create( GameObjectTag::Weapon, kWeaponName ); // 画像の obj_manager.Register( "AK47" )
	obj->addComponent< WeaponModelComponent >( mWeaponData, magazine );
	obj->addComponent< GunComponent >();
	obj->addComponent< LightComponent >();
	obj->addComponent< MuzzleFlashComponent >();
	world->spawn( obj );
	return obj;
}
