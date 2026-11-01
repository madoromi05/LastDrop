#include "GameSrc/Scene/Game/GameWorld.h"
#include "GameSrc/Render/MeshCache.h"
#include "GameSrc/Map/SpawnPicker.h"
#include "GameSrc/Hit/DamageInfo.h"
#include "GameSrc/Component/TransformComponent.h"
#include "GameSrc/Component/SkinnedModelComponent.h"
#include "GameSrc/Component/StaticModelComponent.h"
#include "GameSrc/Component/HitboxComponent.h"
#include "GameSrc/Component/CharacterColliderComponent.h"
#include "GameSrc/Player/PlayerComponent.h"
#include "GameSrc/Player/FirstPersonBodyComponent.h"
#include "GameSrc/Player/HoldComponent.h"
#include "GameSrc/Player/PlayerHealthComponent.h"
#include "GameSrc/Weapon/GunComponent.h"
#include "GameSrc/Effect/MuzzleFlashComponent.h"
#include "GameSrc/Interaction/ItemComponent.h"
#include "GameSrc/Interaction/PickupComponent.h"
#include "GameSrc/Interaction/ItemSourceComponent.h"
#include "GameSrc/Interaction/FuelTankComponent.h"
#include "GameSrc/Zombie/ZombieAttack.h"
#include "GameSrc/Zombie/ZombieStateComponent.h"
#include "GameSrc/Zombie/ZombieAttackComponent.h"
#include "GameSrc/Zombie/ZombieHealthComponent.h"
#include "GameSrc/Zombie/ZombieCorpseComponent.h"
#include "GameSrc/Zombie/ZombieDismemberComponent.h"

#include "GameLib/GameLib.h"
#include "Framework.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Random.h"
#include "GameLib/Profiling/Profiler.h"

#include <algorithm>
#include <iterator>

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// 置き場所はどれも TownMap が決める。ここにあるのは、何を置くか（ファイル）と、置き場所以外の決めごとだけ。

	// 飛ぶ弾（7.62 NATO の弾頭。実物大・底が原点・先が +Y）。tools/configs/bullet_tip.json で変換
	const char* kBulletFile = "Resource/bullet_tip.txt";

	// ── 物の名前（確認用。F6 のエディタの物の一覧・デバッグ出力。ゲームの判断はタグで行う）──
	const char* kFuelName = "Fuel";
	const char* kAmmoDropName = "AmmoDrop";
	const char* kEscapeCarName = "EscapeCar";
	const char* kFuelStationName = "FuelStation";

	// ── 背景 ──
	const char* kZombieFile = "Resource/character.txt";
	const char* kZombieAnimation = "walk";
	const float kAnimationStartStep = 11.f; // 体ごとに歩き出しをずらす（フレーム）
	// 同時に居られるゾンビの数（死体も数える）。これより多くは出さない（追ってくる AI と見通しの線・描画が体の数だけ重くなる）。
	const int kMaxZombies = 30;
	// 倒れるときのアニメ（tools/configs/zombie.json の extra_animations）。体ごとにどれかをランダムで選ぶ（倒れ方が揃わないように）。
	const char* const kZombieDeathAnimations[] = { "death1", "death2" };
	const int kZombieDeathAnimationNumber = sizeof( kZombieDeathAnimations ) / sizeof( kZombieDeathAnimations[ 0 ] );
	// 脚を失って倒れるときのアニメ。前へ倒れてうつ伏せになる方（death2 は仰向けに倒れるので、体を引きずれない。
	// SummaryPlan/個別仕様/ZombieCrawl_Plan.txt の 2 章）。
	const char* kZombieDownedAnimation = "death1";
	// 銃声が届く距離（m。撃った所から水平だけ。SummaryPlan/個別仕様/ZombieHearing_Plan.txt）。隣の通り（33m 先）まで届く。
	// 建物越しでも縮めない（音は回り込むので）。
	const float kGunshotHearingRange = 40.f;
	// 撃たれてよろけるときのアニメ（同じく extra_animations）。
	const char* kZombieStaggerAnimation = "gethit";
	// 気付く前・追いついた・見失って探し終えたときに立つアニメ（同じく extra_animations）。
	const char* kZombieIdleAnimation = "idle1";
	// 追いついたら、腕で引っかく攻撃のアニメ（同じく extra_animations。右手を振り上げて振り下ろす）。
	const char* kZombieAttackAnimation = "atack1";
	// 血しぶきの量と大きさの倍率。頭は多く大きく飛ばす（当たった手応えを一番強くする部位）。
	const float kBodyBloodScale = 1.f;
	const float kHeadBloodScale = 1.5f;
	// 部位が千切れたときに切り口から出す血しぶきの倍率。当たったときより大きく、一番多く出す（BloodSpray::kMaxDropNumber まで）。
	const float kSeverBloodScale = 2.f;
	// 建物は tools/configs/building.json の scale で、ドアが人の背丈に合う大きさ（ドアの高さ約 2.0m）に縮めてある。
	const char* kBuildingFile = "Resource/building.txt";
	// 町を囲む建物。同じ建物から足元の歩道の板を除いた形（tools/configs/building_noground.json）。
	// 板があると、壁どうしを付けて並べたときに隣の板と重なって面がちらつくため。
	const char* kPerimeterBuildingFile = "Resource/building_noground.txt";
	// 建物の壁の材質（着弾音）。どちらもレンガの建物なので、コンクリートの音で鳴らす。
	const GroundMaterial kBuildingMaterial = GROUND_CONCRETE;

	// ── ゲームループ（ガソリンを集めて車で脱出）──
	const char* kVehicleFile = "Resource/uaz.txt"; // tools/configs/uaz_atlas.json → uaz.json で変換（実物大・原点は底面の中心）
	const char* kFuelCanFile = "Resource/fuelcan.txt"; // tools/configs/fuelcan_atlas.json → fuelcan.json で変換（同上）
	// 車に入れると走れるようになるガソリンの数。候補の場所（TownMap）から、毎回この数だけ選んで置く（SpawnPicker）。
	const int kRequiredFuel = 3;
	const int kFuelPerCan = 1;
	// 置物（車・ガソリン缶・落ちた弾）の表面の材質（着弾音）。どれも金属。木箱などを置くときは、物ごとに変える。
	const GroundMaterial kPropMaterial = GROUND_METAL;

	// ── ゾンビが落とす予備の弾（2026-10-22 ユーザー指定：40% で 10 発）──
	// 1 体あたり平均 4 発。倒すのに 3〜6 発使うので、無駄撃ちすると減っていく（バイオハザードの乏しさ）。
	const char* kAmmoDropFile = "Resource/ammo_drop.txt"; // tools/configs/ammo_drop.json で変換（7.62 NATO の弾薬を 3 倍にして寝かせた物）
	const float kAmmoDropChance = 0.4f;
	const int kAmmoDropRounds = 10;
	// 同時に落ちていられる数。超えたら一番古い物を消す（拾わずに倒し続けても名簿が増え続けないように）。
	const int kMaxAmmoDrops = 10;
	// 足元から自キャラの方へ寄せる長さ（m）。死体の脚の下に隠れにくくする（隠れても、死体を見れば拾える）。
	const float kAmmoDropTowardPlayer = 0.5f;
	const float kFullTurnDegree = 360.f; // 落ちた弾の向きは体ごとにばらばら

	Vector3 toPosition( const TownPlacement& placement ) {
		return Vector3( placement.x, placement.y, placement.z );
	}

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

