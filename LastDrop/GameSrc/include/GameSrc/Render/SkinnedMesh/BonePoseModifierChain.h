#ifndef INCLUDED_BONE_POSE_MODIFIER_CHAIN_H
#define INCLUDED_BONE_POSE_MODIFIER_CHAIN_H

#include "GameSrc/Render/SkinnedMesh/BonePoseModifier.h"
#include <vector>

// 姿勢の書き換えを、足した順に掛ける。SkinnedMeshInstance は書き換えを1つしか持てないので、
// 1体に2つ以上掛けたいとき（ゾンビの「のけぞり → 失った部位を潰す」）は、これを1つ渡す
// 順番に意味がある（後の書き換えは、前の書き換えの結果の上に掛かる）
class BonePoseModifierChain : public BonePoseModifier{
public:
	// 書き換えを後ろへ足す。所有権は受け取らない。nullptr 不可。このチェーンより長く生きていること
	void add( BonePoseModifier* modifier );
	// 足した書き換えを全部外す（組み直すとき。書き換えの持ち主が外れることがある物に使う）
	void clear();
	virtual void apply( std::vector< GameLib::Math::Matrix34 >* pose ) override;

private:
	std::vector< BonePoseModifier* > mModifiers; // 借りているだけ。掛ける順
};

#endif
