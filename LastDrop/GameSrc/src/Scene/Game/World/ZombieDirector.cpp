#include "GameSrc/Scene/Game/World/ZombieDirector.h"
#include "GameSrc/Render/MeshCache.h"
#include "GameSrc/Map/TownMap.h"
#include "GameSrc/Map/TownVisibility.h"
#include "GameSrc/Component/TransformComponent.h"
#include "GameSrc/Component/CharacterColliderComponent.h"
#include "GameSrc/Player/PlayerGrabComponent.h"
#include "GameSrc/Player/PlayerHealthComponent.h"
#include "GameSrc/Player/Body/FirstPersonBodyComponent.h"
#include "GameSrc/Zombie/Component/ZombieStateComponent.h"
#include "GameSrc/Zombie/Component/ZombieAttackComponent.h"
#include "GameSrc/Zombie/Move/ZombieCrowdParams.h"
#include "GameSrc/World/GameObject.h"
#include "GameSrc/World/GameObjectWorld.h"
#include "GameSrc/Raid/DayCondition.h"

#include "GameLib/GameLib.h"
#include "GameLib/Base/DebugStream.h"
#include "GameLib/Profiling/Profiler.h"
#include "Framework.h"

#include <cmath>
#include <iterator>

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const char* kZombieFile = "Resource/Model/Character/zombie.txt";
	const char* kZombieAnimation = "walk";
	const char* kZombieRunAnimation = "run";		// 走る体が追う・寄るときのアニメ
	const float kAnimationStartStep = 11.f;			// 体ごとに歩き出しをずらす（フレーム）
	// 倒れるときのアニメ
	const char* const kZombieDeathAnimations[] = { "death1", "death2" };
	const int kZombieDeathAnimationNumber = sizeof( kZombieDeathAnimations ) / sizeof( kZombieDeathAnimations[ 0 ] );
	const char* kZombieDownedAnimation = "death1";	// 脚を失って倒れるときのアニメ。前へ倒れてうつ伏せになる方（death2 は仰向けに倒れるので、体を引きずれない
	const char* kZombieStaggerAnimation = "gethit";	// 撃たれてよろけるときのアニメ
	const char* kZombieIdleAnimation = "idle1";		// 気付く前・追いついた・見失って探し終えたときに立つアニメ
	// 立っている体の攻撃の種類。ここを替えれば、流す攻撃のアニメも一緒に替わる
	// つかみも消さずに残してある（Grab にすれば、つかむ → 噛むになる）
	const ZombieAttackStyle kZombieAttackStyle = ZombieAttackStyle::Swing;
	const char* kZombieSwingAnimation = "atack1";	// 右手を 1 回振り下ろす
	const char* kZombieGrabAnimation = "atack3";	// 両手でつかんで噛む
	const char* kZombieAttackAnimation =
		( kZombieAttackStyle == ZombieAttackStyle::Swing ) ? kZombieSwingAnimation : kZombieGrabAnimation;
	// ゾンビの形が読めないときの、ナビメッシュの床の縁を壁から離す長さ（m）。ゾンビは出ないので床は使われないが、作るのに要る
	const float kNoZombieNavMeshRadius = 0.4f;
}

ZombieDirector::ZombieDirector( MeshCache* meshes, GameObjectWorld* objects, const HitWorld* hitWorld, const CollisionWorld* collisionWorld,
	const DayConditionEffect* day ) :
	mObjects( objects ),
	mHitWorld( hitWorld ),
	mCollisionWorld( collisionWorld ),
	mDay( day ),
	mZombieData( nullptr ),
	mHearing( objects ),
	mZombieNumber( 0 ) {
	ASSERT( meshes && objects && hitWorld && collisionWorld && day && "ZombieDirector : an argument is null." );
	mZombieData = meshes->skinnedMesh( kZombieFile );
	mContext.hitWorld = mHitWorld;
	mContext.crowd = &mCrowd;
}

// ZombieFactory などの定義が見える所で破棄する
ZombieDirector::~ZombieDirector() {
}

void ZombieDirector::provideShared() {
	// 世界に 1 つの物を共有の棚へ置く（部品が付けるときの引数ではなく、ここから引く）
	mObjects->provide( &mPathBudget );
	mObjects->provide( &mContext );
	mObjects->provide( &mHearing );
}

