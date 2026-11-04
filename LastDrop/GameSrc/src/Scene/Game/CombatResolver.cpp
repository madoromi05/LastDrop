#include "GameSrc/Scene/Game/CombatResolver.h"
#include "GameSrc/Hit/HitWorld.h"
#include "GameSrc/Interaction/PropFactory.h"
#include "GameSrc/Interaction/CorpseLootComponent.h"
#include "GameSrc/Hit/DamageInfo.h"
#include "GameSrc/Component/TransformComponent.h"
#include "GameSrc/Component/SkinnedModelComponent.h"
#include "GameSrc/Component/StaticModelComponent.h"
#include "GameSrc/Component/HitboxComponent.h"
#include "GameSrc/Player/PlayerHealthComponent.h"
#include "GameSrc/Player/HoldComponent.h"
#include "GameSrc/Weapon/GunComponent.h"
#include "GameSrc/Zombie/ZombieAttack.h"
#include "GameSrc/Zombie/ZombieModelComponent.h"
#include "GameSrc/Zombie/ZombieStateComponent.h"
#include "GameSrc/Zombie/ZombieAttackComponent.h"
#include "GameSrc/Zombie/ZombieHealthComponent.h"
#include "GameSrc/Zombie/ZombieDismemberComponent.h"
#include "GameSrc/World/GameObjectWorld.h"

#include "GameLib/GameLib.h"
#include "Framework.h"
#include "GameLib/Profiling/Profiler.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// ── ゾンビが落とす予備の弾
	const float kAmmoDropChance = 0.5f;	// 落とす確率
	// 素手のときに落とす弾（手に持っている銃が無いので、どちらかに決める。始めの銃ではない方＝拾ったライフル用）。
	const ItemId kBareHandAmmo = ITEM_AMMO_762;
	// 同時に落ちていられる数。超えたら一番古い物を消す
	const int kMaxAmmoDrops = 10;
	// 足元から自キャラの方へ寄せる長さ（m）。死体の脚の下に隠れにくくする（隠れても、死体を見れば拾える）。
	const float kAmmoDropTowardPlayer = 0.5f;
	const float kFullTurnDegree = 360.f; // 落ちた弾の向きは体ごとにばらばら

	// ── 血の跡（BloodStain_Plan.txt 3 章。ZombieHitFeedback_Plan.txt 4 章）
	const float kStainBehindDistance = 2.f; // 当たった所から弾の向きへ、血が飛ぶ所まで（m）
	const float kStainDropDistance = 1.5f;  // 当たった所から真下へ、滴が落ちる所まで（m）
	const Vector3 kDown( 0.f, -1.f, 0.f );
	// 滴の跡を付ける面の法線の上向きの下限（cos。約 45°）。斜めの車の屋根などの上にだけ付け、横向きの面には付けない。
	const float kMinDripFacingUp = 0.7f;

	// 頭に当たったとき・部位が千切れたときは大きくする（一番多く血が出る所）。
	EffectManager::BloodStainSize bloodStainSize( BodyPart part, bool severed ) {
		return ( part == BodyPart::Head || severed ) ? EffectManager::BloodStainSize::Large : EffectManager::BloodStainSize::Normal;
	}
}

CombatResolver::CombatResolver( GameObjectWorld* objects, EffectManager* effects, const PropFactory* props, const HitWorld* hitWorld ) :
	mObjects( objects ),
	mEffects( effects ),
	mProps( props ),
	mHitWorld( hitWorld ),
	mRandom( Random::create( static_cast< int >( Framework::instance().time() ) ) ),
	mZombiesKilled( 0 ) {
	ASSERT( objects && effects && props && hitWorld && "CombatResolver : an argument is null." );
}

void CombatResolver::beginTick() {
	mZombieHits.clear();
	mSurfaceHits.clear();
	mZombieAttacks.clear();
	mZombiesKilled = 0;
}

void CombatResolver::resolveBulletImpacts( const std::vector< BulletList::Impact >& impacts, const GameObject* player ) {
	// 物（建物・車・地面・拾える物）に当たった所にだけ、弾痕と砂ぼこりを出し、着弾音のために材質を記録する。
	// ゾンビ（target）には出さず、血しぶきを出す。
	for ( const BulletList::Impact& impact : impacts ) {
		const HitWorld::Hit& hit = impact.hit;
		if ( hit.blocker ) {
			mEffects->spawnSurfaceImpact( hit.point, hit.normal, hit.blocker );
			mSurfaceHits.push_back( { hit.point, hit.blocker->surfaceMaterial( hit.point ) } );
		} else if ( hit.target ) {
			hitZombie( impact, player );
		}
	}
}

