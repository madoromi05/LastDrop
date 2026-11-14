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
#include "GameSrc/Weapon/GunComponent.h"
#include "GameSrc/Math/AngleMath.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Functions.h"
#include "GameLib/Profiling/Profiler.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// 置き場所はどれも TownMap が決める。ここにあるのは、何を置くか（ファイル）と、置き場所以外の決めごとだけ
	// 建物・置物は TownEnvironment（WorldBuilder）・PropFactory、ゾンビは ZombieDirector、血の量は EffectManager、
	// 倒したゾンビを漁れる場所にするのは CombatResolver が持つ
	const char* kBulletFile = "Resource/Model/Weapon/bullet_tip.txt";

	// カバンから捨てた物を置く、自キャラの足元から向いている方への長さ（m）
	// ガソリンの缶は移動を止める箱を持つので、体（半径 約 0.4m）と缶の箱が重ならず、目の前に見える所
	const float kDropForward = 0.8f;

	// 街灯のガラスを光る形で置くか（停電の日＝街灯の明るさ 0 は消えた形）
	bool lampsLit( const DayConditionEffect& day ) {
		const float kLightsOut = 0.f;
		return day.streetLightScale > kLightsOut;
	}
}

GameWorld::GameWorld( MeshCache* meshes, TownCache* town, DayCondition condition, const ItemGrid& startBag ) :
	mCondition( condition ),
	mDay( dayConditionEffect( condition ) ),
	mStartBag( startBag ),
	mPlayerFactory( meshes ),
	mPropFactory( meshes ),
	mTown( meshes, town, &mPropFactory ),
	mBulletData( meshes->staticMesh( kBulletFile ) ), // mBullets が借りるので、ここで頼む
	mBullets( mBulletData ),
	mZombies( meshes, &mObjects, &mHitWorld, &mCollisionWorld, &mDay ), // mObjects は覚えるだけ（使うのは provideShared から）
	mObjects( &mCollisionWorld, &mHitWorld ),
	mCombat( &mObjects, &mEffects, &mHitWorld ),
	mInteraction( &mHitWorld, &mObjects, &mEffects ),
	mPlayer( nullptr ),
	mPlayerControl( nullptr ) {
	ASSERT( meshes && town && "GameWorld : an argument is null." );
	// ゾンビの部品が共有の棚から引く物（TickContext・道探しの回数・音を知らせる係）を置く
	// 歩ける床（NavMesh）は buildNavMesh で TownCache から借りたときに置く（ゾンビを置くのはその後の placeActors）
	mZombies.provideShared();
	mObjects.provide< IInteractFinder >( &mInteraction ); // 自キャラの「見ている物を使う」部品が、使える物を探すのに引く
}

// 係の定義が見える所で破棄する
GameWorld::~GameWorld() {
}

void GameWorld::buildTown( int escapeCarSpot ) {
	mTown.build( &mCollisionWorld, &mHitWorld, &mObjects, escapeCarSpot, lampsLit( mDay ) );
}

void GameWorld::buildNavMesh() {
	mObjects.provide( mTown.buildNavMesh( mCollisionWorld ) );
}

void GameWorld::buildVisibility() {
	mTown.buildVisibility( mHitWorld );
}

void GameWorld::placeActors( int escapeCarSpot ) {
	LOAD_SCOPE( "town", "GameWorld::placeActors" );
	if ( mPlayerFactory.isReady() ) {
		placePlayer( escapeCarSpot );
	}
	mZombies.setup( mTown.map() );
}

void GameWorld::alertZombiesToNoise( const Vector3& position ) {
	mZombies.hearing().hearNoise( position );
}

void GameWorld::placePlayer( int escapeCarSpot ) {
	const TownPlacement& start = mTown.map().escapeCarSpots()[ escapeCarSpot ].playerStart;
	mPlayer = mPlayerFactory.spawn( &mObjects, start.position(), start.yawDegree, mStartBag );
	mPlayerControl = mPlayer->getComponent< PlayerComponent >();
}

void GameWorld::setAnimationView( const Matrix44& projectionView, const Vector3& eye ) {
	mZombies.setAnimationView( projectionView, eye, mTown.visibilityIfEnabled() );	// 隠れの表は F6 の PVS を切っている間は渡さない（描く側と同じ。描かない体を「映らない」にそろえる）
}

bool GameWorld::spawnZombie( const Vector3& position, float yawDegree ) {
	return mZombies.spawn( position, yawDegree );
}

bool GameWorld::dropItem( const ItemStack& stack, float cameraYaw ) {
	if ( !mPlayer ) {
		return false;
	}
	Vector3 position = mPlayer->getComponent< TransformComponent >()->position();
	position.madd( AngleMath::forwardOf( cameraYaw ), kDropForward );
	return mPropFactory.spawnDroppedItem( &mObjects, stack, position, cameraYaw ) != nullptr;
}

