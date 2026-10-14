#ifndef INCLUDED_ZOMBIE_MODEL_H
#define INCLUDED_ZOMBIE_MODEL_H

#include "GameSrc/Hit/HitboxLayout.h"
#include "GameSrc/Collision/BodyFootprint.h"

class SkinnedMeshData;

// ゾンビ1種類ぶんの、全員で共有する決まりごと。形（SkinnedMeshData）と、そこから求めた
// 部位の箱の設計図（HitboxLayout）・移動の当たり判定の大きさ（BodyFootprint）。
// どちらも全頂点を調べて求めるが、メッシュが同じなら結果も同じなので、種類ごとに1回だけ求めてここに置く。
// ゾンビ（Zombie）はこれを借りるだけなので、何体出しても、途中で湧かせても、求め直しにならない。
// 骨の名前の決まり・当たり判定の余白といったゾンビ固有の値は、この .cpp だけが知っている。
class ZombieModel{
public:
	// data: 形・見た目・アニメ。所有権は受け取らない。nullptr 不可、isReady() 済みであること。
	//       この ZombieModel より長く生きていること（MeshCache が持つ）。
	explicit ZombieModel( const SkinnedMeshData* data );
	ZombieModel( const ZombieModel& ) = delete;
	ZombieModel& operator=( const ZombieModel& ) = delete;

	const SkinnedMeshData* data() const;
	const HitboxLayout& hitboxLayout() const;	// 部位の箱の設計図
	const BodyFootprint& footprint() const;		// 移動の当たり判定の大きさ（体を真上から囲む長方形）

private:
	const SkinnedMeshData* mData; // 借りているだけ
	HitboxLayout mHitboxLayout;
	BodyFootprint mFootprint;
};

#endif
