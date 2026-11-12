#ifndef INCLUDED_SEVERED_LIMB_LIST_H
#define INCLUDED_SEVERED_LIMB_LIST_H

#include "GameSrc/Zombie/Damage/SeveredLimb.h"
#include <memory>
#include <random>
#include <vector>

// 千切れた部位の持ち主。出す（spawn）と1つ増え、進めて（update）沈み終わった物を消し、全部を描く（draw）
// 同時に出しておけるのは決まった数まで。超えたら一番古い物から消す（1つにつきゾンビ1体ぶんの頂点を変形するため）
// どこで出すか（ゾンビの部位が 0 になったら）は頼む側（CombatResolver。EffectManager を通す）が決める
class SeveredLimbList{
public:
	SeveredLimbList();
	SeveredLimbList( const SeveredLimbList& ) = delete;
	SeveredLimbList& operator=( const SeveredLimbList& ) = delete;

	// 引数は SeveredLimb のコンストラクタと同じ（random はこのリストが持つ物を使う）
	void spawn( const SkinnedMeshData* data, const ZombieSeverLayout::Cut& cut, const std::vector< GameLib::Math::Matrix34 >& worldPose,
		const GameLib::Math::Vector3& direction );
	void update( const IRayBlocker* ground );		// 全部を1ティック進め、沈み終わった物を消す。ground: SeveredLimb::update と同じ
	void draw( const DrawCulling& culling ) const;	// culling で描くと決まった物だけ描く

private:
	std::vector< std::unique_ptr< SeveredLimb > > mLimbs; // 古い順
	std::mt19937 mRandom; // 回る軸と速さのばらつき
};

#endif
