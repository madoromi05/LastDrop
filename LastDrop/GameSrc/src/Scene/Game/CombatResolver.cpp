#include "GameSrc/Scene/Game/CombatResolver.h"
#include "GameSrc/Scene/Game/EffectManager.h"
#include "GameSrc/Interaction/PropFactory.h"
#include "GameSrc/Interaction/CorpseLootComponent.h"
#include "GameSrc/Hit/DamageInfo.h"
#include "GameSrc/Component/TransformComponent.h"
#include "GameSrc/Component/SkinnedModelComponent.h"
#include "GameSrc/Component/StaticModelComponent.h"
#include "GameSrc/Component/HitboxComponent.h"
#include "GameSrc/Player/PlayerHealthComponent.h"
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
	// 同時に落ちていられる数。超えたら一番古い物を消す
	const int kMaxAmmoDrops = 10;
	// 足元から自キャラの方へ寄せる長さ（m）。死体の脚の下に隠れにくくする（隠れても、死体を見れば拾える）。
	const float kAmmoDropTowardPlayer = 0.5f;
	const float kFullTurnDegree = 360.f; // 落ちた弾の向きは体ごとにばらばら
}

CombatResolver::CombatResolver( GameObjectWorld* objects, EffectManager* effects, const PropFactory* props ) :
	mObjects( objects ),
	mEffects( effects ),
	mProps( props ),
	mRandom( Random::create( static_cast< int >( Framework::instance().time() ) ) ) {
	ASSERT( objects && effects && props && "CombatResolver : an argument is null." );
}

void CombatResolver::beginTick() {
	mZombieHits.clear();
	mSurfaceHits.clear();
	mZombieAttacks.clear();
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
		dropAmmo( *state, player ); // 脚を失って倒れたまま生きている（Downed）間は落とさない。止めを刺したときに落とす
	}
	// 切り口の決まりは、このゾンビの設計図から読む（設計図の種類が増えても、当たった体の形で切れる）。
	const SkinnedMeshInstance& mesh = zombie->getComponent< SkinnedModelComponent >()->mesh();
	const ZombieModel& model = *zombie->getComponent< ZombieModelComponent >()->model();
	for ( BodyPart part : zombie->getComponent< ZombieDismemberComponent >()->dismemberment().severedThisHit() ) {
		mEffects->spawnSeveredLimb( mesh, model, part, impact.direction );
	}
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
	GameObject* drop = mProps->spawnAmmoDrop( mObjects, position, mRandom.getFloat( kFullTurnDegree ) );
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