GameWorld::GameWorld( MeshCache* meshes ) :
	mPlayerFactory( meshes ),
	mBulletData( meshes->staticMesh( kBulletFile ) ), // mBullets が借りるので、ここで頼む（他は下でまとめて頼む）
	mZombieData( nullptr ),
	mBuildingData( nullptr ),
	mPerimeterBuildingData( nullptr ),
	mVehicleData( nullptr ),
	mFuelCanData( nullptr ),
	mAmmoDropData( nullptr ),
	mGround( meshes ),
	mBullets( mBulletData ),
	mObjects( &mCollisionWorld, &mHitWorld ),
	mPlayer( nullptr ),
	mPlayerControl( nullptr ),
	mFuelTank( nullptr ),
	mZombieNumber( 0 ) {
	ASSERT( meshes && "GameWorld : meshes is null." );
	// 読み込みを頼むだけ（非同期）。構築が終わったら place で並べる。
	mZombieData = meshes->skinnedMesh( kZombieFile );
	mBuildingData = meshes->staticMesh( kBuildingFile );
	mPerimeterBuildingData = meshes->staticMesh( kPerimeterBuildingFile );
	mVehicleData = meshes->staticMesh( kVehicleFile );
	mFuelCanData = meshes->staticMesh( kFuelCanFile );
	mAmmoDropData = meshes->staticMesh( kAmmoDropFile );
	mZombieContext.hitWorld = &mHitWorld;
	mZombieContext.crowd = &mCrowd;
	// 世界に 1 つの物を共有の棚へ置く（部品が付けるときの引数ではなく、ここから引く。SummaryPlan/個別仕様/ComponentSystem3_Plan.txt の B）。
	// 床の形は place で作るが、置くのは借りる場所だけなので先に置いてよい（ゾンビが道を聞くのは作った後）。
	mObjects.provide( &mNavMesh );
	mObjects.provide( &mZombieContext );
}

// ZombieFactory などの定義が見える所で破棄する。
GameWorld::~GameWorld() {
}