bool ZombieDirector::prepareModel() {
	if ( mZombieModel ) {
		return true;
	}
	if ( !mZombieData->isReady() ) {
		return false;
	}
	// 根の動きを置き場所へ移すのは、1回だけ流すアニメ（攻撃・よろけ・倒れる）
	std::vector< const char* > rootMotionAnimations( std::begin( kZombieDeathAnimations ), std::end( kZombieDeathAnimations ) );
	rootMotionAnimations.push_back( kZombieStaggerAnimation );
	rootMotionAnimations.push_back( kZombieAttackAnimation );
	mZombieModel = std::make_unique< ZombieModel >( mZombieData, kZombieAnimation, kZombieRunAnimation, rootMotionAnimations );
	return true;
}

float ZombieDirector::navMeshRadius() {
	if ( !prepareModel() ) {
		return kNoZombieNavMeshRadius;
	}
	float reach = mZombieModel->footprint().reachRadius();
	float radius = reach + zombieCrowdParams().navMeshClearance;
	GameLib::cout << "ZombieFootprint: reach " << reach << " m -> navmesh radius " << radius << " m" << GameLib::endl;
	return radius;
}

float ZombieDirector::defaultNavMeshRadius() const {
	if ( !mZombieData->isReady() ) {
		return kNoZombieNavMeshRadius; // navMeshRadius が設計図を作れないのと同じ条件
	}
	// 設計図の長方形は作ったときの F6 の余白なので使わず、初期値の余白で求め直す。navMeshRadius と同じ足し方にする
	const ZombieCrowdParams defaults{};
	const float reach = ZombieModel::makeFootprint( mZombieData, defaults.footprintPadding ).reachRadius();
	return reach + defaults.navMeshClearance;
}

void ZombieDirector::setup( const TownMap& map ) {
	if ( !prepareModel() ) {
		return;
	}
	mFactory = std::make_unique< ZombieFactory >( mZombieModel.get(), kZombieAttackStyle );
	mRandom = Random::create( static_cast< int >( Framework::instance().time() ) );	// 遊ぶたびに違う倒れ方にする（起動からの時間を種にする。ガソリンの場所と同じ）
	// 歩くゾンビは、最初の update から ZombieSpawner が見えない所へ湧かせる（自キャラの目とカメラの向きが分かってから選ぶため）
	std::vector< Vector3 > spawnPoints;
	for ( const TownPlacement& p : map.zombieSpawnPoints() ) {
		spawnPoints.push_back( p.position() );
	}
	mSpawner = std::make_unique< ZombieSpawner >( spawnPoints, mHitWorld, mCollisionWorld,
		&mZombieModel->footprint(), mRandom.getInt(), mDay->zombieCount );
}

void ZombieDirector::onDayConditionChanged() {
	if ( mSpawner ) {
		mSpawner->setTargetCount( mDay->zombieCount );
	}
}

bool ZombieDirector::spawn( const Vector3& position, float yawDegree ) {
	// 同時に居られるゾンビの数（死体も数える）は、今日の条件で決まる（群れの日は多い）
	if ( !mFactory || mObjects->count< ZombieBrainComponent >() >= mDay->zombieCount ) {
		return false;
	}
	ZombieAnimationNames animations;
	animations.move = kZombieAnimation;
	animations.run = kZombieRunAnimation;
	animations.moveStartTime = kAnimationStartStep * mZombieNumber;
	animations.idle = kZombieIdleAnimation;
	animations.death = kZombieDeathAnimations[ mRandom.getInt( kZombieDeathAnimationNumber ) ];
	animations.downed = kZombieDownedAnimation;
	animations.stagger = kZombieStaggerAnimation;
	animations.attack = kZombieAttackAnimation;
	// 何体目か（mZombieNumber）で見通しを調べる時刻をずらす。うろつき方の乱数の種も体ごとに変える
	// （同じ種だと全員が同じ向き・同じ間で歩き出す）
	mFactory->spawn( mObjects, position, yawDegree, animations, mZombieNumber, mRandom.getInt() );
	++mZombieNumber;
	return true;
}

void ZombieDirector::beginTick() {
	mPathBudget.beginTick( static_cast< int >( std::lround( zombieCrowdParams().pathSearchesPerTick ) ) );
}

void ZombieDirector::setAnimationView( const Matrix44& projectionView, const Vector3& eye, const TownVisibility* visibility ) {
	mAnimationView.emplace( projectionView, eye, visibility, visibility ? visibility->cellAt( eye ) : TownVisibility::kNoCell );
	mContext.view = &*mAnimationView;
}

