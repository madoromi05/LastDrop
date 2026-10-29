#include "GameSrc/Scene/Game/GameWorld.h"
#include "GameSrc/Render/MeshCache.h"
#include "GameSrc/Player/PlayerCharacter.h"
#include "GameSrc/Map/SpawnPicker.h"
#include "GameSrc/Hit/DamageInfo.h"
#include "GameSrc/Zombie/ChaseTarget.h"
#include "GameSrc/Zombie/ZombieCrowdParams.h"

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

	// ── 自キャラ（Mixamo Ch15。身長約1.8）──
	const char* kPlayerFile = "Resource/player.txt"; // tools/configs/player.json で変換
	const char* kWeaponFile = "Resource/rifle.txt"; // tools/configs/rifle.json で変換（AKM。弾倉を除いた形）
	const char* kMagazineFile = "Resource/rifle_mag.txt"; // tools/configs/rifle_mag.json で変換（AKM の弾倉だけ。銃と同じ座標）
	// 飛ぶ弾（7.62 NATO の弾頭。実物大・底が原点・先が +Y）。tools/configs/bullet_tip.json で変換
	const char* kBulletFile = "Resource/bullet_tip.txt";

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
	// 追いついたら、つかんで噛む攻撃のアニメ（同じく extra_animations。両手を前へ伸ばしてつかむ）。
	const char* kZombieAttackAnimation = "atack3";
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
	const bool kBlocksMovement = true;  // ガソリンは移動を止める（ユーザー指定）
	const bool kWalkOver = false;       // 落ちた弾は上を歩ける（足元の小さな物で引っかからないように）

	// ── ゾンビが落とす予備の弾（2026-10-22 ユーザー指定：40% で 10 発）──
	// 1 体あたり平均 4 発。倒すのに 3〜6 発使うので、無駄撃ちすると減っていく（バイオハザードの乏しさ）。
	const char* kAmmoDropFile = "Resource/ammo_drop.txt"; // tools/configs/ammo_drop.json で変換（7.62 NATO の弾薬を 3 倍にして寝かせた物）
	const float kAmmoDropChance = 0.4f;
	const int kAmmoDropRounds = 10;
	// 同時に落ちていられる数。超えたら一番古い物を消す（拾わずに倒し続けても名簿が増え続けないように）。
	const int kMaxAmmoDrops = 10;
	// 足元から自キャラの方へ寄せる長さ（m）。死体の脚の下に隠れにくくする（視線は死体を素通りするので、隠れても拾える）。
	const float kAmmoDropTowardPlayer = 0.5f;
	const float kFullTurnDegree = 360.f; // 落ちた弾の向きは体ごとにばらばら

	Vector3 toPosition( const TownPlacement& placement ) {
		return Vector3( placement.x, placement.y, placement.z );
	}
}

GameWorld::GameWorld( MeshCache* meshes ) :
	mStreets( mMap ),
	mPlayerData( nullptr ),
	mWeaponData( nullptr ),
	mMagazineData( nullptr ),
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
	mZombieNumber( 0 ) {
	ASSERT( meshes && "GameWorld : meshes is null." );
	// 読み込みを頼むだけ（非同期）。構築が終わったら place で並べる。
	mPlayerData = meshes->skinnedMesh( kPlayerFile );
	mWeaponData = meshes->staticMesh( kWeaponFile );
	mMagazineData = meshes->staticMesh( kMagazineFile );
	mZombieData = meshes->skinnedMesh( kZombieFile );
	mBuildingData = meshes->staticMesh( kBuildingFile );
	mPerimeterBuildingData = meshes->staticMesh( kPerimeterBuildingFile );
	mVehicleData = meshes->staticMesh( kVehicleFile );
	mFuelCanData = meshes->staticMesh( kFuelCanFile );
	mAmmoDropData = meshes->staticMesh( kAmmoDropFile );
}

// PlayerCharacter の定義が見える所で破棄する（ヘッダでは前方宣言だけ）。
GameWorld::~GameWorld() {
}

