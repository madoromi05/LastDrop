#include "GameSrc/Zombie/ZombieFactory.h"
#include "GameSrc/Zombie/Animation/ZombieModel.h"
#include "GameSrc/Zombie/ZombieUncertaintyParams.h"
#include "GameSrc/Zombie/Move/ZombieRunParams.h"
#include "GameSrc/Zombie/Component/ZombieModelComponent.h"
#include "GameSrc/Zombie/Component/ZombieBrainComponent.h"
#include "GameSrc/Zombie/Component/ZombieHealthComponent.h"
#include "GameSrc/Zombie/Component/ZombieDismemberComponent.h"
#include "GameSrc/Zombie/Component/ZombieAttackComponent.h"
#include "GameSrc/Zombie/Component/ZombieCorpseComponent.h"
#include "GameSrc/Zombie/Component/ZombieChaseComponent.h"
#include "GameSrc/Zombie/Component/ZombieRootMotionComponent.h"
#include "GameSrc/Zombie/Component/ZombieStateComponent.h"
#include "GameSrc/Zombie/Component/ZombieCrawlComponent.h"
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
	// ── 体力の個体差（幅から選ぶのは ZombieUncertaintyParams） ──
	// T ポーズの確認用（歩くアニメが無い）の体の丈夫さ。表のままにして、部位ごとの減り方を確かめられるようにする
	const float kFixedToughness = 1.f;
}

ZombieFactory::ZombieFactory( const ZombieModel* model, ZombieAttackStyle attackStyle ) :
	mModel( model ),
	mAttackStyle( attackStyle ) {
	ASSERT( model && "ZombieFactory : model is null." );
}

ZombieBrainComponent* ZombieFactory::spawn( GameObjectWorld* world, const Vector3& position, float angleY,
const ZombieAnimationNames& names, int sightPhase, int randomSeed ) const {
	// 体ごとの乱数から、揺れの速さ・丈夫さ・うろつき方の種を引く（同じ種のままだと、選ぶ値の並びが揃うので種も引き直す）
	// 残りは頭のクリティカルを引くのに、体力の部品が持つ
	Random random = Random::create( randomSeed );
	const ZombieUncertaintyParams& params = zombieUncertaintyParams();
	float swayPeriod = params.pickSwayPeriod( &random );
	float swayPhase = random.getFloat();
	float toughness = names.move ? params.pickToughness( &random ) : kFixedToughness;
	int wanderSeed = random.getInt();
	// 走る体か（湧いたときに 1 回だけ決め、一生変わらない。走るアニメが無ければ走らない）
	// 乱数はいつも引く（走るアニメの有無で、後に引く頭のクリティカルの並びが変わらないように）
	bool runner = zombieRunParams().pickRunner( &random ) && names.run != nullptr;

	// 外せる部品（ZombieAttack・ZombieHealth・ZombieCorpse・ZombieCrawl）は、行を消せばその振る舞いだけ無くなる
	// （例：ZombieAttack の行を消すと攻撃しないゾンビ。どれが外せるかは ZombieBrainComponent.h）
	GameObject* obj = world->create( GameObjectTag::Zombie, kZombieName );
	obj->addComponent< TransformComponent >( position, angleY );
	obj->addComponent< ZombieModelComponent >( mModel );
	obj->addComponent< SkinnedModelComponent >( mModel->data() );
	obj->addComponent< HitboxComponent >( &mModel->hitboxLayout() );
	obj->addComponent< CharacterColliderComponent >();
	obj->addComponent< ZombieDismemberComponent >();
	obj->addComponent< ZombieRootMotionComponent >( names );
	obj->addComponent< ZombieStateComponent >();
	obj->addComponent< ZombieChaseComponent >( sightPhase, wanderSeed, swayPeriod, swayPhase, runner );
	obj->addComponent< ZombieAttackComponent >( mAttackStyle );
	obj->addComponent< ZombieHealthComponent >( toughness, random );
	obj->addComponent< ZombieCorpseComponent >();
	obj->addComponent< ZombieCrawlComponent >();
	ZombieBrainComponent* brain = obj->addComponent< ZombieBrainComponent >();
	world->spawn( obj );
	return brain;
}