void ZombieDirector::prepareThink( const GameObject* player ) {
	// ゾンビが追う先は自キャラ（このティックに動いた後の位置）。自キャラが居なければ追わない
	mContext.target = nullptr;
	if ( player ) {
		const PlayerHealthComponent* health = player->getComponent< PlayerHealthComponent >();
		mChaseTarget.position = player->getComponent< TransformComponent >()->position();
		mChaseTarget.eyePosition = player->getComponent< FirstPersonBodyComponent >()->eyePosition();
		// 体力の部品を外した自キャラ（死なない）も襲う。つかまれている間・離された直後は、ほかのゾンビもつかまない
		// つかまれる部品（PlayerGrabComponent）を外した自キャラは襲わない
		const PlayerGrabComponent* grab = player->getComponent< PlayerGrabComponent >();
		mChaseTarget.attackable = ( !health || health->canBeAttacked() ) && grab && grab->canBeGrabbed();
		mChaseTarget.attackTaken = false; // 体ごとに決め直す（ZombieBrainComponent::update）
		mChaseTarget.body = player->getComponent< CharacterColliderComponent >(); // 外していれば nullptr
		mContext.target = &mChaseTarget;
	}
	// 攻撃に行っているゾンビ（前のティックから続いている 1 体）。Think の段の途中で攻撃を始めた体も、そこへ入って
	// 後の体が攻撃を始めない（同じティックに 2 体が攻撃を始めないように）
	mContext.attacker = nullptr;
	mObjects->forEach< ZombieAttackComponent >( [ this ]( const ZombieAttackComponent& attack ) {
		if ( !mContext.attacker && attack.isAttacking() ) {
			mContext.attacker = attack.gameObject();
		}
	} );
	// 立って歩ける体の位置（このティックに動く前）。近くの仲間を避けるのに全員が読む（倒れた体・這う体・死体は数えない）
	mCrowd.clear();
	mObjects->forEach< ZombieStateComponent >( [ this ]( const ZombieStateComponent& zombie ) {
		if ( zombie.state() == ZombieStateComponent::State::Alive ) {
			mCrowd.add( zombie.gameObject(), zombie.bodyCenter() );
		}
	} );
}

void ZombieDirector::finishThink( GameObject* player ) {
	PlayerGrabComponent* grab = player ? player->getComponent< PlayerGrabComponent >() : nullptr; // 外せる部品
	if ( !grab ) {
		return;
	}
	bool held = false;
	mObjects->forEach< ZombieAttackComponent >( [ &held ]( const ZombieAttackComponent& attack ) {
		held = held || attack.isHoldingTarget();
	} );
	grab->setHeld( held );
}

void ZombieDirector::releaseTarget( GameObject* player ) {
	PlayerGrabComponent* grab = player ? player->getComponent< PlayerGrabComponent >() : nullptr; // 外せる部品
	if ( grab ) {
		grab->setHeld( false );
	}
}

void ZombieDirector::spawnFromSpawner( const GameObject* player, float cameraYaw ) {
	if ( !mSpawner || !player ) {
		return; // ゾンビの形が無い・見えない所を決める自キャラが居ない
	}
	ZombieSpawner::View view;
	view.playerEye = player->getComponent< FirstPersonBodyComponent >()->eyePosition();
	view.viewYaw = cameraYaw;
	view.standingZombies = 0;
	mObjects->forEach< ZombieStateComponent >( [ &view ]( const ZombieStateComponent& zombie ) {
		if ( zombie.state() == ZombieStateComponent::State::Alive ) {
			++view.standingZombies;
		}
	} );
	view.hasRoom = mObjects->count< ZombieBrainComponent >() < mDay->zombieCount;
	mSpawnOrders.clear();
	mSpawner->update( view, &mSpawnOrders );
	for ( const ZombieSpawner::Order& order : mSpawnOrders ) {
		PROFILE_EVENT( "ZombieSpawn" ); // 計測の印（湧いた体ごと）
		spawn( order.position, order.yawDegree );
	}
}

ZombieHearing& ZombieDirector::hearing() {
	return mHearing;
}

const ZombieSpawner* ZombieDirector::spawner() const {
	return mSpawner.get();
}

bool& zombiesFrozen() {
	static bool sFrozen = false; // 起動中ずっと 1 つ（F6 で書き換える）
	return sFrozen;
}
