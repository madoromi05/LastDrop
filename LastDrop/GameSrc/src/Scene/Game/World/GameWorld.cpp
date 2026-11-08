#include "GameSrc/Scene/Game/World/GameWorld.h"
#include "GameSrc/Render/MeshCache.h"
#include "GameSrc/Map/TownCache.h"
#include "GameSrc/Component/TransformComponent.h"
#include "GameSrc/Component/HitboxComponent.h"
#include "GameSrc/Component/CharacterColliderComponent.h"
#include "GameSrc/Player/PlayerComponent.h"
#include "GameSrc/Player/Body/FirstPersonBodyComponent.h"
#include "GameSrc/Player/Body/HoldComponent.h"
#include "GameSrc/Player/PlayerHealthComponent.h"
#include "GameSrc/Player/PlayerLocomotionComponent.h"
#include "GameSrc/Core/GameTime.h"
#include "GameSrc/Math/FlatMath.h"
#include "GameSrc/Weapon/GunComponent.h"
#include "GameSrc/Zombie/Component/ZombieStateComponent.h"
#include "GameSrc/Zombie/Component/ZombieAttackComponent.h"
#include "GameSrc/Zombie/Component/ZombieCorpseComponent.h"
#include "GameSrc/Zombie/Move/ZombieCrowdParams.h"

#include "GameLib/GameLib.h"
#include "Framework.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Random.h"
#include "GameLib/Math/Functions.h"
#include "GameLib/Profiling/Profiler.h"

#include <cmath>
#include <iterator>

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// 置き場所はどれも TownMap が決める。ここにあるのは、何を置くか（ファイル）と、置き場所以外の決めごとだけ。
	// 建物・置物は WorldBuilder・PropFactory、血の量は EffectManager、倒したゾンビを漁れる場所にするのは CombatResolver が持つ。
	const char* kBulletFile = "Resource/Model/Weapon/bullet_tip.txt";

	const char* kZombieFile = "Resource/Model/Character/zombie.txt";
	const char* kZombieAnimation = "walk";
	const char* kZombieRunAnimation = "run"; // 走る体が追う・寄るときのアニメ（R4_Plan.txt）
	const float kAnimationStartStep = 11.f; // 体ごとに歩き出しをずらす（フレーム）
	// 倒れるときのアニメ
	const char* const kZombieDeathAnimations[] = { "death1", "death2" };
	const int kZombieDeathAnimationNumber = sizeof( kZombieDeathAnimations ) / sizeof( kZombieDeathAnimations[ 0 ] );
	// 脚を失って倒れるときのアニメ。前へ倒れてうつ伏せになる方（death2 は仰向けに倒れるので、体を引きずれない。
	const char* kZombieDownedAnimation = "death1";
	const float kGunshotHearingRange = 30.f;				// 銃声が届く距離（m。撃った所から水平だけ
	// 足音が届く距離（m。自キャラの足元から水平だけ。壁では切らない。SummaryPlan/個別仕様/ZombieFootstepHearing_Plan.txt）。
	// 走りは横・後ろの目（10m）より遠くまで届き、歩きはそれより短いので、そっと歩けば後ろを抜けられる。
	const float kWalkFootstepHearingRange = 5.f;
	const float kRunFootstepHearingRange = 18.f;
	// 足音を知らせる間隔（秒。ゾンビが目の線を引く間隔と同じ。毎ティック全員を調べないため）。
	const float kFootstepNoiseIntervalSeconds = 0.3f;
	const int kFootstepNoiseIntervalTicks = GameTime::secondsToTicks( kFootstepNoiseIntervalSeconds );

	// 歩き方 gait の足音が届く距離（m。止まっていれば 0）。
	float footstepHearingRange( Gait gait ) {
		switch ( gait ) {
			case GAIT_WALK: return kWalkFootstepHearingRange;
			case GAIT_RUN:  return kRunFootstepHearingRange;
			default:        return 0.f;
		}
	}
	const char* kZombieStaggerAnimation = "gethit";			// 撃たれてよろけるときのアニメ
	const char* kZombieIdleAnimation = "idle1";				// 気付く前・追いついた・見失って探し終えたときに立つアニメ
	const char* kZombieAttackAnimation = "atack3";			// 追いついたら、つかんで噛む攻撃のアニメ

	// カバンから捨てた物を置く、自キャラの足元から向いている方への長さ（m）。
	// ガソリンの缶は移動を止める箱を持つので、体（半径 約 0.4m）と缶の箱が重ならず、目の前に見える所。
	const float kDropForward = 0.8f;

	Vector3 toPosition( const TownPlacement& placement ) {
		return Vector3( placement.x, placement.y, placement.z );
	}
}

