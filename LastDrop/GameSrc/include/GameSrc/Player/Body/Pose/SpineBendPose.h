#ifndef INCLUDED_SPINE_BEND_POSE_H
#define INCLUDED_SPINE_BEND_POSE_H

#include "GameSrc/Render/SkinnedMesh/BonePoseModifier.h"

struct FirstPersonRig;

// 一人称の体の書き換えの段 1：視点の上下（ピッチ）ぶん背骨を曲げる。腕と銃が視線に付いてきて、見上げても見下ろしても画面に残る
// 背骨3本（Spine / Spine1 / Spine2）に分けて曲げるのは、1本で曲げると腰から折れたように見えるため
class SpineBendPose : public BonePoseModifier{
public:
	explicit SpineBendPose( const FirstPersonRig* rig );	// rig: 骨の番号とピッチを読む。所有権は受け取らない。nullptr 不可。この段より長く生きていること

	virtual void apply( std::vector< GameLib::Math::Matrix34 >* pose ) override;

private:
	const FirstPersonRig* mRig;
};

#endif
