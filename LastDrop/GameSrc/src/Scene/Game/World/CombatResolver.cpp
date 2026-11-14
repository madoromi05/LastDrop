#include "GameSrc/Scene/Game/World/CombatResolver.h"
#include "GameSrc/Hit/HitWorld.h"
#include "GameSrc/Hit/DamageInfo.h"
#include "GameSrc/Component/TransformComponent.h"
#include "GameSrc/Component/SkinnedModelComponent.h"
#include "GameSrc/Component/StaticModelComponent.h"
#include "GameSrc/Component/HitboxComponent.h"
#include "GameSrc/Player/PlayerHealthComponent.h"
#include "GameSrc/Player/Body/HoldComponent.h"
#include "GameSrc/Weapon/GunComponent.h"
#include "GameSrc/Zombie/Attack/ZombieAttack.h"
#include "GameSrc/Zombie/Component/ZombieModelComponent.h"
#include "GameSrc/Zombie/Component/ZombieStateComponent.h"
#include "GameSrc/Zombie/Component/ZombieAttackComponent.h"
#include "GameSrc/Zombie/Component/ZombieHealthComponent.h"
#include "GameSrc/Zombie/Component/ZombieDismemberComponent.h"
#include "GameSrc/Zombie/Animation/ZombieModel.h"
#include "GameSrc/Render/SkinnedMesh/SkinnedMeshInstance.h"
#include "GameSrc/Math/PlacementMatrix.h"
#include "GameSrc/World/GameObjectWorld.h"

#include "GameLib/GameLib.h"
#include "Framework.h"
#include "GameLib/Profiling/Profiler.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// 血の跡
	const float kStainBehindDistance = 2.f;	// 当たった所から弾の向きへ、血が飛ぶ所まで（m）
	const float kStainDropDistance = 1.5f;	// 当たった所から真下へ、滴が落ちる所まで（m）
	const Vector3 kDown( 0.f, -1.f, 0.f );
	const float kMinDripFacingUp = 0.7f;	// 滴の跡を付ける面の法線の上向きの下限（cos。約 45°）。斜めの車の屋根などの上にだけ付け、横向きの面には付けない

	// 頭に当たったとき・部位が千切れたときは大きくする（一番多く血が出る所）
	EffectManager::BloodStainSize bloodStainSize( BodyPart part, bool severed ) {
		return ( part == BodyPart::Head || severed ) ? EffectManager::BloodStainSize::Large : EffectManager::BloodStainSize::Normal;
	}

	// 命中の重さ。重い方から調べる（頭で倒した ＞ 部位が飛んだ ＞ 倒した ＞ ふつう）
	// 重さの決まりを変えるときは、ここだけを直す
	CombatResolver::ZombieHit::Weight chooseHitWeight( BodyPart part, bool killed, bool severed ) {
		typedef CombatResolver::ZombieHit::Weight Weight;
		if ( killed && part == BodyPart::Head ) {
			return Weight::HeadBurst;
		}
		if ( severed ) {
			return Weight::Severed;
		}
		return killed ? Weight::Killed : Weight::Normal;
	}

	// 命中の重さ → 血しぶきの大きさ（ふつうだけ、頭かどうかで分ける）
	EffectManager::BloodHitSize bloodHitSize( CombatResolver::ZombieHit::Weight weight, BodyPart part ) {
		typedef CombatResolver::ZombieHit::Weight Weight;
		switch ( weight ) {
			case Weight::HeadBurst: return EffectManager::BloodHitSize::Burst;
			case Weight::Severed:
			case Weight::Killed:    return EffectManager::BloodHitSize::Heavy;
			default: return ( part == BodyPart::Head ) ? EffectManager::BloodHitSize::Head : EffectManager::BloodHitSize::Body;
		}
	}
}

CombatResolver::CombatResolver( GameObjectWorld* objects, EffectManager* effects, const HitWorld* hitWorld ) :
	mObjects( objects ),
	mEffects( effects ),
	mHitWorld( hitWorld ),
	mZombiesKilled( 0 ) {
	ASSERT( objects && effects && hitWorld && "CombatResolver : an argument is null." );
}

void CombatResolver::beginTick() {
	mZombieHits.clear();
	mSurfaceHits.clear();
	mZombieAttacks.clear();
	mZombiesKilled = 0;
}

void CombatResolver::resolveBulletImpacts( const std::vector< BulletList::Impact >& impacts ) {
	// 物（建物・車・地面・拾える物）に当たった所にだけ、弾痕と砂ぼこりを出し、着弾音のために材質を記録する
	// ゾンビ（target）には出さず、血しぶきを出す
	for ( const BulletList::Impact& impact : impacts ) {
		const HitWorld::Hit& hit = impact.hit;
		if ( hit.blocker ) {
			mEffects->spawnSurfaceImpact( hit.point, hit.normal, hit.blocker );
			mSurfaceHits.push_back( { hit.point, hit.blocker->surfaceMaterial( hit.point ) } );
		} else if ( hit.target ) {
			hitZombie( impact );
		}
	}
}