GameWorld::GameWorld( MeshCache* meshes, TownCache* town, const DayConditionEffect& day ) :
	mDay( day ),
	mTown( town ),
	mNavMesh( nullptr ),
	mEscapeCarSpot( 0 ),
	mPlayerFactory( meshes ),
	mPropFactory( meshes ),
	mBuilder( meshes, &mMap, &mPropFactory ),
	mBulletData( meshes->staticMesh( kBulletFile ) ), // mBullets が借りるので、ここで頼む
	mZombieData( meshes->skinnedMesh( kZombieFile ) ),
	mGround( meshes ),
	mBullets( mBulletData ),
	mObjects( &mCollisionWorld, &mHitWorld ),
	mCombat( &mObjects, &mEffects, &mHitWorld ),
	mInteraction( &mHitWorld, &mObjects, &mEffects ),
	mPlayer( nullptr ),
	mPlayerControl( nullptr ),
	mZombieNumber( 0 ),
	mFootstepNoiseTicks( 0 ),
	mVisibility( nullptr ) {
	ASSERT( meshes && town && "GameWorld : an argument is null." );
	mZombieContext.hitWorld = &mHitWorld;
	mZombieContext.crowd = &mCrowd;
	// 世界に 1 つの物を共有の棚へ置く（部品が付けるときの引数ではなく、ここから引く。SummaryPlan/個別仕様/ComponentSystem3_Plan.txt の B）。
	// 歩ける床（NavMesh）は buildNavMesh で TownCache から借りたときに置く（ゾンビを置くのはその後の placeActors）。
	mObjects.provide( &mPathBudget );
	mObjects.provide( &mZombieContext );
}

// ZombieFactory などの定義が見える所で破棄する。
GameWorld::~GameWorld() {
}

void GameWorld::buildTown( int escapeCarSpot ) {
	LOAD_SCOPE( "town", "WorldBuilder::build" );
	mEscapeCarSpot = escapeCarSpot;
	WorldBuildTarget target;
	target.ground = &mGround;
	target.buildings = &mBuildings;
	target.collisionWorld = &mCollisionWorld;
	target.hitWorld = &mHitWorld;
	target.objects = &mObjects;
	mBuilder.build( target, escapeCarSpot );
}

void GameWorld::buildNavMesh() {
	// 建物・車・ゴミ箱・木箱を名簿へ載せ終えた形を材料にする（ZombieNavMesh_Plan.txt）。2 回目からの出撃は作らずに借りる。
	mNavMesh = mTown->navMesh( mEscapeCarSpot, mCollisionWorld );
	mObjects.provide( mNavMesh );
}

void GameWorld::buildVisibility() {
	LOAD_SCOPE( "town", "TownVisibility" );
	mVisibility = mTown->visibility( mBuildings, mHitWorld );
}

void GameWorld::placeActors() {
	LOAD_SCOPE( "town", "GameWorld::placeActors" );
	if ( mPlayerFactory.isReady() ) {
		placePlayer();
	}
	if ( mZombieData->isReady() ) {
		// 根の動きを置き場所へ移すのは、1回だけ流すアニメ（攻撃・よろけ・倒れる。SummaryPlan/個別仕様/ZombieRootMotion_Plan.txt）。
		std::vector< const char* > rootMotionAnimations( std::begin( kZombieDeathAnimations ), std::end( kZombieDeathAnimations ) );
		rootMotionAnimations.push_back( kZombieStaggerAnimation );
		rootMotionAnimations.push_back( kZombieAttackAnimation );
		mZombieModel = std::make_unique< ZombieModel >( mZombieData, kZombieAnimation, kZombieRunAnimation, rootMotionAnimations );
		mZombieFactory = std::make_unique< ZombieFactory >( mZombieModel.get() );
		// 遊ぶたびに違う倒れ方にする（起動からの時間を種にする。ガソリンの場所と同じ）。
		mZombieRandom = Random::create( static_cast< int >( Framework::instance().time() ) );
		// 歩くゾンビは、最初の update から ZombieSpawner が見えない所へ湧かせる（自キャラの目とカメラの向きが分かってから選ぶため）。
		std::vector< Vector3 > spawnPoints;
		for ( const TownPlacement& p : mMap.zombieSpawnPoints() ) {
			spawnPoints.push_back( toPosition( p ) );
		}
		mZombieSpawner = std::make_unique< ZombieSpawner >( spawnPoints, &mHitWorld, &mCollisionWorld,
			&mZombieModel->footprint(), mZombieRandom.getInt(), mDay.zombieCount );
	}
}