void GameWorld::place() {
	if ( mPlayerData->isReady() ) {
		// 銃の構築に失敗していたら素手で出す（キャラまで出さないと操作の確認ができないため）。
		const StaticMeshData* weapon = mWeaponData->isReady() ? mWeaponData : nullptr;
		const StaticMeshData* magazine = mMagazineData->isReady() ? mMagazineData : nullptr; // 失敗したら弾倉の無い銃になるだけ
		const TownPlacement& start = mMap.playerStart();
		mPlayer = std::make_unique< PlayerCharacter >( mPlayerData, weapon, magazine, &mCollisionWorld, toPosition( start ), start.yawDegree );
		mPlayer->addTo( &mCollisionWorld, &mHitWorld );
	}
	if ( mZombieData->isReady() ) {
		// 根の動きを置き場所へ移すのは、1回だけ流すアニメ（攻撃・よろけ・倒れる。SummaryPlan/個別仕様/ZombieRootMotion_Plan.txt）。
		std::vector< const char* > rootMotionAnimations( std::begin( kZombieDeathAnimations ), std::end( kZombieDeathAnimations ) );
		rootMotionAnimations.push_back( kZombieStaggerAnimation );
		rootMotionAnimations.push_back( kZombieAttackAnimation );
		mZombieModel = std::make_unique< ZombieModel >( mZombieData, kZombieAnimation, rootMotionAnimations );
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
		Zombie::Animations tPose;
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
}

bool GameWorld::spawnZombie( const Vector3& position, float yawDegree ) {
	if ( !mZombieModel || mObjects.count< Zombie >() >= kMaxZombies ) {
		return false;
	}
	Zombie::Animations animations;
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

void GameWorld::addZombie( const Vector3& position, float yawDegree, const Zombie::Animations& animations ) {
	// 移動・命中の両方の当たり判定への登録は GameObjectWorld が行う。何体目か（mZombieNumber）で見通しを調べる時刻をずらす。
	// うろつき方の乱数の種も体ごとに変える（同じ種だと全員が同じ向き・同じ間で歩き出す）。
	int randomSeed = mZombieRandom.getInt();
	mObjects.spawn( GameObjectTag::Zombie, std::make_unique< Zombie >( mZombieModel.get(), &mCollisionWorld, &mStreets,
		position, yawDegree, animations, mZombieNumber, randomSeed ) );
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

void GameWorld::placeMissionObjects() {
	// 箱の大きさを頂点から求めるので、構築に失敗したデータは置かない（その場合はクリアできないが、他の確認はできる）。
	if ( mVehicleData->isReady() ) {
		const TownPlacement& p = mMap.escapeCar();
		mEscapeCar = std::make_unique< EscapeCar >( mVehicleData, toPosition( p ), p.yawDegree, kRequiredFuel );
		mEscapeCar->addTo( &mCollisionWorld, &mHitWorld );
		// 車は通りの途中にあり、交差点を結ぶ道が車を通り抜けるので、車の角を道の図に足す（ZombieCrowd_Plan.txt の 4 章）。
		mStreets.addObstacle( mEscapeCar->collider(), zombieCrowdParams().obstacleMargin, mHitWorld );
	}
	if ( mFuelCanData->isReady() ) {
		const TownPlacement& endless = mMap.endlessFuel();
		mEndlessFuel = std::make_unique< ItemSource >( mFuelCanData, toPosition( endless ), endless.yawDegree, ITEM_FUEL );
		mEndlessFuel->addTo( &mCollisionWorld, &mHitWorld );
		mStreets.addObstacle( mEndlessFuel->collider(), zombieCrowdParams().obstacleMargin, mHitWorld );
		// 遊ぶたびに違う場所に出す（起動からの時間を種にする）。
		unsigned seed = Framework::instance().time();
		const std::vector< TownPlacement >& spawnPoints = mMap.fuelSpawnPoints();
		for ( int index : SpawnPicker::pick( static_cast< int >( spawnPoints.size() ), kRequiredFuel, seed ) ) {
			const TownPlacement& p = spawnPoints[ index ];
			mObjects.spawn( GameObjectTag::Fuel,
				std::make_unique< Pickup >( mFuelCanData, toPosition( p ), p.yawDegree, ITEM_FUEL, kFuelPerCan, kBlocksMovement ) );
		}
	}
}

void GameWorld::update( const PlayerCommand& command, float cameraYaw, float cameraPitch, const Vector3& viewDirection,
	Inventory* inventory ) {
	// 前に撃った光を先に進めてから、このティックに撃った光を灯す（撃ったティックの描画が光り始めになる）。
	mMuzzleFlash.update();
	if ( mPlayer ) {
		PROFILE_SCOPE( "World.player" );
		mPlayer->update( command, cameraYaw, cameraPitch, inventory );
		// 弾は銃口ではなく目から出す。画面の中央（照準）へ飛び、銃口が壁にめり込んでいても壁の向こうへ撃てないようにするため。
		if ( mPlayer->firedThisTick() ) {
			// 計測の印。押した瞬間（Fire）ではなく、実際に弾が出たティック（連射の 2 発目以降も）。
			PROFILE_EVENT( "Shot" );
			mBullets.fire( mPlayer->eyePosition(), viewDirection );
			mMuzzleFlash.fire();
			alertZombiesToGunshot( mPlayer->position() );
		}
	}
	// 撃ったティックのうちに1回進める（出たばかりの弾が、目の前に止まって見えないように）。
	mImpacts.clear();
	mZombieHits.clear();
	mSurfaceHits.clear();
	mZombieAttacks.clear();
	{
		PROFILE_SCOPE( "World.bullets" );
		mBullets.update( mHitWorld, mPlayer.get(), &mImpacts );
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
	// このティックで消す印の付いたものを、全員の更新が済んでから消す（名簿からも外れる）。
	mObjects.flush();
	// 名簿をたどり終えてから増やす（たどっている最中に spawn すると並びが動く）。
	spawnZombiesFromSpawner( cameraYaw );
}

void GameWorld::alertZombiesToGunshot( const Vector3& position ) {
	mObjects.forEach< Zombie >( [ &position ]( Zombie& zombie ) {
		Vector3 toZombie;
		toZombie.setSub( zombie.position(), position );
		toZombie.y = 0.f;
		if ( toZombie.squareLength() <= kGunshotHearingRange * kGunshotHearingRange ) {
			zombie.hearNoise( position ); // 聞くかどうか（倒れている・よろけている等）はゾンビが決める
		}
	} );
}

void GameWorld::updateZombies() {
	// ゾンビが追う先は自キャラ（このティックに動いた後の位置）。自キャラが居なければ追わない。
	ChaseTarget target;
	const ChaseTarget* chaseTarget = nullptr;
	if ( mPlayer ) {
		target.position = mPlayer->position();
		target.eyePosition = mPlayer->eyePosition();
		target.grabbable = mPlayer->canBeGrabbed();
		target.attackTaken = false;
		target.body = mPlayer.get();
		chaseTarget = &target;
	}
	// 攻撃に行っているゾンビ（前のティックから続いている 1 体）。名簿をたどる途中で攻撃を始めた体も、ここへ入れて
	// 後の体に attackTaken を渡す（同じティックに 2 体が攻撃を始めないように）。
	const Zombie* attacker = nullptr;
	mObjects.forEach< Zombie >( [ &attacker ]( const Zombie& zombie ) {
		if ( !attacker && zombie.isAttacking() ) {
			attacker = &zombie;
		}
	} );
	// 立って歩ける体の位置（このティックに動く前）。近くの仲間を避けるのに全員が読む（倒れた体・這う体・死体は数えない）。
	mCrowd.clear();
	mObjects.forEach< Zombie >( [ this ]( const Zombie& zombie ) {
		if ( zombie.state() == Zombie::State::Alive ) {
			mCrowd.add( &zombie, zombie.bodyCenter() );
		}
	} );
	const Zombie* holder = nullptr;
	mObjects.forEach< Zombie >( [ & ]( Zombie& zombie ) {
		target.attackTaken = ( attacker != nullptr && attacker != &zombie );
		zombie.update( chaseTarget, mHitWorld, &mCrowd );
		if ( !attacker && zombie.isAttacking() ) {
			attacker = &zombie;
		}
		if ( zombie.isHoldingTarget() ) {
			holder = &zombie;
		}
		if ( zombie.grabbedThisTick() ) {
			ZombieAttackEvent event;
			event.kind = ZombieAttackEvent::Kind::Grab;
			event.point = zombie.headPosition();
			mZombieAttacks.push_back( event );
		}
		if ( zombie.bitThisTick() && mPlayer ) {
			// 体力は体全体で 1 つなので、体を引きずって足元から噛むゾンビも同じダメージ。
			mPlayer->applyBite( ZombieAttack::biteDamage() );
			ZombieAttackEvent event;
			event.kind = ZombieAttackEvent::Kind::Bite;
			event.point = zombie.headPosition();
			mZombieAttacks.push_back( event );
		}
		// 沈み終わった死体は、全員の更新が済んでから消す（update の flush）。
		if ( zombie.isGone() ) {
			mObjects.destroyLater( zombie.gameObject() );
		}
	} );
	// つかんでいるゾンビが倒れた・振りほどかれた・噛み終えたら、次の update から動ける。
	if ( mPlayer ) {
		mPlayer->setHeld( holder != nullptr );
	}
}

void GameWorld::spawnZombiesFromSpawner( float cameraYaw ) {
	if ( !mZombieSpawner || !mPlayer ) {
		return; // ゾンビの形が無い・見えない所を決める自キャラが居ない
	}
	ZombieSpawner::View view;
	view.playerEye = mPlayer->eyePosition();
	view.viewYaw = cameraYaw;
	view.standingZombies = 0;
	mObjects.forEach< Zombie >( [ &view ]( const Zombie& zombie ) {
		if ( zombie.state() == Zombie::State::Alive ) {
			++view.standingZombies;
		}
	} );
	view.fuelDelivered = mEscapeCar ? mEscapeCar->fuelDelivered() : 0;
	view.hasRoom = mObjects.count< Zombie >() < kMaxZombies;
	mSpawnOrders.clear();
	mZombieSpawner->update( view, &mSpawnOrders );
	for ( const ZombieSpawner::Order& order : mSpawnOrders ) {
		PROFILE_EVENT( "ZombieSpawn" ); // 計測の印（湧いた体ごと）
		spawnZombie( order.position, order.yawDegree );
	}
}

void GameWorld::hitZombie( const BulletList::Impact& impact ) {
	const HitWorld::Hit& hit = impact.hit;
	Zombie* zombie = mObjects.find< Zombie >( hit.target );
	if ( !zombie ) {
		return; // ゾンビでない的（今は無いが、自キャラを撃つ相手が増えたとき）
	}
	PROFILE_EVENT( "ZombieHit" ); // 計測の印（血しぶき・ひるみ・部位欠損が始まる）
	// 血と出来事は、死体に当たったときも出す（体には変わりないため。体力は Zombie が生きている間だけ減らす）。
	mBloodSpray.spawn( hit.point, impact.direction, ( hit.part == BodyPart::Head ) ? kHeadBloodScale : kBodyBloodScale );
	ZombieHit event;
	event.point = hit.point;
	event.part = hit.part;
	mZombieHits.push_back( event );

	DamageInfo damage;
	damage.part = hit.part;
	damage.baseAmount = Bullet::damage();
	damage.point = hit.point;
	damage.direction = impact.direction;
	bool wasAlive = zombie->isAlive();
	zombie->applyDamage( damage );
	if ( wasAlive && !zombie->isAlive() ) {
		dropAmmo( *zombie ); // 脚を失って倒れたまま生きている（Downed）間は落とさない。止めを刺したときに落とす
	}
	for ( BodyPart part : zombie->severedByLastDamage() ) {
		severLimb( *zombie, part, impact.direction );
	}
	// 倒れ始めたら（脚を失って倒れたときも）移動の名簿から外す（死体の上を歩ける）。命中の名簿には消えるまで残す（死体も弾を止める）。
	// ここは名簿をたどっている最中ではないので、すぐ外してよい。外すのは何回呼んでも安全。
	if ( !zombie->blocksMovement() ) {
		mCollisionWorld.removeBody( zombie );
	}
}

void GameWorld::severLimb( const Zombie& zombie, BodyPart part, const Vector3& direction ) {
	// 切れた部位は、applyDamage の直後ならまだ見えている（体から消えるのは次の update）ので、その姿勢を写して飛ばす。
	zombie.shownWorldPose( &mSeverPose );
	const ZombieSeverLayout::Cut& cut = mZombieModel->severLayout().cut( part );
	const Matrix34& joint = mSeverPose[ cut.jointBone ];
	mBloodSpray.spawn( Vector3( joint.m03, joint.m13, joint.m23 ), direction, kSeverBloodScale );
	mSeveredLimbs.spawn( mZombieData, cut, mSeverPose, direction );
}

void GameWorld::dropAmmo( const Zombie& zombie ) {
	if ( !mAmmoDropData->isReady() || mZombieRandom.getFloat() >= kAmmoDropChance ) {
		return; // 形の構築に失敗した（理由は MeshCache が出力済み）・外れた
	}
	if ( mObjects.countTagged( GameObjectTag::AmmoDrop ) >= kMaxAmmoDrops ) {
		GameObject* oldest = mObjects.oldestTagged( GameObjectTag::AmmoDrop );
		mBulletHoles.removeAttachedTo( oldest->component< Pickup >() ); // 物が消える前に外す
		mObjects.destroyLater( oldest );
	}
	Vector3 position = zombie.position();
	if ( mPlayer ) {
		Vector3 toPlayer;
		toPlayer.setSub( mPlayer->position(), position );
		toPlayer.y = 0.f;
		if ( toPlayer.length() > kAmmoDropTowardPlayer ) {
			toPlayer.normalize();
			position.madd( toPlayer, kAmmoDropTowardPlayer );
		}
	}
	float yaw = mZombieRandom.getFloat( kFullTurnDegree );
	mObjects.spawn( GameObjectTag::AmmoDrop, std::make_unique< Pickup >( mAmmoDropData, position, yaw, ITEM_AMMO, kAmmoDropRounds, kWalkOver ) );
}

void GameWorld::draw( const ViewFrustum& frustum, const Vector3& eye ) const {
	// 一人称の腕と銃は画面の大きな所を占め、いつも一番手前にあるので最初に描く。
	if ( mPlayer ) {
		mPlayer->draw();
	}
	mBullets.draw();
	Profiling::gpuMark( "Player" );
	mObjects.forEach< Zombie >( []( const Zombie& zombie ) { zombie.draw(); } );
	mSeveredLimbs.draw();
	Profiling::gpuMark( "Zombies" );
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
		entry.second->draw( frustum );
	}
	Profiling::gpuMark( "Buildings" );
	if ( mEscapeCar ) {
		mEscapeCar->draw( frustum );
	}
	if ( mEndlessFuel ) {
		mEndlessFuel->draw( frustum );
	}
	// 落ちているガソリンと、ゾンビが落とした弾（どちらも Pickup の部品）。
	mObjects.forEach< Pickup >( [ &frustum ]( const Pickup& pickup ) { pickup.draw( frustum ); } );
	Profiling::gpuMark( "Props" );
	// 地面は画面の下半分を覆うが、その多くは建物・キャラ・車に隠れるので最後。
	mGround.draw( frustum );
	Profiling::gpuMark( "Ground" );
}

void GameWorld::drawShadowCasters( const ViewFrustum& frustum, bool includesPlayer ) const {
	// 深度だけなので、draw のような近い順の並べ替えはしない（速さにほとんど効かない）。
	if ( includesPlayer && mPlayer ) {
		mPlayer->draw();
	}
	mObjects.forEach< Zombie >( []( const Zombie& zombie ) { zombie.draw(); } );
	mSeveredLimbs.draw();
	for ( const Building& building : mBuildings ) {
		building.draw( frustum );
	}
	if ( mEscapeCar ) {
		mEscapeCar->draw( frustum );
	}
	if ( mEndlessFuel ) {
		mEndlessFuel->draw( frustum );
	}
	mObjects.forEach< Pickup >( [ &frustum ]( const Pickup& pickup ) { pickup.draw( frustum ); } );
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
	Vector3 muzzlePosition;
	Vector3 muzzleDirection;
	// 撃った後に銃をしまったら、銃口が無いので描かない。
	if ( !mPlayer || !mPlayer->muzzle( &muzzlePosition, &muzzleDirection ) ) {
		return;
	}
	mMuzzleFlash.draw( projectionView, viewDirection, muzzlePosition, muzzleDirection, mPlayer->aimWeight() );
}

bool GameWorld::muzzleLight( LightSource* out ) const {
	Vector3 muzzlePosition;
	Vector3 muzzleDirection;
	if ( !mPlayer || !mPlayer->muzzle( &muzzlePosition, &muzzleDirection ) ) {
		return false;
	}
	return mMuzzleFlash.light( muzzlePosition, muzzleDirection, out );
}

void GameWorld::removeTakenPickups() {
	// ガソリンも弾も同じ手順（どちらも Pickup の部品）。
	mObjects.forEach< Pickup >( [ this ]( Pickup& pickup ) {
		if ( pickup.isTaken() ) {
			mBulletHoles.removeAttachedTo( &pickup ); // 物が消える前に外す
			mObjects.destroyLater( pickup.gameObject() );
		}
	} );
}

IInteractable* GameWorld::resolveInteractable( const HitWorld::Hit& hit ) {
	if ( !hit.blocker ) {
		return nullptr; // キャラ（ゾンビ）は使えない
	}
	if ( Pickup* pickup = mObjects.find< Pickup >( hit.blocker ) ) {
		return pickup; // 落ちているガソリン・落ちた弾
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

const StreetGraph& GameWorld::streets() const {
	return mStreets;
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
	mObjects.forEach< Pickup >( [ out ]( const Pickup& pickup ) {
		if ( pickup.gameObject()->tag() == GameObjectTag::Fuel && !pickup.isTaken() ) {
			out->push_back( pickup.collider() );
		}
	} );
}

const std::vector< GameWorld::ZombieAttackEvent >& GameWorld::zombieAttacksThisTick() const {
	return mZombieAttacks;
}

bool GameWorld::isPlayerDead() const {
	return mPlayer && mPlayer->isDead();
}
