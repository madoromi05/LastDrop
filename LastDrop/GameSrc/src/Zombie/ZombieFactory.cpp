#include "GameSrc/Zombie/ZombieFactory.h"
#include "GameSrc/Zombie/ZombieModel.h"
#include "GameSrc/Zombie/ZombieUncertaintyParams.h"
#include "GameSrc/Zombie/ZombieHealthComponent.h"
#include "GameSrc/Zombie/ZombieDismemberComponent.h"
#include "GameSrc/Zombie/ZombieAttackComponent.h"
#include "GameSrc/Zombie/ZombieCorpseComponent.h"
#include "GameSrc/Zombie/ZombieChaseComponent.h"
#include "GameSrc/Zombie/ZombieRootMotionComponent.h"
#include "GameSrc/Component/TransformComponent.h"
#include "GameSrc/Component/SkinnedModelComponent.h"
#include "GameSrc/Component/HitboxComponent.h"
#include "GameSrc/Component/CharacterColliderComponent.h"
#include "GameSrc/World/GameObjectWorld.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Random.h"

using namespace GameLib::Math;

namespace{
	const char* kZombieName = "Zombie"; // 確認用の名前（F6 のエディタの物の一覧）
	// ── 体力の個体差（SummaryPlan/個別仕様/ZombieUncertainty_Plan.txt の 3 章。幅から選ぶのは ZombieUncertaintyParams）──
	// T ポーズの確認用（歩くアニメが無い）の体の丈夫さ。表のままにして、部位ごとの減り方を確かめられるようにする。
	const float kFixedToughness = 1.f;
}

ZombieFactory::ZombieFactory( const ZombieModel* model, const CollisionWorld* collisionWorld, const NavMesh* navMesh,
ZombieBrainComponent::TickContext* context ) :
	mModel( model ),
	mCollisionWorld( collisionWorld ),
	mNavMesh( navMesh ),
	mContext( context ) {
	ASSERT( model && collisionWorld && navMesh && context && "ZombieFactory : argument is null." );
}

ZombieBrainComponent* ZombieFactory::spawn( GameObjectWorld* world, const Vector3& position, float angleY,
const ZombieAnimationNames& names, int sightPhase, int randomSeed ) const {
	// 体ごとの乱数から、揺れの速さ・丈夫さ・うろつき方の種を引く（同じ種のままだと、選ぶ値の並びが揃うので種も引き直す）。
	// 残りは頭のクリティカルを引くのに、つなぐ部品が持つ。
	Random random = Random::create( randomSeed );
	const ZombieUncertaintyParams& params = zombieUncertaintyParams();
	float swayPeriod = params.pickSwayPeriod( &random );
	float swayPhase = random.getFloat();
	float toughness = names.move ? params.pickToughness( &random ) : kFixedToughness;
	int wanderSeed = random.getInt();

	GameObject* zombie = world->create( GameObjectTag::Zombie, kZombieName );
	zombie->addComponent< TransformComponent >( position, angleY );
	zombie->addComponent< SkinnedModelComponent >( mModel->data() );
	zombie->addComponent< HitboxComponent >( &mModel->hitboxLayout() );
	zombie->addComponent< CharacterColliderComponent >( mCollisionWorld );
	zombie->addComponent< ZombieHealthComponent >( toughness, &mModel->flinchLayout() );
	zombie->addComponent< ZombieDismemberComponent >( mModel );
	zombie->addComponent< ZombieAttackComponent >();
	zombie->addComponent< ZombieCorpseComponent >();
	zombie->addComponent< ZombieChaseComponent >( mNavMesh, sightPhase, wanderSeed, &mModel->flinchLayout(), swayPeriod, swayPhase );
	zombie->addComponent< ZombieRootMotionComponent >( mModel, names );
	ZombieBrainComponent* brain = zombie->addComponent< ZombieBrainComponent >( mModel, mContext, random );
	world->spawn( zombie );
	return brain;
}