void GameWorld::alertZombiesToNoise( const Vector3& position ) {
	alertZombiesToGunshot( position ); // 届く距離・寄って来方は銃声と同じ
}

void GameWorld::placePlayer() {
	const TownPlacement& start = mMap.playerStart();
	mPlayer = mPlayerFactory.spawn( &mObjects, toPosition( start ), start.yawDegree );
	mPlayerControl = mPlayer->getComponent< PlayerComponent >();
}

void GameWorld::setAnimationView( const Matrix44& projectionView, const Vector3& eye ) {
	// 隠れの表は F6 の PVS を切っている間は渡さない（描く側と同じ。描かない体を「映らない」にそろえる）。
	const TownVisibility* visibility = ( pvsEnabled() && mVisibility && mVisibility->isReady() ) ? mVisibility : nullptr;
	mAnimationView.emplace( projectionView, eye, visibility, visibility ? visibility->cellAt( eye ) : TownVisibility::kNoCell );
	mZombieContext.view = &*mAnimationView;
}

bool GameWorld::spawnZombie( const Vector3& position, float yawDegree ) {
	// 同時に居られるゾンビの数（死体も数える）は、今日の条件で決まる（群れの日は多い）。
	if ( !mZombieFactory || mObjects.count< ZombieBrainComponent >() >= mDay.zombieCount ) {
		return false;
	}
	ZombieAnimationNames animations;
	animations.move = kZombieAnimation;
	animations.run = kZombieRunAnimation;
	animations.moveStartTime = kAnimationStartStep * mZombieNumber;
	animations.idle = kZombieIdleAnimation;
	animations.death = kZombieDeathAnimations[ mZombieRandom.getInt( kZombieDeathAnimationNumber ) ];
	animations.downed = kZombieDownedAnimation;
	animations.stagger = kZombieStaggerAnimation;
	animations.attack = kZombieAttackAnimation;
	// 何体目か（mZombieNumber）で見通しを調べる時刻をずらす。うろつき方の乱数の種も体ごとに変える
	// （同じ種だと全員が同じ向き・同じ間で歩き出す）。
	// 2026-11-06 の「T ポーズのゾンビを削除」で、この組み立ての行（前は addZombie）も一緒に消えて、ゾンビが湧かなくなっていた。
	mZombieFactory->spawn( &mObjects, position, yawDegree, animations, mZombieNumber, mZombieRandom.getInt() );
	++mZombieNumber;
	return true;
}

bool GameWorld::dropItem( const ItemStack& stack, float cameraYaw ) {
	if ( !mPlayer ) {
		return false;
	}
	Vector3 position = mPlayer->getComponent< TransformComponent >()->position();
	Vector3 forward( GameLib::Math::sin( cameraYaw ), 0.f, GameLib::Math::cos( cameraYaw ) );
	position.madd( forward, kDropForward );
	return mPropFactory.spawnDroppedItem( &mObjects, stack, position, cameraYaw ) != nullptr;
}

