#include "GameSrc/Zombie/Animation/ZombieModel.h"
#include "GameSrc/Render/SkinnedMesh/SkinnedMeshData.h"
#include "GameSrc/Render/SkinnedMesh/BoneNamePattern.h"
#include "GameSrc/Zombie/Move/ZombieCrowdParams.h"

#include "GameLib/GameLib.h"
#include "GameLib/Base/DebugStream.h"

namespace{
	const char* kBoneNamePrefix = "Zombie_";	// 骨の名前の頭の共通部分（Resource/Model/Character/zombie.txt の骨は "Zombie_Head" のような名前）
	// 歩くときに地面についているかを比べる足先の骨（ZombieStride）
	const char* kLeftToeBone = "LeftToeBase";
	const char* kRightToeBone = "RightToeBase";
	const char* kRootBone = "Hips";				// 骨格の根の骨（腰）。1回だけ流すアニメでは、この骨が前後へ動く（ZombieRootMotion）

	// name のアニメ。name が nullptr なら空（表を作らない）
	GameLib::Scene::Animation animationOrEmpty( const SkinnedMeshData* data, const char* name ) {
		return name ? data->animation( name ) : GameLib::Scene::Animation();
	}
}

ZombieModel::ZombieModel( const SkinnedMeshData* data, const char* walkAnimation, const char* runAnimation,
const std::vector< const char* >& rootMotionAnimations ) :
	mData( data ),
	mHitboxLayout( data, kBoneNamePrefix ),
	mFootprint( data, kBoneNamePrefix, zombieCrowdParams().footprintPadding ),	// 部位の当たり判定（HitboxLayout）には余白を足さない（弾は体ぴったりで当てる）
	mFlinchLayout( data, kBoneNamePrefix ),
	mSeverLayout( data, kBoneNamePrefix ),
	mStride( data, data->animation( walkAnimation ),
		BoneNamePattern::find( data, kBoneNamePrefix, kLeftToeBone ), BoneNamePattern::find( data, kBoneNamePrefix, kRightToeBone ) ),
	// 走りも同じ足先で測る（走りのアニメも腰が進まず、その場で走る）
	mRunStride( data, animationOrEmpty( data, runAnimation ),
		BoneNamePattern::find( data, kBoneNamePrefix, kLeftToeBone ), BoneNamePattern::find( data, kBoneNamePrefix, kRightToeBone ) ),
	mCrawlLayout( data, kBoneNamePrefix ) {
	int rootBone = BoneNamePattern::find( data, kBoneNamePrefix, kRootBone );
	for ( const char* name : rootMotionAnimations ) {
		mRootMotions.push_back( NamedRootMotion{ name, ZombieRootMotion( data, data->animation( name ), rootBone ) } );
		// 起動時のデバッグ出力で表が作れたかを見る（atack1 約 0.16m・gethit 約 0.18m・death1 約 0.88m・death2 約 1.2m）
		const ZombieRootMotion& table = mRootMotions.back().table;
		if ( table.isValid() ) {
			GameLib::cout << "ZombieRootMotion: " << name << " " << table.farthestDistance() << " m" << GameLib::endl;
		} else {
			GameLib::cout << "ZombieRootMotion: no table for " << name << GameLib::endl;
		}
	}
}

const ZombieRootMotion* ZombieModel::rootMotion( const char* name ) const {
	if ( !name ) {
		return nullptr;
	}
	for ( const NamedRootMotion& entry : mRootMotions ) {
		if ( entry.name == name ) {
			return entry.table.isValid() ? &entry.table : nullptr;
		}
	}
	return nullptr;
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

const ZombieStride& ZombieModel::runStride() const {
	return mRunStride;
}

const ZombieCrawlLayout& ZombieModel::crawlLayout() const {
	return mCrawlLayout;
}
