#ifndef INCLUDED_EYE_POINT_H
#define INCLUDED_EYE_POINT_H

#include "GameSrc/Render/SkinnedMesh/BonePoseModifier.h"

struct FirstPersonRig;

// 一人称の体の書き換えの段 2：背骨を曲げた後の頭から、目の位置（カメラの視点）を決めて rig へ書く。骨は動かさない（測るだけ）。
// 銃を目へ寄せる段・照準のぶれの段が、この目を読む。目を曲げる前の頭で決めると、見上げ・見下ろしで目と銃がずれる。
// 両手持ち・胸元への寄せより先に決める（あの 2 つが動かすのは腕と右手だけで、頭は動かないので答えは同じ。HighPrioritySplit_Plan.txt 5-4）。
class EyePoint : public BonePoseModifier{
public:
	// rig: 骨の番号とピッチを読み、目の位置を書く。所有権は受け取らない。nullptr 不可。この段より長く生きていること。
	explicit EyePoint( FirstPersonRig* rig );

	virtual void apply( std::vector< GameLib::Math::Matrix34 >* pose ) override;

private:
	FirstPersonRig* mRig;
};

#endif