void GameWorld::place() {
	if ( mPlayerFactory.isReady() ) {
		placePlayer();
	}
	if ( mZombieData->isReady() ) {
		// 根の動きを置き場所へ移すのは、1回だけ流すアニメ（攻撃・よろけ・倒れる。SummaryPlan/個別仕様/ZombieRootMotion_Plan.txt）。
		std::vector< const char* > rootMotionAnimations( std::begin( kZombieDeathAnimations ), std::end( kZombieDeathAnimations ) );
		rootMotionAnimations.push_back( kZombieStaggerAnimation );
		rootMotionAnimations.push_back( kZombieAttackAnimation );
		mZombieModel = std::make_unique< ZombieModel >( mZombieData, kZombieAnimation, rootMotionAnimations );
		mZombieFactory = std::make_unique< ZombieFactory >( mZombieModel.get() );
		// 遊ぶたびに違う倒れ方にする（起動からの時間を種にする。ガソリンの場所と同じ）。
		mZombieRandom = Random::create( static_cast< int >( Framework::instance().time() ) );
		// 歩くゾンビは、最初の update から ZombieSpawner が見えない所へ湧かせる（自キャラの目とカメラの向きが分かってから選ぶため）。
		std::vector< Vector3 > spawnPoints;
		for ( const TownPlacement& p : mMap.zombieSpawnPoints() ) {
			spawnPoints.push_back( toPosition( p ) );
		}
		mZombieSpawner = std::make_unique< ZombieSpawner >( spawnPoints, &mHitWorld, &mCollisionWorld,
			&mZombieModel->footprint(), mZombieRandom.getInt() );
		// 当たり判定の確認用の1体。手足が伸びていて、部位の箱の大きさ・位置を見比べやすい。
		// アニメを付けない（T ポーズ）。追ってこず、倒れず（体力は減って 0 で止まる）、よろけない（のけぞりだけ）。
		// 止まった的なので、部位ごとの体力の減り方・のけぞりの向きを確かめやすい。
		ZombieAnimationNames tPose;
		tPose.move = nullptr;
		tPose.moveStartTime = 0.f;
		tPose.idle = nullptr;
		tPose.death = nullptr;
		tPose.downed = nullptr;
		tPose.stagger = nullptr;
		tPose.attack = nullptr;
		addZombie( toPosition( mMap.tPoseZombie() ), mMap.tPoseZombie().yawDegree, tPose );
	}
	mGround.place( mMap );
	mHitWorld.addBlocker( &mGround ); // 地面も弾・視線を止める（地面全体で1つの物）
	mCollisionWorld.setGround( &mGround ); // キャラは地面の高さに乗る（歩道の段差）
	placeBuildings( mBuildingData, mMap.buildings() );
	placeBuildings( mPerimeterBuildingData, mMap.perimeterBuildings() );
	// 弾を止める物として登録する。mBuildings を並べ終えてから行う（push_back の途中で登録すると、
	// 並べ直しで要素の場所が変わり、名簿が古い場所を指してしまうため）。
	for ( const Building& building : mBuildings ) {
		mHitWorld.addBlocker( &building );
	}
	placeMissionObjects();
	// 建物・車・ガソリンの置き場を名簿へ載せ終えてから、それを材料に歩ける床の形を作る（ZombieNavMesh_Plan.txt）。
	mNavMesh.build( mCollisionWorld );
}

void GameWorld::placePlayer() {
	const TownPlacement& start = mMap.playerStart();
	mPlayer = mPlayerFactory.spawn( &mObjects, toPosition( start ), start.yawDegree );
	mPlayerControl = mPlayer->getComponent< PlayerComponent >();
}

bool GameWorld::spawnZombie( const Vector3& position, float yawDegree ) {
	if ( !mZombieFactory || mObjects.count< ZombieBrainComponent >() >= kMaxZombies ) {
		return false;
	}
	ZombieAnimationNames animations;
	animations.move = kZombieAnimation;
	animations.moveStartTime = kAnimationStartStep * mZombieNumber;
	animations.idle = kZombieIdleAnimation;
	animations.death = kZombieDeathAnimations[ mZombieRandom.getInt( kZombieDeathAnimationNumber ) ];
	animations.downed = kZombieDownedAnimation;
	animations.stagger = kZombieStaggerAnimation;
	animations.attack = kZombieAttackAnimation;
	addZombie( position, yawDegree, animations );
	return true;
}

void GameWorld::addZombie( const Vector3& position, float yawDegree, const ZombieAnimationNames& animations ) {
	// 何体目か（mZombieNumber）で見通しを調べる時刻をずらす。うろつき方の乱数の種も体ごとに変える
	// （同じ種だと全員が同じ向き・同じ間で歩き出す）。
	mZombieFactory->spawn( &mObjects, position, yawDegree, animations, mZombieNumber, mZombieRandom.getInt() );
	++mZombieNumber;
}

void GameWorld::placeBuildings( const StaticMeshData* data, const std::vector< TownPlacement >& placements ) {
	// 当たり判定を三角形から作るので、構築に失敗した建物は置かない。
	if ( !data->isReady() ) {
		return;
	}
	mBuildings.reserve( mBuildings.size() + placements.size() );
	for ( const TownPlacement& p : placements ) {
		mBuildings.emplace_back( data, toPosition( p ), p.yawDegree, kBuildingMaterial );
		mBuildings.back().addCollidersTo( &mCollisionWorld );
	}
}

GameObject* GameWorld::createProp( GameObjectTag tag, const char* name, const StaticMeshData* data, const Vector3& position,
float yawDegree, DrawCulling::Target cullTarget, BoxColliderComponent::Mode colliderMode ) {
	GameObject* obj = mObjects.create( tag, name );
	obj->addComponent< TransformComponent >( position, yawDegree );
	obj->addComponent< StaticModelComponent >( data, kPropMaterial, cullTarget );
	obj->addComponent< BoxColliderComponent >( colliderMode ); // 箱は見た目の頂点から求める（見た目は awake で置かれる）
	return obj;
}