void CombatResolver::hitZombie( const BulletList::Impact& impact, const GameObject* player ) {
	const HitWorld::Hit& hit = impact.hit;
	HitboxComponent* hitbox = mObjects->find< HitboxComponent >( hit.target );
	const ZombieStateComponent* state = hitbox ? hitbox->getComponent< ZombieStateComponent >() : nullptr;
	if ( !state ) {
		return; // ゾンビでない的（今は無いが、自キャラを撃つ相手が増えたとき）
	}
	PROFILE_EVENT( "ZombieHit" ); // 計測の印（血しぶき・ひるみ・部位欠損が始まる）
	// 血と出来事は、死体に当たったときも出す（体には変わりないため。体力はゾンビが生きている間だけ減らす）。
	mEffects->spawnBloodHit( hit.point, impact.direction, hit.part );
	ZombieHit event;
	event.point = hit.point;
	event.part = hit.part;
	mZombieHits.push_back( event );

	// 体力の部品は外せる（外したゾンビは撃っても死なない。血と当たった出来事だけ出す）。
	ZombieHealthComponent* zombie = state->getComponent< ZombieHealthComponent >();
	if ( !zombie ) {
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
	if ( wasAlive && !state->isAlive() ) {
		dropAmmo( *state, player ); // 脚を失って倒れたまま生きている（Downed）間は落とさない。止めを刺したときに落とす
		++mZombiesKilled; // 倒した数（スコア。弾を落とすのと同じ時＝止めを刺したとき）
	}
	// 切り口の決まりは、このゾンビの設計図から読む（設計図の種類が増えても、当たった体の形で切れる）。
	const SkinnedMeshInstance& mesh = zombie->getComponent< SkinnedModelComponent >()->mesh();
	const ZombieModel& model = *zombie->getComponent< ZombieModelComponent >()->model();
	const std::vector< BodyPart >& severed = zombie->getComponent< ZombieDismemberComponent >()->dismemberment().severedThisHit();
	for ( BodyPart part : severed ) {
		mEffects->spawnSeveredLimb( mesh, model, part, impact.direction );
	}
	// 千切れた部位がいくつあっても、血の跡は 1 発につき 1 組（切れたのはこの 1 発の当たった所なので）。
	stainAroundZombie( impact, bloodStainSize( hit.part, !severed.empty() ) );
}

void CombatResolver::stainAroundZombie( const BulletList::Impact& impact, EffectManager::BloodStainSize size ) {
	const HitWorld::Hit& hit = impact.hit;
	// 後ろ：キャラも聞く（当たったゾンビ自身は外す）。物に当たったときだけ付け、別のゾンビに当たったら付けない。
	Vector3 behind;
	behind.setMul( impact.direction, kStainBehindDistance );
	HitWorld::Hit wall;
	if ( mHitWorld->raycast( hit.point, behind, hit.target, &wall ) && wall.blocker ) {
		mEffects->spawnBloodSplatter( wall.point, wall.normal, impact.direction, wall.blocker, size );
	}
	// 真下：物だけを聞く（当たったゾンビ自身の脚で止まらないように）。上を向いた面にだけ付ける。
	Vector3 drop;
	drop.setMul( kDown, kStainDropDistance );
	HitWorld::Hit floor;
	if ( mHitWorld->raycastBlockers( hit.point, drop, &floor ) && floor.normal.y >= kMinDripFacingUp ) {
		mEffects->spawnBloodDrip( floor.point, floor.normal, floor.blocker, size );
	}
}

ItemId CombatResolver::droppedAmmo( const GameObject* player ) {
	// 手に持っている銃の弾（拳銃なら 9mm。Pistol_Plan.txt Q3）。素手・自キャラがいなければ 7.62。
	const GunComponent* gun = HoldComponent::heldGun( player );
	return gun ? gun->spec().ammo : kBareHandAmmo;
}

void CombatResolver::dropAmmo( const ZombieStateComponent& zombie, const GameObject* player ) {
	if ( mRandom.getFloat() >= kAmmoDropChance ) {
		return;
	}
	Vector3 position = zombie.position();
	if ( player ) {
		Vector3 toPlayer;
		toPlayer.setSub( player->getComponent< TransformComponent >()->position(), position );
		toPlayer.y = 0.f;
		if ( toPlayer.length() > kAmmoDropTowardPlayer ) {
			toPlayer.normalize();
			position.madd( toPlayer, kAmmoDropTowardPlayer );
		}
	}
	GameObject* drop = mProps->spawnAmmoDrop( mObjects, droppedAmmo( player ), position, mRandom.getFloat( kFullTurnDegree ) );
	if ( !drop ) {
		return; // 形の構築に失敗した（理由は MeshCache が出力済み）
	}
	// 今置いた物を数えて上限を超えたら、一番古い物（今置いた物ではない）を消す。
	if ( mObjects->countTagged( GameObjectTag::AmmoDrop ) > kMaxAmmoDrops ) {
		GameObject* oldest = mObjects->oldestTagged( GameObjectTag::AmmoDrop );
		mEffects->detach( oldest->getComponent< StaticModelComponent >() ); // 物が消える前に外す
		mObjects->destroyLater( oldest );
	}
	// 死体を見ても拾えるよう、死体の物に落とした弾を覚えさせる（★1）。倒れたゾンビは 1 回しか落とさない。
	zombie.gameObject()->addComponent< CorpseLootComponent >( drop );
}

void CombatResolver::resolveZombieAttacks( GameObject* player ) {
	PlayerHealthComponent* playerHealth = player ? player->getComponent< PlayerHealthComponent >() : nullptr;
	mObjects->forEach< ZombieAttackComponent >( [ & ]( const ZombieAttackComponent& attack ) {
		if ( attack.hitTargetThisTick() && playerHealth ) {
			// 体力は体全体で 1 つなので、体を引きずって足元から腕を伸ばすゾンビも同じダメージ。
			playerHealth->applyZombieHit( ZombieAttack::hitDamage() );
			ZombieAttackEvent event;
			event.point = attack.getComponent< ZombieStateComponent >()->headPosition();
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
