#include "GameSrc/Zombie/Damage/SeveredLimbFactory.h"
#include "GameSrc/Zombie/Component/SeveredLimbComponent.h"
#include "GameSrc/World/GameObjectWorld.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

namespace{
	const int kCapacity = 24;					// 同時に出しておける数。ゾンビ 1 体で腕 2・脚 2・頭 1 の 5 つまでなので、4〜5 体ぶん
	const unsigned kRandomSeed = 4517u;			// 見た目のばらつきに使うだけなので、毎回同じ種でよい
	const char* kLimbName = "SeveredLimb";		// 確認用の名前（F6 のエディタの物の一覧）
	const int kOneMore = 1;						// これから出す 1 つの分
}

SeveredLimbFactory::SeveredLimbFactory() :
	mRandom( kRandomSeed ) {
}

GameObject* SeveredLimbFactory::spawn( GameObjectWorld* world, const SkinnedMeshData* data, const ZombieSeverLayout::Cut& cut,
const std::vector< Matrix34 >& worldPose, const Vector3& direction ) {
	ASSERT( world && "SeveredLimbFactory::spawn : world is null." );
	retireOldest( world );
	GameObject* obj = world->create( GameObjectTag::SeveredLimb, kLimbName );
	obj->addComponent< SeveredLimbComponent >( data, cut, worldPose, direction, &mRandom );
	world->spawn( obj );
	return obj;
}

void SeveredLimbFactory::retireOldest( GameObjectWorld* world ) const {
	int over = world->count< SeveredLimbComponent >() + kOneMore - kCapacity;
	if ( over <= 0 ) {
		return;
	}
	// たどる順は spawn した順（古い物から）。もう印の付いた物にもう一度付けても同じ
	world->forEach< SeveredLimbComponent >( [ world, &over ]( SeveredLimbComponent& limb ) {
		if ( over > 0 ) {
			world->destroyLater( limb.gameObject() );
			--over;
		}
	} );
}