void GameWorld::placeMissionObjects() {
	// 箱の大きさを頂点から求めるので、構築に失敗したデータは置かない（その場合はクリアできないが、他の確認はできる）。
	if ( mVehicleData->isReady() ) {
		placeEscapeCar();
	}
	if ( mFuelCanData->isReady() ) {
		placeFuelStation();
		// 遊ぶたびに違う場所に出す（起動からの時間を種にする）。
		unsigned seed = Framework::instance().time();
		const std::vector< TownPlacement >& spawnPoints = mMap.fuelSpawnPoints();
		for ( int index : SpawnPicker::pick( static_cast< int >( spawnPoints.size() ), kRequiredFuel, seed ) ) {
			placeFuel( spawnPoints[ index ] );
		}
	}
}

void GameWorld::placeEscapeCar() {
	const TownPlacement& p = mMap.escapeCar();
	// 最後まで置いておく物なので、動かない箱として載せる（ゾンビの道の図にも、回り込む角として入る）。
	GameObject* obj = createProp( GameObjectTag::EscapeCar, kEscapeCarName, mVehicleData, toPosition( p ), p.yawDegree,
		DrawCulling::Target::Large, BoxColliderComponent::Mode::Fixed );
	mFuelTank = obj->addComponent< FuelTankComponent >( kRequiredFuel );
	mObjects.spawn( obj );
}

void GameWorld::placeFuelStation() {
	const TownPlacement& p = mMap.endlessFuel();
	GameObject* obj = createProp( GameObjectTag::FuelStation, kFuelStationName, mFuelCanData, toPosition( p ), p.yawDegree,
		DrawCulling::Target::Small, BoxColliderComponent::Mode::Fixed );
	obj->addComponent< ItemComponent >( ITEM_FUEL, kFuelPerCan );
	obj->addComponent< ItemSourceComponent >();
	mObjects.spawn( obj );
}

void GameWorld::placeFuel( const TownPlacement& p ) {
	// 拾うと消えるので、外せる箱で載せる（移動を止める。ユーザー指定）。
	GameObject* obj = createProp( GameObjectTag::Fuel, kFuelName, mFuelCanData, toPosition( p ), p.yawDegree,
		DrawCulling::Target::Small, BoxColliderComponent::Mode::Removable );
	obj->addComponent< ItemComponent >( ITEM_FUEL, kFuelPerCan );
	obj->addComponent< PickupComponent >();
	mObjects.spawn( obj );
}

void GameWorld::update( const PlayerCommand& command, float cameraYaw, float cameraPitch, const Vector3& viewDirection,
	Inventory* inventory ) {
	{
		PROFILE_SCOPE( "World.player" );
		if ( mPlayerControl ) {
			mPlayerControl->setInput( command, cameraYaw, cameraPitch, inventory );
		}
		mObjects.update( UpdatePhase::Input );
	}
	// 弾は銃口ではなく目から出す。画面の中央（照準）へ飛び、銃口が壁にめり込んでいても壁の向こうへ撃てないようにするため。
	const HoldComponent* hold = mPlayer ? mPlayer->getComponent< HoldComponent >() : nullptr; // 外せる部品（無ければ素手）
	const GunComponent* gun = hold ? hold->gun() : nullptr;
	if ( gun && gun->firedThisTick() ) {
		// 計測の印。押した瞬間（Fire）ではなく、実際に弾が出たティック（連射の 2 発目以降も）。
		PROFILE_EVENT( "Shot" );
		mBullets.fire( mPlayer->getComponent< FirstPersonBodyComponent >()->eyePosition(), viewDirection );
		alertZombiesToGunshot( mPlayer->getComponent< TransformComponent >()->position() );
	}
	// 撃ったティックのうちに1回進める（出たばかりの弾が、目の前に止まって見えないように）。
	mImpacts.clear();
	mZombieHits.clear();
	mSurfaceHits.clear();
	mZombieAttacks.clear();
	{
		PROFILE_SCOPE( "World.bullets" );
		mBullets.update( mHitWorld, playerTarget(), &mImpacts );
	}
	// 物（建物・車・地面・拾える物）に当たった所にだけ、弾痕と砂ぼこりを出し、着弾音のために材質を記録する。
	// ゾンビ（target）には出さず、血しぶきを出す。
	for ( const BulletList::Impact& impact : mImpacts ) {
		const HitWorld::Hit& hit = impact.hit;
		if ( hit.blocker ) {
			mBulletHoles.add( hit.point, hit.normal, hit.blocker );
			mImpactDust.spawn( hit.point, hit.normal );
			mSurfaceHits.push_back( { hit.point, hit.blocker->surfaceMaterial( hit.point ) } );
		} else if ( hit.target ) {
			hitZombie( impact );
		}
	}
	{
		PROFILE_SCOPE( "World.effects" );
		mBulletHoles.update();
		mImpactDust.update();
		mBloodSpray.update();
		mSeveredLimbs.update( &mGround ); // 落ちる先は地面だけ（建物・車の上には乗らず、抜ける）
	}
	{
		PROFILE_SCOPE( "World.zombies" );
		updateZombies();
	}
	// 動いた後の物に付いて動く部品（銃口の光。撃ったティックの描画が光り始めになる）。
	mObjects.update( UpdatePhase::Late );
	// このティックで消す印の付いたものを、全員の更新が済んでから消す（名簿からも外れる）。
	mObjects.flush();
	// どちらかが消えた死体と弾の組を捨てる（死体が消えた後も弾は残り、弾そのものを見て拾う）。
	mCorpseDrops.erase( std::remove_if( mCorpseDrops.begin(), mCorpseDrops.end(),
		[]( const CorpseDrop& link ) { return !link.corpse.get() || !link.drop.get(); } ), mCorpseDrops.end() );
	// 物の並びをたどり終えてから増やす（たどっている最中に spawn すると並びが動く）。
	spawnZombiesFromSpawner( cameraYaw );
}

