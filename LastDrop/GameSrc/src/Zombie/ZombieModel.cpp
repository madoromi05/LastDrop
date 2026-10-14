#include "GameSrc/Zombie/ZombieModel.h"

namespace{
	// 骨の名前の頭の共通部分（Resource/character.txt の骨は "Zombie_Head" のような名前）。
	const char* kBoneNamePrefix = "Zombie_";
	// 移動の当たり判定を体より一回り大きくする幅（m。四辺それぞれ）。体の見た目どうしがめり込む前に止めるため。
	// 部位の当たり判定（HitboxLayout）は変えない（弾は体ぴったりで当てる）。
	const float kFootprintPadding = 0.15f;
}

ZombieModel::ZombieModel( const SkinnedMeshData* data ) :
	mData( data ),
	mHitboxLayout( data, kBoneNamePrefix ),
	mFootprint( data, kBoneNamePrefix, kFootprintPadding ) {
}

const SkinnedMeshData* ZombieModel::data() const {
	return mData;
}

const HitboxLayout& ZombieModel::hitboxLayout() const {
	return mHitboxLayout;
}

const BodyFootprint& ZombieModel::footprint() const {
	return mFootprint;
}
