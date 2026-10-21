#include "GameSrc/Scene/Game/GameWorld.h"
#include "GameSrc/Render/MeshCache.h"
#include "GameSrc/Player/PlayerCharacter.h"
#include "GameSrc/Map/SpawnPicker.h"
#include "GameSrc/Hit/DamageInfo.h"
#include "GameSrc/Zombie/ChaseTarget.h"

#include "GameLib/GameLib.h"
#include "Framework.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Random.h"

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
	// 同時に居られるゾンビの数（死体も数える）。これより多くは出さない（追ってくる AI と見通しの線・描画が体の数だけ重くなる）。
	const int kMaxZombies = 30;
	// 倒れるときのアニメ（tools/configs/zombie.json の extra_animations）。体ごとにどれかをランダムで選ぶ（倒れ方が揃わないように）。
	const char* const kZombieDeathAnimations[] = { "death1", "death2" };
	const int kZombieDeathAnimationNumber = sizeof( kZombieDeathAnimations ) / sizeof( kZombieDeathAnimations[ 0 ] );
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
	mZombieNumber( 0 ),
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
		mZombieModel = std::make_unique< ZombieModel >( mZombieData, kZombieAnimation );
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
	if ( !mZombieModel || mZombies.size() >= kMaxZombies ) {
		return false;
	}
	Zombie::Animations animations;
	animations.move = kZombieAnimation;
	animations.moveStartTime = kAnimationStartStep * mZombieNumber;
	animations.idle = kZombieIdleAnimation;
	animations.death = kZombieDeathAnimations[ mZombieRandom.getInt( kZombieDeathAnimationNumber ) ];
	animations.stagger = kZombieStaggerAnimation;
	animations.attack = kZombieAttackAnimation;
	addZombie( position, yawDegree, animations );
	return true;
}

void GameWorld::addZombie( const Vector3& position, float yawDegree, const Zombie::Animations& animations ) {
	// 移動・命中の両方の当たり判定への登録は ActorList が行う。何体目か（mZombieNumber）で見通しを調べる時刻をずらす。
	// うろつき方の乱数の種も体ごとに変える（同じ種だと全員が同じ向き・同じ間で歩き出す）。
	int randomSeed = mZombieRandom.getInt();
	mZombies.spawn( std::make_unique< Zombie >( mZombieModel.get(), &mCollisionWorld, position, yawDegree, animations,
		mZombieNumber, randomSeed ) );
	++mZombieNumber;
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
	// 前に撃った光を先に進めてから、このティックに撃った光を灯す（撃ったティックの描画が光り始めになる）。
	mMuzzleFlash.update();
	if ( mPlayer ) {
		mPlayer->update( command, cameraYaw, cameraPitch );
		// 弾は銃口ではなく目から出す。画面の中央（照準）へ飛び、銃口が壁にめり込んでいても壁の向こうへ撃てないようにするため。
		if ( mPlayer->firedThisTick() ) {
			mBullets.fire( mPlayer->eyePosition(), viewDirection );
			mMuzzleFlash.fire();
		}
	}
	// 撃ったティックのうちに1回進める（出たばかりの弾が、目の前に止まって見えないように）。
	mImpacts.clear();
	mZombieHits.clear();
	mZombieAttacks.clear();
	mBullets.update( mHitWorld, mPlayer.get(), &mImpacts );
	// 物（建物・車・地面・拾える物）に当たった所にだけ、弾痕と砂ぼこりを出す。ゾンビ（target）には出さず、血しぶきを出す。
	for ( const BulletList::Impact& impact : mImpacts ) {
		const HitWorld::Hit& hit = impact.hit;
		if ( hit.blocker ) {
			mBulletHoles.add( hit.point, hit.normal, hit.blocker );
			mImpactDust.spawn( hit.point, hit.normal );
		} else if ( hit.target ) {
			hitZombie( impact );
		}
	}
	mBulletHoles.update();
	mImpactDust.update();
	mBloodSpray.update();
	mSeveredLimbs.update( &mGround ); // 落ちる先は地面だけ（建物・車の上には乗らず、抜ける）
	updateZombies();
	// このティックで消す印の付いたものを、全員の更新が済んでから消す（名簿からも外れる）。
	mZombies.flush();
	mPickups.flush();
	// 名簿をたどり終えてから増やす（たどっている最中に spawn すると並びが動く）。
	spawnZombiesFromSpawner( cameraYaw );
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
		chaseTarget = &target;
	}
	// 攻撃に行っているゾンビ（前のティックから続いている 1 体）。名簿をたどる途中で攻撃を始めた体も、ここへ入れて
	// 後の体に attackTaken を渡す（同じティックに 2 体が攻撃を始めないように）。
	const Zombie* attacker = nullptr;
	for ( const auto& zombie : mZombies ) {
		if ( zombie->isAttacking() ) {
			attacker = zombie.get();
			break;
		}
	}
	const Zombie* holder = nullptr;
	for ( auto& zombie : mZombies ) {
		target.attackTaken = ( attacker != nullptr && attacker != zombie.get() );
		zombie->update( chaseTarget, mHitWorld );
		if ( !attacker && zombie->isAttacking() ) {
			attacker = zombie.get();
		}
		if ( zombie->isHoldingTarget() ) {
			holder = zombie.get();
		}
		if ( zombie->grabbedThisTick() ) {
			ZombieAttackEvent event;
			event.kind = ZombieAttackEvent::Kind::Grab;
			event.point = zombie->headPosition();
			mZombieAttacks.push_back( event );
		}
		if ( zombie->bitThisTick() && mPlayer ) {
			mPlayer->applyBite( ZombieAttack::biteDamage(), mZombieRandom.getFloat() );
			ZombieAttackEvent event;
			event.kind = ZombieAttackEvent::Kind::Bite;
			event.point = zombie->headPosition();
			mZombieAttacks.push_back( event );
		}
		// 沈み終わった死体は、全員の更新が済んでから消す（update の flush）。
		if ( zombie->isGone() ) {
			mZombies.destroyLater( zombie.get() );
		}
	}
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
	for ( const auto& zombie : mZombies ) {
		if ( zombie->state() == Zombie::State::Alive ) {
			++view.standingZombies;
		}
	}
	view.fuelDelivered = mEscapeCar ? mEscapeCar->fuelDelivered() : 0;
	view.hasRoom = mZombies.size() < kMaxZombies;
	mSpawnOrders.clear();
	mZombieSpawner->update( view, &mSpawnOrders );
	for ( const ZombieSpawner::Order& order : mSpawnOrders ) {
		spawnZombie( order.position, order.yawDegree );
	}
}