void GameWorld::alertZombiesToGunshot( const Vector3& position ) {
	mObjects.forEach< ZombieStateComponent >( [ &position ]( ZombieStateComponent& zombie ) {
		Vector3 toZombie;
		toZombie.setSub( zombie.position(), position );
		toZombie.y = 0.f;
		if ( toZombie.squareLength() <= kGunshotHearingRange * kGunshotHearingRange ) {
			zombie.getComponent< ZombieBrainComponent >()->hearNoise( position ); // 聞くかどうか（倒れている・よろけている等）はゾンビが決める
		}
	} );
}

void GameWorld::updateZombies() {
	// ゾンビが追う先は自キャラ（このティックに動いた後の位置）。自キャラが居なければ追わない。
	mZombieContext.target = nullptr;
	if ( mPlayer ) {
		const PlayerHealthComponent* health = mPlayer->getComponent< PlayerHealthComponent >();
		mChaseTarget.position = mPlayer->getComponent< TransformComponent >()->position();
		mChaseTarget.eyePosition = mPlayer->getComponent< FirstPersonBodyComponent >()->eyePosition();
		mChaseTarget.attackable = !health || health->canBeAttacked(); // 体力の部品を外した自キャラ（死なない）も襲う
		mChaseTarget.attackTaken = false; // 体ごとに決め直す（ZombieBrainComponent::update）
		mChaseTarget.body = playerBody();
		mZombieContext.target = &mChaseTarget;
	}
	// 攻撃に行っているゾンビ（前のティックから続いている 1 体）。Think の段の途中で攻撃を始めた体も、そこへ入って
	// 後の体が攻撃を始めない（同じティックに 2 体が攻撃を始めないように）。
	mZombieContext.attacker = nullptr;
	mObjects.forEach< ZombieAttackComponent >( [ this ]( const ZombieAttackComponent& attack ) {
		if ( !mZombieContext.attacker && attack.isAttacking() ) {
			mZombieContext.attacker = attack.gameObject();
		}
	} );
	// 立って歩ける体の位置（このティックに動く前）。近くの仲間を避けるのに全員が読む（倒れた体・這う体・死体は数えない）。
	mCrowd.clear();
	mObjects.forEach< ZombieStateComponent >( [ this ]( const ZombieStateComponent& zombie ) {
		if ( zombie.state() == ZombieStateComponent::State::Alive ) {
			mCrowd.add( zombie.gameObject(), zombie.bodyCenter() );
		}
	} );
	mObjects.update( UpdatePhase::Think );
	PlayerHealthComponent* playerHealth = mPlayer ? mPlayer->getComponent< PlayerHealthComponent >() : nullptr;
	mObjects.forEach< ZombieAttackComponent >( [ & ]( const ZombieAttackComponent& attack ) {
		if ( attack.hitTargetThisTick() && playerHealth ) {
			// 体力は体全体で 1 つなので、体を引きずって足元から腕を伸ばすゾンビも同じダメージ。
			playerHealth->applyZombieHit( ZombieAttack::hitDamage() );
			ZombieAttackEvent event;
			event.point = attack.getComponent< ZombieStateComponent >()->headPosition();
			mZombieAttacks.push_back( event );
		}
	} );
	// 沈み終わった死体は、全員の更新が済んでから消す（update の flush）。
	mObjects.forEach< ZombieCorpseComponent >( [ this ]( const ZombieCorpseComponent& corpse ) {
		if ( corpse.isGone() ) {
			mObjects.destroyLater( corpse.gameObject() );
		}
	} );
}

void GameWorld::spawnZombiesFromSpawner( float cameraYaw ) {
	if ( !mZombieSpawner || !mPlayer ) {
		return; // ゾンビの形が無い・見えない所を決める自キャラが居ない
	}
	ZombieSpawner::View view;
	view.playerEye = mPlayer->getComponent< FirstPersonBodyComponent >()->eyePosition();
	view.viewYaw = cameraYaw;
	view.standingZombies = 0;
	mObjects.forEach< ZombieStateComponent >( [ &view ]( const ZombieStateComponent& zombie ) {
		if ( zombie.state() == ZombieStateComponent::State::Alive ) {
			++view.standingZombies;
		}
	} );
	view.fuelDelivered = mFuelTank ? mFuelTank->fuelDelivered() : 0;
	view.hasRoom = mObjects.count< ZombieBrainComponent >() < kMaxZombies;
	mSpawnOrders.clear();
	mZombieSpawner->update( view, &mSpawnOrders );
	for ( const ZombieSpawner::Order& order : mSpawnOrders ) {
		PROFILE_EVENT( "ZombieSpawn" ); // 計測の印（湧いた体ごと）
		spawnZombie( order.position, order.yawDegree );
	}
}

