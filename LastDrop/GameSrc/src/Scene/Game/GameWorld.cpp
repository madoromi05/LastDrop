#include "GameSrc/Scene/Game/GameWorld.h"
#include "GameSrc/Render/MeshCache.h"
#include "GameSrc/Component/TransformComponent.h"
#include "GameSrc/Component/HitboxComponent.h"
#include "GameSrc/Component/CharacterColliderComponent.h"
#include "GameSrc/Player/PlayerComponent.h"
#include "GameSrc/Player/FirstPersonBodyComponent.h"
#include "GameSrc/Player/HoldComponent.h"
#include "GameSrc/Player/PlayerHealthComponent.h"
#include "GameSrc/Weapon/GunComponent.h"
#include "GameSrc/Interaction/FuelTankComponent.h"
#include "GameSrc/Zombie/ZombieStateComponent.h"
#include "GameSrc/Zombie/ZombieAttackComponent.h"
#include "GameSrc/Zombie/ZombieCorpseComponent.h"

#include "GameLib/GameLib.h"
#include "Framework.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Random.h"
#include "GameLib/Profiling/Profiler.h"

#include <iterator>

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// 置き場所はどれも TownMap が決める。ここにあるのは、何を置くか（ファイル）と、置き場所以外の決めごとだけ。
	// 建物・置物は WorldBuilder・PropFactory、血の量は EffectManager、落とす弾は CombatResolver が持つ。

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
	// 追いついたら、腕で引っかく攻撃のアニメ（同じく extra_animations。右手を振り上げて振り下ろす）。
	const char* kZombieAttackAnimation = "atack1";

	Vector3 toPosition( const TownPlacement& placement ) {
		return Vector3( placement.x, placement.y, placement.z );
	}
}

GameWorld::GameWorld( MeshCache* meshes ) :
	mPlayerFactory( meshes ),
	mPropFactory( meshes ),
	mBuilder( meshes, &mMap, &mPropFactory ),
	mBulletData( meshes->staticMesh( kBulletFile ) ), // mBullets が借りるので、ここで頼む
	mZombieData( meshes->skinnedMesh( kZombieFile ) ),
	mGround( meshes ),
	mBullets( mBulletData ),
	mObjects( &mCollisionWorld, &mHitWorld ),
	mCombat( &mObjects, &mEffects, &mPropFactory, &mHitWorld ),
	mInteraction( &mHitWorld, &mObjects, &mEffects ),
	mPlayer( nullptr ),
	mPlayerControl( nullptr ),
	mFuelTank( nullptr ),
	mZombieNumber( 0 ) {
	ASSERT( meshes && "GameWorld : meshes is null." );
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
	WorldBuildTarget target;
	target.ground = &mGround;
	target.buildings = &mBuildings;
	target.collisionWorld = &mCollisionWorld;
	target.hitWorld = &mHitWorld;
	target.objects = &mObjects;
	target.navMesh = &mNavMesh;
	mFuelTank = mBuilder.build( target );
}

void GameWorld::placePlayer() {
	const TownPlacement& start = mMap.playerStart();
	mPlayer = mPlayerFactory.spawn( &mObjects, toPosition( start ), start.yawDegree );
	mPlayerControl = mPlayer->getComponent< PlayerComponent >();
}

void GameWorld::setAnimationView( const Matrix44& projectionView, const Vector3& eye ) {
	mAnimationView.emplace( projectionView, eye );
	mZombieContext.view = &*mAnimationView;
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

void GameWorld::update( const PlayerCommand& command, float cameraYaw, float cameraPitch, const Vector3& viewDirection,
	Inventory* inventory ) {
	// 動く体を今の置き場所でますへ入れ直させる（体が動く前に 1 回。OptimizationB_Plan.txt 7 章）。
	mCollisionWorld.refreshBodies();
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
	mCombat.beginTick();
	{
		PROFILE_SCOPE( "World.bullets" );
		mBullets.update( mHitWorld, playerTarget(), &mImpacts );
	}
	mCombat.resolveBulletImpacts( mImpacts, mPlayer );
	{
		PROFILE_SCOPE( "World.effects" );
		mEffects.update( &mGround ); // 千切れた部位が落ちる先は地面だけ（建物・車の上には乗らず、抜ける）
	}
	{
		PROFILE_SCOPE( "World.zombies" );
		updateZombies();
	}
	// 動いた後の物に付いて動く部品（銃口の光。撃ったティックの描画が光り始めになる）。
	mObjects.update( UpdatePhase::Late );
	// このティックで消す印の付いたものを、全員の更新が済んでから消す（名簿からも外れる）。
	mObjects.flush();
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
	mCombat.resolveZombieAttacks( mPlayer );
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

InteractionSystem& GameWorld::interaction() {
	return mInteraction;
}

const InteractionSystem& GameWorld::interaction() const {
	return mInteraction;
}

const CombatResolver& GameWorld::combat() const {
	return mCombat;
}

EffectManager& GameWorld::effects() {
	return mEffects;
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

const std::vector< Building >& GameWorld::buildings() const {
	return mBuildings;
}

const BulletList& GameWorld::bullets() const {
	return mBullets;
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

const ZombieSpawner* GameWorld::zombieSpawner() const {
	return mZombieSpawner.get();
}

bool GameWorld::isPlayerDead() const {
	const PlayerHealthComponent* health = mPlayer ? mPlayer->getComponent< PlayerHealthComponent >() : nullptr;
	return health && health->isDead(); // 体力の部品を外した自キャラは死なない
}
