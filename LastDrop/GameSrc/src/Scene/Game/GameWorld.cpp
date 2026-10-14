#include "GameSrc/Scene/Game/GameWorld.h"
#include "GameSrc/Render/MeshCache.h"
#include "GameSrc/Player/PlayerCharacter.h"
#include "GameSrc/Map/SpawnPicker.h"

#include "GameLib/GameLib.h"
#include "Framework.h"
#include "GameLib/Math/Vector3.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// 置き場所はどれも TownMap が決める。ここにあるのは、何を置くか（ファイル）と、置き場所以外の決めごとだけ。

	// ── 自キャラ（Mixamo Ch15。身長約1.8）──
	const char* kPlayerFile = "Resource/player.txt"; // tools/configs/player.json で変換
	const char* kWeaponFile = "Resource/rifle.txt"; // tools/configs/rifle.json で変換（AKM）
	// 飛ぶ弾（7.62 NATO の弾頭。実物大・底が原点・先が +Y）。tools/configs/bullet_tip.json で変換
	const char* kBulletFile = "Resource/bullet_tip.txt";

	// ── 背景 ──
	const char* kZombieFile = "Resource/character.txt";
	const char* kZombieAnimation = "walk";
	const float kAnimationStartStep = 11.f; // 体ごとに歩き出しをずらす（フレーム）
	// 建物は tools/configs/building.json の scale で、ドアが人の背丈に合う大きさ（ドアの高さ約 2.0m）に縮めてある。
	const char* kBuildingFile = "Resource/building.txt";
	// 町を囲む建物。同じ建物から足元の歩道の板を除いた形（tools/configs/building_noground.json）。
	// 板があると、壁どうしを付けて並べたときに隣の板と重なって面がちらつくため。
	const char* kPerimeterBuildingFile = "Resource/building_noground.txt";

	// ── ゲームループ（ガソリンを集めて車で脱出）──
	const char* kVehicleFile = "Resource/uaz.txt"; // tools/configs/uaz_atlas.json → uaz.json で変換（実物大・原点は底面の中心）
	const char* kFuelCanFile = "Resource/fuelcan.txt"; // tools/configs/fuelcan_atlas.json → fuelcan.json で変換（同上）
	// 車に入れると走れるようになるガソリンの数。候補の場所（TownMap）から、毎回この数だけ選んで置く（SpawnPicker）。
	const int kRequiredFuel = 3;

	Vector3 toPosition( const TownPlacement& placement ) {
		return Vector3( placement.x, placement.y, placement.z );
	}
}

GameWorld::GameWorld( MeshCache* meshes ) :
	mPlayerData( nullptr ),
	mWeaponData( nullptr ),
	mBulletData( meshes->staticMesh( kBulletFile ) ), // mBullets が借りるので、ここで頼む（他は下でまとめて頼む）
	mZombieData( nullptr ),
	mBuildingData( nullptr ),
	mPerimeterBuildingData( nullptr ),
	mVehicleData( nullptr ),
	mFuelCanData( nullptr ),
	mGround( meshes ),
	mBullets( mBulletData ),
	mZombies( &mCollisionWorld, &mHitWorld ),
	mPickups( &mCollisionWorld, &mHitWorld ) {
	ASSERT( meshes && "GameWorld : meshes is null." );
	// 読み込みを頼むだけ（非同期）。構築が終わったら place で並べる。
	mPlayerData = meshes->skinnedMesh( kPlayerFile );
	mWeaponData = meshes->staticMesh( kWeaponFile );
	mZombieData = meshes->skinnedMesh( kZombieFile );
	mBuildingData = meshes->staticMesh( kBuildingFile );
	mPerimeterBuildingData = meshes->staticMesh( kPerimeterBuildingFile );
	mVehicleData = meshes->staticMesh( kVehicleFile );
	mFuelCanData = meshes->staticMesh( kFuelCanFile );
}

// PlayerCharacter の定義が見える所で破棄する（ヘッダでは前方宣言だけ）。
GameWorld::~GameWorld() {
}