void GameWorld::hitZombie( const BulletList::Impact& impact ) {
	const HitWorld::Hit& hit = impact.hit;
	HitboxComponent* hitbox = mObjects.find< HitboxComponent >( hit.target );
	const ZombieStateComponent* state = hitbox ? hitbox->getComponent< ZombieStateComponent >() : nullptr;
	if ( !state ) {
		return; // ゾンビでない的（今は無いが、自キャラを撃つ相手が増えたとき）
	}
	PROFILE_EVENT( "ZombieHit" ); // 計測の印（血しぶき・ひるみ・部位欠損が始まる）
	// 血と出来事は、死体に当たったときも出す（体には変わりないため。体力はゾンビが生きている間だけ減らす）。
	mBloodSpray.spawn( hit.point, impact.direction, ( hit.part == BodyPart::Head ) ? kHeadBloodScale : kBodyBloodScale );
	ZombieHit event;
	event.point = hit.point;
	event.part = hit.part;
	mZombieHits.push_back( event );

	// 体力の部品は外せる（外したゾンビは撃っても死なない。血と当たった出来事だけ出す）。
	ZombieHealthComponent* zombie = state->getComponent< ZombieHealthComponent >();
	if ( !zombie ) {
		return;
	}
	DamageInfo damage;
	damage.part = hit.part;
	damage.baseAmount = Bullet::damage();
	damage.point = hit.point;
	damage.direction = impact.direction;
	bool wasAlive = state->isAlive();
	zombie->applyDamage( damage ); // 倒れ始めたら（脚を失って倒れたときも）、ゾンビが自分で移動の名簿から外れる
	if ( wasAlive && !state->isAlive() ) {
		dropAmmo( *state ); // 脚を失って倒れたまま生きている（Downed）間は落とさない。止めを刺したときに落とす
	}
	const SkinnedMeshInstance& mesh = zombie->getComponent< SkinnedModelComponent >()->mesh();
	for ( BodyPart part : zombie->getComponent< ZombieDismemberComponent >()->dismemberment().severedThisHit() ) {
		severLimb( mesh, part, impact.direction );
	}
}

void GameWorld::severLimb( const SkinnedMeshInstance& zombieMesh, BodyPart part, const Vector3& direction ) {
	// 切れた部位は、applyDamage の直後ならまだ見えている（体から消えるのは次の update）ので、その姿勢を写して飛ばす。
	zombieMesh.shownWorldPose( &mSeverPose );
	const ZombieSeverLayout::Cut& cut = mZombieModel->severLayout().cut( part );
	const Matrix34& joint = mSeverPose[ cut.jointBone ];
	mBloodSpray.spawn( Vector3( joint.m03, joint.m13, joint.m23 ), direction, kSeverBloodScale );
	mSeveredLimbs.spawn( mZombieData, cut, mSeverPose, direction );
}

void GameWorld::dropAmmo( const ZombieStateComponent& zombie ) {
	if ( !mAmmoDropData->isReady() || mZombieRandom.getFloat() >= kAmmoDropChance ) {
		return; // 形の構築に失敗した（理由は MeshCache が出力済み）・外れた
	}
	if ( mObjects.countTagged( GameObjectTag::AmmoDrop ) >= kMaxAmmoDrops ) {
		GameObject* oldest = mObjects.oldestTagged( GameObjectTag::AmmoDrop );
		mBulletHoles.removeAttachedTo( oldest->getComponent< StaticModelComponent >() ); // 物が消える前に外す
		mObjects.destroyLater( oldest );
	}
	Vector3 position = zombie.position();
	if ( mPlayer ) {
		Vector3 toPlayer;
		toPlayer.setSub( mPlayer->getComponent< TransformComponent >()->position(), position );
		toPlayer.y = 0.f;
		if ( toPlayer.length() > kAmmoDropTowardPlayer ) {
			toPlayer.normalize();
			position.madd( toPlayer, kAmmoDropTowardPlayer );
		}
	}
	float yaw = mZombieRandom.getFloat( kFullTurnDegree );
	// 足元の小さな物でつまずかないよう、移動の名簿には載せない（上を歩ける）。
	GameObject* obj = createProp( GameObjectTag::AmmoDrop, kAmmoDropName, mAmmoDropData, position, yaw,
		DrawCulling::Target::Small, BoxColliderComponent::Mode::WalkThrough );
	obj->addComponent< ItemComponent >( ITEM_AMMO, kAmmoDropRounds );
	obj->addComponent< PickupComponent >();
	mObjects.spawn( obj );
	mCorpseDrops.push_back( CorpseDrop{ GameObjectHandle( zombie.gameObject() ), GameObjectHandle( obj ) } );
}

