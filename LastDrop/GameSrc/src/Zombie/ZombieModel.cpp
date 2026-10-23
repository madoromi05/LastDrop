#include "GameSrc/Zombie/ZombieModel.h"
#include "GameSrc/Render/SkinnedMesh/SkinnedMeshData.h"
#include "GameSrc/Render/SkinnedMesh/BoneNamePattern.h"

namespace{
	// 骨の名前の頭の共通部分（Resource/character.txt の骨は "Zombie_Head" のような名前）。
	const char* kBoneNamePrefix = "Zombie_";
	// 移動の当たり判定を体より一回り大きくする幅（m。四辺それぞれ）。体の見た目どうしがめり込む前に止めるため。
	// 部位の当たり判定（HitboxLayout）は変えない（弾は体ぴったりで当てる）。
	const float kFootprintPadding = 0.15f;
	// 歩くときに地面についているかを比べる足先の骨（ZombieStride）。
	const char* kLeftToeBone = "LeftToeBase";
	const char* kRightToeBone = "RightToeBase";
}

ZombieModel::ZombieModel( const SkinnedMeshData* data, const char* walkAnimation ) :
	mData( data ),
	mHitboxLayout( data, kBoneNamePrefix ),
	mFootprint( data, kBoneNamePrefix, kFootprintPadding ),
	mFlinchLayout( data, kBoneNamePrefix ),
	mSeverLayout( data, kBoneNamePrefix ),
	mStride( data, data->animation( walkAnimation ),
		BoneNamePattern::find( data, kBoneNamePrefix, kLeftToeBone ), BoneNamePattern::find( data, kBoneNamePrefix, kRightToeBone ) ),
	mCrawlLayout( data, kBoneNamePrefix ) {
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

const ZombieFlinchLayout& ZombieModel::flinchLayout() const {
	return mFlinchLayout;
}

const ZombieSeverLayout& ZombieModel::severLayout() const {
	return mSeverLayout;
}

const ZombieStride& ZombieModel::stride() const {
	return mStride;
}

const ZombieCrawlLayout& ZombieModel::crawlLayout() const {
	return mCrawlLayout;
}