void GameWorld::place() {
	if ( mPlayerData->isReady() ) {
		// 銃の構築に失敗していたら素手で出す（キャラまで出さないと操作の確認ができないため）。
		const StaticMeshData* weapon = mWeaponData->isReady() ? mWeaponData : nullptr;
		const TownPlacement& start = mMap.playerStart();
		mPlayer = std::make_unique< PlayerCharacter >( mPlayerData, weapon, &mCollisionWorld, toPosition( start ), start.yawDegree );
		mPlayer->addTo( &mCollisionWorld, &mHitWorld );
	}
	if ( mZombieData->isReady() ) {
		mZombieModel = std::make_unique< ZombieModel >( mZombieData );
		// 置く（移動・命中の両方の当たり判定への登録は ActorList が行う）。animation が nullptr なら T ポーズ。
		auto addZombie = [ this ]( const TownPlacement& p, const char* animation, float startTime ) {
			mZombies.spawn( std::make_unique< Zombie >( mZombieModel.get(), toPosition( p ), p.yawDegree, animation, startTime ) );
		};
		int zombieIndex = 0;
		for ( const TownPlacement& p : mMap.walkingZombies() ) {
			addZombie( p, kZombieAnimation, kAnimationStartStep * zombieIndex );
			++zombieIndex;
		}
		// 当たり判定の確認用の1体。手足が伸びていて、部位の箱の大きさ・位置を見比べやすい。
		addZombie( mMap.tPoseZombie(), nullptr, 0.f );
	}
	mGround.place( mMap );
	mHitWorld.addBlocker( &mGround ); // 地面も弾・視線を止める（地面全体で1つの物）
	placeBuildings( mBuildingData, mMap.buildings() );
	placeBuildings( mPerimeterBuildingData, mMap.perimeterBuildings() );
	// 弾を止める物として登録する。mBuildings を並べ終えてから行う（push_back の途中で登録すると、
	// 並べ直しで要素の場所が変わり、名簿が古い場所を指してしまうため）。
	for ( const Building& building : mBuildings ) {
		mHitWorld.addBlocker( &building );
	}
	placeMissionObjects();
}

void GameWorld::placeBuildings( const StaticMeshData* data, const std::vector< TownPlacement >& placements ) {
	// 当たり判定を三角形から作るので、構築に失敗した建物は置かない。
	if ( !data->isReady() ) {
		return;
	}
	mBuildings.reserve( mBuildings.size() + placements.size() );
	for ( const TownPlacement& p : placements ) {
		mBuildings.emplace_back( data, toPosition( p ), p.yawDegree );
		mBuildings.back().addCollidersTo( &mCollisionWorld );
	}
}

void GameWorld::placeMissionObjects() {
	// 箱の大きさを頂点から求めるので、構築に失敗したデータは置かない（その場合はクリアできないが、他の確認はできる）。
	if ( mVehicleData->isReady() ) {
		const TownPlacement& p = mMap.escapeCar();
		mEscapeCar = std::make_unique< EscapeCar >( mVehicleData, toPosition( p ), p.yawDegree, kRequiredFuel );
		mEscapeCar->addTo( &mCollisionWorld, &mHitWorld );
	}
	if ( mFuelCanData->isReady() ) {
		const TownPlacement& endless = mMap.endlessFuel();
		mEndlessFuel = std::make_unique< ItemSource >( mFuelCanData, toPosition( endless ), endless.yawDegree, ITEM_FUEL );
		mEndlessFuel->addTo( &mCollisionWorld, &mHitWorld );
		// 遊ぶたびに違う場所に出す（起動からの時間を種にする）。
		unsigned seed = Framework::instance().time();
		const std::vector< TownPlacement >& spawnPoints = mMap.fuelSpawnPoints();
		for ( int index : SpawnPicker::pick( static_cast< int >( spawnPoints.size() ), kRequiredFuel, seed ) ) {
			const TownPlacement& p = spawnPoints[ index ];
			mPickups.spawn( std::make_unique< Pickup >( mFuelCanData, toPosition( p ), p.yawDegree, ITEM_FUEL ) );
		}
	}
}