void GameWorld::drawStatic( const DrawCulling& culling, const Vector3& eye ) const {
	// 建物は目に近い順。手前の建物が奥の建物を隠すので、奥の建物の画素の多くが色の計算の前に捨てられる。
	mBuildingDrawOrder.clear();
	for ( const Building& building : mBuildings ) {
		Vector3 toBuilding;
		toBuilding.setSub( building.position(), eye );
		mBuildingDrawOrder.emplace_back( toBuilding.squareLength(), &building );
	}
	std::sort( mBuildingDrawOrder.begin(), mBuildingDrawOrder.end(),
		[]( const std::pair< float, const Building* >& a, const std::pair< float, const Building* >& b ) { return a.first < b.first; } );
	for ( const auto& entry : mBuildingDrawOrder ) {
		entry.second->draw( culling );
	}
	Profiling::gpuMark( "Buildings" );
	// 置物（車・ガソリンの置き場・落ちているガソリン・ゾンビが落とした弾。作った順＝車が先）。
	mObjects.forEach< StaticModelComponent >( [ &culling ]( const StaticModelComponent& model ) { model.draw( culling ); } );
	Profiling::gpuMark( "Props" );
	// 地面は画面の下半分を覆うが、その多くは建物・キャラ・車に隠れるので最後。まとめて描くので映る範囲だけで間引く。
	mGround.draw( culling.frustum() );
	Profiling::gpuMark( "Ground" );
}

void GameWorld::drawFirstPerson() const {
	// 一人称の腕と銃は画面の大きな所を占め、いつも一番手前にあるので、動く物の中では最初に描く。
	drawPlayer();
	mBullets.draw();
	Profiling::gpuMark( "Player" );
}

void GameWorld::drawPlayer() const {
	if ( !mPlayer ) {
		return;
	}
	mPlayer->getComponent< SkinnedModelComponent >()->draw();
	if ( const HoldComponent* hold = mPlayer->getComponent< HoldComponent >() ) {
		hold->draw(); // しまった武器は描かない
	}
}

void GameWorld::drawCharacters( const DrawCulling& culling ) const {
	drawZombies( culling );
	mSeveredLimbs.draw( culling );
	Profiling::gpuMark( "Zombies" );
}

void GameWorld::drawShadowCasters( const DrawCulling& culling, bool includesPlayer ) const {
	// 深度だけなので、draw のような近い順の並べ替えはしない（速さにほとんど効かない）。
	// 自キャラは部位の箱で間引く（月の遠い枚には映らないことが多い）。
	if ( includesPlayer && mPlayer && culling.showsPosed( playerTarget()->hitboxes(), DrawCulling::Target::Large ) ) {
		drawPlayer();
	}
	drawZombies( culling );
	mSeveredLimbs.draw( culling );
	for ( const Building& building : mBuildings ) {
		building.draw( culling );
	}
	mObjects.forEach< StaticModelComponent >( [ &culling ]( const StaticModelComponent& model ) { model.draw( culling ); } );
}

void GameWorld::drawZombies( const DrawCulling& culling ) const {
	// 部位の箱（毎ティック今のポーズへ置き直している）を囲む箱で間引く（OptimizationA_Plan.txt 6-1）。
	mObjects.forEach< ZombieBrainComponent >( [ &culling ]( const ZombieBrainComponent& zombie ) {
		const GameObject* object = zombie.gameObject();
		if ( culling.showsPosed( object->getComponent< HitboxComponent >()->hitboxes(), DrawCulling::Target::Zombie ) ) {
			object->getComponent< SkinnedModelComponent >()->draw();
		}
	} );
}

void GameWorld::drawBulletHoles( const Matrix44& projectionView ) {
	mBulletHoles.draw( projectionView );
}

void GameWorld::drawImpactDust( const Matrix44& projectionView, const Vector3& viewDirection, const Vector3& eye ) {
	mImpactDust.draw( projectionView, viewDirection, eye );
}

void GameWorld::drawBloodSpray( const Matrix44& projectionView, const Vector3& viewDirection, const Vector3& eye ) {
	mBloodSpray.draw( projectionView, viewDirection, eye );
}

void GameWorld::drawMuzzleFlash( const Matrix44& projectionView, const Vector3& viewDirection ) {
	if ( !mPlayer ) {
		return;
	}
	const HoldComponent* hold = mPlayer->getComponent< HoldComponent >();
	GameObject* weapon = hold ? hold->heldObject() : nullptr; // 持つ部品が無ければ素手
	MuzzleFlashComponent* flash = weapon ? weapon->getComponent< MuzzleFlashComponent >() : nullptr;
	Vector3 muzzlePosition;
	Vector3 muzzleDirection;
	// 撃った後に銃をしまったら、銃口が無いので描かない。
	if ( !flash || !hold->muzzle( &muzzlePosition, &muzzleDirection ) ) {
		return;
	}
	flash->flash().draw( projectionView, viewDirection, muzzlePosition, muzzleDirection, hold->aimWeight() );
}

bool GameWorld::muzzleLight( LightSource* out ) const {
	if ( !mPlayer ) {
		return false;
	}
	const HoldComponent* hold = mPlayer->getComponent< HoldComponent >();
	const GameObject* weapon = hold ? hold->heldObject() : nullptr; // 持つ部品が無ければ素手
	const MuzzleFlashComponent* flash = weapon ? weapon->getComponent< MuzzleFlashComponent >() : nullptr;
	Vector3 muzzlePosition;
	Vector3 muzzleDirection;
	if ( !flash || !hold->muzzle( &muzzlePosition, &muzzleDirection ) ) {
		return false;
	}
	return flash->flash().light( muzzlePosition, muzzleDirection, out );
}