void GameWorld::update( const PlayerCommand& command, float cameraYaw, float cameraPitch, const Vector3& viewDirection,
	Inventory* inventory ) {
	{
		PROFILE_SCOPE( "World.prepare" );
		// 動く体を今の置き場所でますへ入れ直させる（体が動く前に 1 回。OptimizationB_Plan.txt 7 章）。
		mCollisionWorld.refreshBodies();
		// このティックに道を探してよい回数を戻す（OptimizationC_Plan.txt 6 章）。
		mPathBudget.beginTick( static_cast< int >( std::lround( zombieCrowdParams().pathSearchesPerTick ) ) );
	}
	{
		PROFILE_SCOPE( "World.player" );
		if ( mPlayerControl ) {
			mPlayerControl->setInput( command, cameraYaw, cameraPitch, inventory );
		}
		mObjects.update( UpdatePhase::Input );
	}
	// 弾は銃口ではなく目から出す。画面の中央（照準）へ飛び、銃口が壁にめり込んでいても壁の向こうへ撃てないようにするため。
	{
		PROFILE_SCOPE( "World.alert" ); // 弾を出す所も含む（撃ったティックだけ。銃声で寄せる処理と分けられないため）
		const GunComponent* gun = HoldComponent::heldGun( mPlayer ); // 素手なら nullptr
		if ( gun && gun->firedThisTick() ) {
			// 計測の印。押した瞬間（Fire）ではなく、実際に弾が出たティック（連射の 2 発目以降も）。
			PROFILE_EVENT( "Shot" );
			mBullets.fire( mPlayer->getComponent< FirstPersonBodyComponent >()->eyePosition(), viewDirection, gun->spec().damage );
			alertZombiesToGunshot( mPlayer->getComponent< TransformComponent >()->position() );
		}
		alertZombiesToFootsteps();
	}
	// 撃ったティックのうちに1回進める（出たばかりの弾が、目の前に止まって見えないように）。
	mImpacts.clear();
	mCombat.beginTick();
	{
		PROFILE_SCOPE( "World.bullets" );
		mBullets.update( mHitWorld, playerTarget(), &mImpacts );
	}
	{
		PROFILE_SCOPE( "World.combat" );
		mCombat.resolveBulletImpacts( mImpacts );
	}
	{
		PROFILE_SCOPE( "World.effects" );
		mEffects.update( &mGround ); // 千切れた部位が落ちる先は地面だけ（建物・車の上には乗らず、抜ける）
	}
	{
		PROFILE_SCOPE( "World.zombies" );
		updateZombies();
	}
	{
		PROFILE_SCOPE( "World.late" );
		// 動いた後の物に付いて動く部品（銃口の光。撃ったティックの描画が光り始めになる）。
		mObjects.update( UpdatePhase::Late );
		// このティックで消す印の付いたものを、全員の更新が済んでから消す（名簿からも外れる）。
		mObjects.flush();
	}
	{
		PROFILE_SCOPE( "World.spawn" );
		// 物の並びをたどり終えてから増やす（たどっている最中に spawn すると並びが動く）。
		spawnZombiesFromSpawner( cameraYaw );
	}
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

void GameWorld::alertZombiesToFootsteps() {
	if ( !mPlayer || --mFootstepNoiseTicks > 0 ) {
		return;
	}
	mFootstepNoiseTicks = kFootstepNoiseIntervalTicks;
	// 雨の日は届きにくい（今日の条件の倍率。DayCondition_Plan.txt）。
	float range = footstepHearingRange( mPlayer->getComponent< PlayerLocomotionComponent >()->gait() ) * mDay.footstepHearingScale;
	if ( range <= 0.f ) {
		return; // 止まっている（つかまれている間も止まっている）
	}
	const Vector3& feet = mPlayer->getComponent< TransformComponent >()->position();
	mObjects.forEach< ZombieStateComponent >( [ &feet, range ]( ZombieStateComponent& zombie ) {
		if ( FlatMath::distance( zombie.position(), feet ) <= range ) {
			zombie.getComponent< ZombieBrainComponent >()->hearFootsteps(); // 聞くかどうかはゾンビが決める（銃声と同じ）
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
		// 体力の部品を外した自キャラ（死なない）も襲う。つかまれている間・離された直後は、ほかのゾンビもつかまない。
		mChaseTarget.attackable = ( !health || health->canBeAttacked() ) && mPlayer->getComponent< PlayerComponent >()->canBeGrabbed();
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
	// つかんでいるゾンビが倒れた・振りほどかれた・噛み終えたら、次の update から動ける。
	if ( mPlayer ) {
		bool held = false;
		mObjects.forEach< ZombieAttackComponent >( [ &held ]( const ZombieAttackComponent& attack ) {
			held = held || attack.isHoldingTarget();
		} );
		mPlayer->getComponent< PlayerComponent >()->setHeld( held );
	}
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
	view.hasRoom = mObjects.count< ZombieBrainComponent >() < mDay.zombieCount;
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

const DayConditionEffect& GameWorld::dayCondition() const {
	return mDay;
}

const NavMesh& GameWorld::navMesh() const {
	ASSERT( mNavMesh && "GameWorld::navMesh : call buildNavMesh first." );
	return *mNavMesh;
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

const TownVisibility& GameWorld::visibility() const {
	ASSERT( mVisibility && "GameWorld::visibility : call buildVisibility first." );
	return *mVisibility;
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

int GameWorld::magazineRounds( ItemId item ) const {
	int rounds = 0;
	mObjects.forEach< GunComponent >( [ item, &rounds ]( const GunComponent& gun ) {
		if ( gun.spec().item == item ) {
			rounds = gun.magazineRounds();
		}
	} );
	return rounds;
}

void GameWorld::setMagazineRounds( ItemId item, int rounds ) {
	mObjects.forEach< GunComponent >( [ item, rounds ]( GunComponent& gun ) {
		if ( gun.spec().item == item ) {
			gun.setMagazineRounds( rounds );
		}
	} );
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