void GameWorld::update( const PlayerCommand& command, float cameraYaw, float cameraPitch, const Vector3& viewDirection ) {
	if ( mPlayer ) {
		mPlayer->update( command, cameraYaw, cameraPitch );
		// 弾は銃口ではなく目から出す。画面の中央（照準）へ飛び、銃口が壁にめり込んでいても壁の向こうへ撃てないようにするため。
		if ( mPlayer->firedThisTick() ) {
			mBullets.fire( mPlayer->eyePosition(), viewDirection );
		}
	}
	// 撃ったティックのうちに1回進める（出たばかりの弾が、目の前に止まって見えないように）。
	std::vector< HitWorld::Hit > hits;
	mBullets.update( mHitWorld, mPlayer.get(), &hits );
	// 物（建物・車・地面・拾える物）に当たった所にだけ弾痕を付ける。ゾンビ（target）には付けない（2026-10-11 決定）。
	for ( const HitWorld::Hit& hit : hits ) {
		if ( hit.blocker ) {
			mBulletHoles.add( hit.point, hit.normal, hit.blocker );
		}
	}
	mBulletHoles.update();
	for ( auto& zombie : mZombies ) {
		zombie->update();
	}
	// このティックで消す印の付いたものを、全員の更新が済んでから消す（名簿からも外れる）。
	mZombies.flush();
	mPickups.flush();
}

void GameWorld::draw( const ViewFrustum& frustum ) const {
	mGround.draw( frustum );
	if ( mPlayer ) {
		mPlayer->draw();
	}
	mBullets.draw();
	for ( const auto& zombie : mZombies ) {
		zombie->draw();
	}
	for ( const Building& building : mBuildings ) {
		building.draw( frustum );
	}
	if ( mEscapeCar ) {
		mEscapeCar->draw( frustum );
	}
	if ( mEndlessFuel ) {
		mEndlessFuel->draw( frustum );
	}
	for ( const auto& pickup : mPickups ) {
		pickup->draw( frustum );
	}
}

void GameWorld::drawBulletHoles( const Matrix44& projectionView ) {
	mBulletHoles.draw( projectionView );
}

void GameWorld::removeTakenPickups() {
	for ( const auto& pickup : mPickups ) {
		if ( pickup->isTaken() ) {
			mBulletHoles.removeAttachedTo( pickup.get() ); // 物が消える前に外す
			mPickups.destroyLater( pickup.get() );
		}
	}
}

IInteractable* GameWorld::resolveInteractable( const HitWorld::Hit& hit ) {
	if ( !hit.blocker ) {
		return nullptr; // キャラ（ゾンビ）は使えない
	}
	if ( Pickup* pickup = mPickups.find( hit.blocker ) ) {
		return pickup;
	}
	if ( mEndlessFuel && mEndlessFuel->isBody( hit.blocker ) ) {
		return mEndlessFuel.get();
	}
	if ( mEscapeCar && mEscapeCar->isBody( hit.blocker ) ) {
		return mEscapeCar.get();
	}
	return nullptr;
}

const TownMap& GameWorld::map() const {
	return mMap;
}

const TownGround& GameWorld::ground() const {
	return mGround;
}

const CollisionWorld& GameWorld::collisionWorld() const {
	return mCollisionWorld;
}

const HitWorld& GameWorld::hitWorld() const {
	return mHitWorld;
}

PlayerCharacter* GameWorld::player() {
	return mPlayer.get();
}

const PlayerCharacter* GameWorld::player() const {
	return mPlayer.get();
}

const EscapeCar* GameWorld::escapeCar() const {
	return mEscapeCar.get();
}

bool GameWorld::isEscapeReady() const {
	return mEscapeCar && mEscapeCar->isFueled();
}

void GameWorld::appendPickupColliders( std::vector< BoxCollider >* out ) const {
	for ( const auto& pickup : mPickups ) {
		if ( !pickup->isTaken() ) {
			out->push_back( pickup->collider() );
		}
	}
}
