#ifndef INCLUDED_BONE_POSE_MODIFIER_H
#define INCLUDED_BONE_POSE_MODIFIER_H

#include "GameLib/Math/Matrix34.h"
#include <vector>

// アニメで決まったポーズを、描く直前に書き換える処理の窓口。
// SkinnedMeshInstance はアニメ・クロスフェードだけを知り、「視点に合わせて上半身を曲げる」
// 「一人称で頭を消す」といったゲーム側の都合はこの派生クラスに任せる（インスタンスを用途ごとに増やさないため）。
//
// 書き換えるのは「見せるポーズ」の写しだけ。アニメのポーズ本体には残らないので、
// クロスフェードの混ぜ元に書き換えが入り込んで二重に掛かることはない。
// apply が const でないのは、書き換えの途中で求めた値（一人称の目の位置など）を派生クラスが覚えておけるようにするため。
class BonePoseModifier{
public:
	virtual ~BonePoseModifier() {}
	// pose: 骨格の根から見たボーンの姿勢（SkinnedMeshData::computeBonePose の並び）。nullptr 不可。
	virtual void apply( std::vector< GameLib::Math::Matrix34 >* pose ) = 0;
};

#endif