void GameWorld::removeTakenPickups() {
	// ガソリンも弾も同じ手順（どちらも PickupComponent を持つ物）。
	mObjects.forEach< PickupComponent >( [ this ]( PickupComponent& pickup ) {
		if ( pickup.isTaken() ) {
			mBulletHoles.removeAttachedTo( pickup.getComponent< StaticModelComponent >() ); // 物が消える前に外す
			mObjects.destroyLater( pickup.gameObject() );
		}
	} );
}

IInteractable* GameWorld::findInteractable( const Vector3& origin, const Vector3& delta ) {
	// 弾と同じ名簿で一番手前の物を探す（壁の向こうは見えない・死体にも当たる）。自分の体には当てない（目が体の中にあるため）。
	float nearest = 1.f;
	IInteractable* found = nullptr;
	HitWorld::Hit hit;
	if ( mHitWorld.raycast( origin, delta, playerTarget(), &hit ) ) {
		nearest = hit.time;
		found = resolveInteractable( hit );
	}
	// 落ちている物の見る箱は名簿に無いので、ここで調べる。見た目より大きいので、手前に当たった物より近ければ見る箱を選ぶ。
	mObjects.forEach< PickupComponent >( [ &origin, &delta, &nearest, &found ]( PickupComponent& pickup ) {
		float time;
		if ( pickup.lookRaycast( origin, delta, &time ) && time <= nearest ) {
			nearest = time;
			found = &pickup;
		}
	} );
	return found;
}

IInteractable* GameWorld::resolveInteractable( const HitWorld::Hit& hit ) {
	if ( hit.target ) {
		// 死体（倒れ始めた後）なら、その死体が落とした弾。生きているゾンビ・脚を失って這っているゾンビ（まだ落としていない）は使えない。
		const HitboxComponent* hitbox = mObjects.find< HitboxComponent >( hit.target );
		const ZombieStateComponent* zombie = hitbox ? hitbox->getComponent< ZombieStateComponent >() : nullptr;
		if ( !zombie || zombie->isAlive() ) {
			return nullptr;
		}
		GameObjectHandle corpse( zombie->gameObject() );
		for ( const CorpseDrop& link : mCorpseDrops ) {
			GameObject* drop = link.drop.get();
			if ( link.corpse == corpse && drop ) {
				return drop->getComponent< PickupComponent >();
			}
		}
		return nullptr; // 弾を落とさなかった死体
	}
	// 落ちているガソリン・落ちた弾・車・置き場（弾を止める見た目の部品から、その物の使える部品をたどる）。
	if ( StaticModelComponent* model = mObjects.find< StaticModelComponent >( hit.blocker ) ) {
		return interactableOf( model->gameObject() );
	}
	return nullptr;
}

const TownMap& GameWorld::map() const {
	return mMap;
}

const NavMesh& GameWorld::navMesh() const {
	return mNavMesh;
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

GameObject* GameWorld::player() {
	return mPlayer;
}

const GameObject* GameWorld::player() const {
	return mPlayer;
}

const IHittable* GameWorld::playerTarget() const {
	return mPlayer ? mPlayer->getComponent< HitboxComponent >() : nullptr;
}

const ICollisionBody* GameWorld::playerBody() const {
	return mPlayer ? mPlayer->getComponent< CharacterColliderComponent >() : nullptr;
}

const FuelTankComponent* GameWorld::fuelTank() const {
	return mFuelTank;
}

bool GameWorld::isEscapeReady() const {
	return mFuelTank && mFuelTank->isFueled();
}

const GameObjectWorld& GameWorld::objects() const {
	return mObjects;
}

const std::vector< GameWorld::ZombieHit >& GameWorld::zombieHitsThisTick() const {
	return mZombieHits;
}

const std::vector< GameWorld::SurfaceHit >& GameWorld::surfaceHitsThisTick() const {
	return mSurfaceHits;
}

const ZombieSpawner* GameWorld::zombieSpawner() const {
	return mZombieSpawner.get();
}

void GameWorld::appendPickupColliders( std::vector< BoxCollider >* out ) const {
	// 見せるのはガソリンだけ（落ちた弾は足元に小さく落ちているだけで、運ぶ物ではない）。
	mObjects.forEach< PickupComponent >( [ out ]( const PickupComponent& pickup ) {
		if ( pickup.gameObject()->tag() == GameObjectTag::Fuel && !pickup.isTaken() ) {
			out->push_back( pickup.getComponent< BoxColliderComponent >()->collider() );
		}
	} );
}

const std::vector< GameWorld::ZombieAttackEvent >& GameWorld::zombieAttacksThisTick() const {
	return mZombieAttacks;
}

bool GameWorld::isPlayerDead() const {
	const PlayerHealthComponent* health = mPlayer ? mPlayer->getComponent< PlayerHealthComponent >() : nullptr;
	return health && health->isDead(); // 体力の部品を外した自キャラは死なない
}