void GameWorld::hitZombie( const BulletList::Impact& impact ) {
	const HitWorld::Hit& hit = impact.hit;
	Zombie* zombie = mZombies.find( hit.target );
	if ( !zombie ) {
		return; // ゾンビでない的（今は無いが、自キャラを撃つ相手が増えたとき）
	}
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
	zombie->applyDamage( damage );
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

void GameWorld::draw( const ViewFrustum& frustum ) const {
	mGround.draw( frustum );
	if ( mPlayer ) {
		mPlayer->draw();
	}
	mBullets.draw();
	for ( const auto& zombie : mZombies ) {
		zombie->draw();
	}
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
	for ( const auto& pickup : mPickups ) {
		pickup->draw( frustum );
	}
}

void GameWorld::drawBulletHoles( const Matrix44& projectionView ) {
	mBulletHoles.draw( projectionView );
}

void GameWorld::drawImpactDust( const Matrix44& projectionView, const Vector3& viewDirection ) {
	mImpactDust.draw( projectionView, viewDirection );
}

void GameWorld::drawBloodSpray( const Matrix44& projectionView, const Vector3& viewDirection ) {
	mBloodSpray.draw( projectionView, viewDirection );
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

bool GameWorld::muzzleLight( MuzzleFlash::Light* out ) const {
	Vector3 muzzlePosition;
	Vector3 muzzleDirection;
	if ( !mPlayer || !mPlayer->muzzle( &muzzlePosition, &muzzleDirection ) ) {
		return false;
	}
	return mMuzzleFlash.light( muzzlePosition, muzzleDirection, out );
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

const ActorList< Zombie >& GameWorld::zombies() const {
	return mZombies;
}

const std::vector< GameWorld::ZombieHit >& GameWorld::zombieHitsThisTick() const {
	return mZombieHits;
}

const ZombieSpawner* GameWorld::zombieSpawner() const {
	return mZombieSpawner.get();
}

void GameWorld::appendPickupColliders( std::vector< BoxCollider >* out ) const {
	for ( const auto& pickup : mPickups ) {
		if ( !pickup->isTaken() ) {
			out->push_back( pickup->collider() );
		}
	}
}

const std::vector< GameWorld::ZombieAttackEvent >& GameWorld::zombieAttacksThisTick() const {
	return mZombieAttacks;
}

bool GameWorld::isPlayerDead() const {
	return mPlayer && mPlayer->isDead();
}
