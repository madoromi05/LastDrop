#ifndef INCLUDED_ZOMBIE_MODEL_H
#define INCLUDED_ZOMBIE_MODEL_H

#include "GameSrc/Hit/HitboxLayout.h"
#include "GameSrc/Collision/BodyFootprint.h"
#include "GameSrc/Zombie/Damage/ZombieFlinchLayout.h"
#include "GameSrc/Zombie/Damage/ZombieSeverLayout.h"
#include "GameSrc/Zombie/Move/ZombieStride.h"
#include "GameSrc/Zombie/Crawl/ZombieCrawlLayout.h"
#include "GameSrc/Zombie/Animation/ZombieRootMotion.h"
#include <string>
#include <vector>

class SkinnedMeshData;

// ゾンビ1種類ぶんの、全員で共有する決まりごと。形（SkinnedMeshData）と、そこから求めた
// 部位の箱の設計図（HitboxLayout）・移動の当たり判定の大きさ（BodyFootprint）・のけぞりで曲げる骨（ZombieFlinchLayout）・
// 部位欠損で切る関節と骨（ZombieSeverLayout）・歩き／走りアニメで体を進める長さの表（ZombieStride）・
// 脚を失って体を引きずるときに曲げる腕の骨（ZombieCrawlLayout）・1回だけ流すアニメの根の動きの表（ZombieRootMotion）。
// どれもメッシュが同じなら結果も同じ（前の2つは全頂点を調べて求める）なので、種類ごとに1回だけ求めてここに置く。
// ゾンビ（Zombie）はこれを借りるだけなので、何体出しても、途中で湧かせても、求め直しにならない。
// 骨の名前の決まり・当たり判定の余白といったゾンビ固有の値は、この .cpp だけが知っている。
class ZombieModel{
public:
	// data: 形・見た目・アニメ。所有権は受け取らない。nullptr 不可、isReady() 済みであること。
	//       この ZombieModel より長く生きていること（MeshCache が持つ）。
	// walkAnimation: その場で歩くアニメの名前（体を進める長さの表を作る）。nullptr 不可。名前は構築中だけ使う（覚えない）。
	// runAnimation: その場で走るアニメの名前（走る体を進める長さの表を作る。R4_Plan.txt）。nullptr なら表を作らない（走る体はいない）。
	// rootMotionAnimations: 根の動きを置き場所へ移すアニメの名前（攻撃・よろけ・倒れる）。名前は構築中だけ使う（覚えない）。
	//                       見つからない名前は表を作らない（rootMotion が nullptr を返す）。
	ZombieModel( const SkinnedMeshData* data, const char* walkAnimation, const char* runAnimation,
		const std::vector< const char* >& rootMotionAnimations );
	ZombieModel( const ZombieModel& ) = delete;
	ZombieModel& operator=( const ZombieModel& ) = delete;

	const SkinnedMeshData* data() const;
	const HitboxLayout& hitboxLayout() const;	// 部位の箱の設計図
	const BodyFootprint& footprint() const;		// 移動の当たり判定の大きさ（体を真上から囲む長方形）
	const ZombieFlinchLayout& flinchLayout() const; // のけぞりで曲げる関節と骨
	const ZombieSeverLayout& severLayout() const;   // 部位欠損で切る関節と、無くなる骨・部位
	const ZombieStride& stride() const;             // 歩きアニメの1フレームごとに体を進める長さ
	const ZombieStride& runStride() const;          // 走りアニメの1フレームごとに体を進める長さ（無ければ isValid が false）
	const ZombieCrawlLayout& crawlLayout() const;   // 体を引きずるときに曲げる腕の骨
	// name のアニメの根の動きの表。name が nullptr・構築で渡していない・表が作れなかったなら nullptr（根の動きを移さない）。
	const ZombieRootMotion* rootMotion( const char* name ) const;

private:
	// 根の動きの表と、そのアニメの名前。
	struct NamedRootMotion{
		std::string name;
		ZombieRootMotion table;
	};
	const SkinnedMeshData* mData; // 借りているだけ
	HitboxLayout mHitboxLayout;
	BodyFootprint mFootprint;
	ZombieFlinchLayout mFlinchLayout;
	ZombieSeverLayout mSeverLayout;
	ZombieStride mStride;
	ZombieStride mRunStride;
	ZombieCrawlLayout mCrawlLayout;
	std::vector< NamedRootMotion > mRootMotions;
};

#endif