void GameWorld::update( const PlayerCommand& command, float cameraYaw, float cameraPitch, const Vector3& viewDirection ) {
	{
		PROFILE_SCOPE( "World.prepare" );
		mCollisionWorld.refreshBodies();	// 動く体を今の置き場所でますへ入れ直させる（体が動く前に 1 回）
		mZombies.beginTick();				// このティックに道を探してよい回数を戻す
	}
	{
		PROFILE_SCOPE( "World.player" );
		if ( mPlayerControl ) {
			mPlayerControl->setInput( command, cameraYaw, cameraPitch );
		}
		mObjects.update( UpdatePhase::Input );	// 自キャラが動いて撃ち、その後に足音・銃声をゾンビへ知らせる（PlayerNoiseComponent。同じ段の中で PlayerComponent の後）
	}
	// 弾は銃口ではなく目から出す。画面の中央（照準）へ飛び、銃口が壁にめり込んでいても壁の向こうへ撃てないようにするため
	{
		PROFILE_SCOPE( "World.alert" ); // 弾を出す所（撃ったティックだけ。計測の名前は、前の計測の CSV と列を揃えるため alert のまま）
		const GunComponent* gun = HoldComponent::heldGun( mPlayer ); // 素手なら nullptr
		if ( gun && gun->firedThisTick() ) {
			// 計測の印。押した瞬間（Fire）ではなく、実際に弾が出たティック（連射の 2 発目以降も）
			PROFILE_EVENT( "Shot" );
			mBullets.fire( mPlayer->getComponent< FirstPersonBodyComponent >()->eyePosition(), viewDirection, gun->spec().damage );
		}
	}
	// 撃ったティックのうちに1回進める（出たばかりの弾が、目の前に止まって見えないように）
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
		mEffects.update(); // 弾痕・砂ぼこり・血。千切れた部位は物なので、Late の段で自分で進む
	}
	{
		PROFILE_SCOPE( "World.zombies" );
		if ( zombiesFrozen() ) {
			mZombies.releaseTarget( mPlayer );	// F6 でゾンビを止めている間は、考える・動く・アニメ・攻撃を全部飛ばす（Think の段はゾンビだけ）
		} else {
			// 攻撃に行けるのは一度に 1 体だけ（ZombieBrainComponent::TickContext::attacker）
			mZombies.prepareThink( mPlayer );
			mObjects.update( UpdatePhase::Think );
			mCombat.resolveZombieAttacks( mPlayer );
			mZombies.finishThink( mPlayer );
		}
	}
	{
		PROFILE_SCOPE( "World.late" );
		mObjects.update( UpdatePhase::Late );	// 動いた後の物に付いて動く部品（銃口の光。撃ったティックの描画が光り始めになる）と、千切れた部位
		mObjects.flush();						// このティックで消す印の付いたもの（沈み終えた死体も）を、全員の更新が済んでから消す（名簿からも外れる）
	}
	{
		PROFILE_SCOPE( "World.spawn" );
		// 物の並びをたどり終えてから増やす（たどっている最中に spawn すると並びが動く）。ゾンビを止めている間は湧かせない
		if ( !zombiesFrozen() ) {
			mZombies.spawnFromSpawner( mPlayer, cameraYaw );
		}
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

const TownEnvironment& GameWorld::town() const {
	return mTown;
}

const ZombieDirector& GameWorld::zombies() const {
	return mZombies;
}

void GameWorld::changeDayCondition( DayCondition condition ) {
	mCondition = condition;
	mDay = dayConditionEffect( condition ); // 街灯の光・霧（SceneLights）・足音（ZombieHearing）・同時に居られる数は、ここを毎回読む
	mZombies.onDayConditionChanged();       // 湧かせる係が作るときに受け取った目標の数を合わせる
	mPropFactory.relightStreetLamps( &mObjects, lampsLit( mDay ) );
}

const DayConditionEffect& GameWorld::dayCondition() const {
	return mDay;
}

DayCondition GameWorld::dayConditionKind() const {
	return mCondition;
}

const CollisionWorld& GameWorld::collisionWorld() const {
	return mCollisionWorld;
}

const HitWorld& GameWorld::hitWorld() const {
	return mHitWorld;
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

const GameObjectWorld& GameWorld::objects() const {
	return mObjects;
}

GameObjectWorld& GameWorld::objects() {
	return mObjects;
}

bool GameWorld::isPlayerDead() const {
	const PlayerHealthComponent* health = mPlayer ? mPlayer->getComponent< PlayerHealthComponent >() : nullptr;
	return health && health->isDead(); // 体力の部品を外した自キャラは死なない
}