void CombatResolver::hitZombie( const BulletList::Impact& impact ) {
	const HitWorld::Hit& hit = impact.hit;
	HitboxComponent* hitbox = mObjects->find< HitboxComponent >( hit.target );		// 基点となるコンポーネントの取得と検証
	const ZombieStateComponent* state = hitbox ? hitbox->getComponent< ZombieStateComponent >() : nullptr;
	if ( !state )
		return; // ゾンビでない的（今は無いが、自キャラを撃つ相手が増えたとき）

	PROFILE_EVENT( "ZombieHit" ); // 計測の印（血しぶき・ひるみ・部位欠損が始まる）
	ZombieHit event;
	event.point = hit.point;
	event.part = hit.part;
	event.weight = ZombieHit::Weight::Normal;
	event.killed = false;

	// 血と出来事は、死体に当たったときも出す（体には変わりないため。体力はゾンビが生きている間だけ減らす）
	// 体力の部品は外せる（外したゾンビは撃っても死なない。血と当たった出来事だけ出す）
	ZombieHealthComponent* zombie = state->getComponent< ZombieHealthComponent >();
	if ( !zombie ) {
		mEffects->spawnBloodHit( hit.point, impact.direction, bloodHitSize( event.weight, hit.part ) );
		mZombieHits.push_back( event );
		stainAroundZombie( impact, bloodStainSize( hit.part, false ) );
		return;
	}
	DamageInfo damage;
	damage.part = hit.part;
	damage.baseAmount = impact.damage;
	damage.point = hit.point;
	damage.direction = impact.direction;
	bool wasAlive = state->isAlive();
	zombie->applyDamage( damage ); // 倒れ始めたら（脚を失って倒れたときも）、ゾンビが自分で移動の名簿から外れる

	bool killed = wasAlive && !state->isAlive();
	if ( killed ) {
		++mZombiesKilled; // 倒した数（結果の画面。止めを刺したとき。脚を失って倒れたまま生きている Downed の間は数えない）
	}

	// 切り口の決まりは、このゾンビの設計図から読む（設計図の種類が増えても、当たった体の形で切れる）
	const SkinnedMeshInstance& mesh = zombie->getComponent< SkinnedModelComponent >()->mesh();
	const ZombieModel& model = *zombie->getComponent< ZombieModelComponent >()->model();
	const std::vector< BodyPart >& severed = zombie->getComponent< ZombieDismemberComponent >()->dismemberment().severedThisHit();

	event.weight = chooseHitWeight( hit.part, killed, !severed.empty() );
	event.killed = killed;
	mZombieHits.push_back( event );
	mEffects->spawnBloodHit( hit.point, impact.direction, bloodHitSize( event.weight, hit.part ) ); // 重さが決まってから（頭は多く）

	for ( BodyPart part : severed ) {
		severLimb( mesh, model, part, impact.direction );
	}
	stainAroundZombie( impact, bloodStainSize( hit.part, !severed.empty() ) );	// 千切れた部位がいくつあっても、血の跡は 1 発につき 1 組（切れたのはこの 1 発の当たった所なので）
}

void CombatResolver::severLimb( const SkinnedMeshInstance& zombieMesh, const ZombieModel& model, BodyPart part, const Vector3& direction ) {
	zombieMesh.shownWorldPose( &mSeverPose );
	const ZombieSeverLayout::Cut& cut = model.severLayout().cut( part );
	mEffects->spawnSeverBlood( PlacementMatrix::translationOf( mSeverPose[ cut.jointBone ] ), direction );
	mLimbFactory.spawn( mObjects, model.data(), cut, mSeverPose, direction ); // 弾の解決は段と段の間なので、ここで物を置いてよい
}

void CombatResolver::stainAroundZombie( const BulletList::Impact& impact, EffectManager::BloodStainSize size ) {
	const HitWorld::Hit& hit = impact.hit;
	// 後ろ：キャラも聞く（当たったゾンビ自身は外す）。物に当たったときだけ付け、別のゾンビに当たったら付けない
	Vector3 behind;
	behind.setMul( impact.direction, kStainBehindDistance );
	HitWorld::Hit wall;
	if ( mHitWorld->raycast( hit.point, behind, hit.target, &wall ) && wall.blocker ) {
		mEffects->spawnBloodSplatter( wall.point, wall.normal, impact.direction, wall.blocker, size );
	}
	// 真下：物だけを聞く（当たったゾンビ自身の脚で止まらないように）。上を向いた面にだけ付ける
	Vector3 drop;
	drop.setMul( kDown, kStainDropDistance );
	HitWorld::Hit floor;
	if ( mHitWorld->raycastBlockers( hit.point, drop, &floor ) && floor.normal.y >= kMinDripFacingUp ) {
		mEffects->spawnBloodDrip( floor.point, floor.normal, floor.blocker, size );
	}
}

void CombatResolver::resolveZombieAttacks( GameObject* player ) {
	if ( !player ) {
		return;
	}
	PlayerHealthComponent* playerHealth = player->getComponent< PlayerHealthComponent >();
	mObjects->forEach< ZombieAttackComponent >( [ & ]( const ZombieAttackComponent& attack ) {
		ZombieAttackEvent event;
		event.point = attack.getComponent< ZombieStateComponent >()->headPosition();
		if ( attack.grabbedTargetThisTick() ) {
			event.kind = ZombieAttackEvent::Kind::Grab;
			mZombieAttacks.push_back( event );
		}
		if ( attack.hitTargetThisTick() && playerHealth ) {
			// 体力は体全体で 1 つなので、体を引きずって足元から腕を伸ばすゾンビも同じダメージ
			playerHealth->applyZombieHit( ZombieAttack::hitDamage() );
			event.kind = ZombieAttackEvent::Kind::Hit;
			mZombieAttacks.push_back( event );
		}
	} );
}

const std::vector< CombatResolver::ZombieHit >& CombatResolver::zombieHitsThisTick() const {
	return mZombieHits;
}

const std::vector< CombatResolver::SurfaceHit >& CombatResolver::surfaceHitsThisTick() const {
	return mSurfaceHits;
}

const std::vector< CombatResolver::ZombieAttackEvent >& CombatResolver::zombieAttacksThisTick() const {
	return mZombieAttacks;
}

int CombatResolver::zombiesKilledThisTick() const {
	return mZombiesKilled;
}
