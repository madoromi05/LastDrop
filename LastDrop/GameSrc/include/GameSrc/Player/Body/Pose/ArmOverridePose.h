#ifndef INCLUDED_ARM_OVERRIDE_POSE_H
#define INCLUDED_ARM_OVERRIDE_POSE_H

#include "GameSrc/Render/SkinnedMesh/BonePoseModifier.h"
#include "GameLib/Math/Matrix34.h"
#include <vector>

struct FirstPersonRig;

// 一人称の体の書き換えの段 0：両腕（肩から先）だけを別のアニメ（リロード・拳銃の構え）のポーズに差し替える
// 胸の骨から見た位置・向きで写すので、脚と背骨は今のアニメ（歩き・静止）のまま、腕だけ胸に付いて動く
// 一番先に掛ける（差し替えた腕も、背骨の曲げ（視線の上下）と、その後の銃の段をそのまま受けるように）
class ArmOverridePose : public BonePoseModifier{
public:
	// rig: 骨の番号を読む。所有権は受け取らない。nullptr 不可。この段より長く生きていること
	explicit ArmOverridePose( const FirstPersonRig* rig );

	// 説明は FirstPersonBody::setArmOverride
	void set( const std::vector< GameLib::Math::Matrix34 >* pose, float weight, float followHandWeight,
		const GameLib::Math::Matrix34* followReference );

	virtual void apply( std::vector< GameLib::Math::Matrix34 >* pose ) override;

private:
	const FirstPersonRig* mRig;
	const std::vector< GameLib::Math::Matrix34 >* mLayer; // 借りているだけ。nullptr なら差し替えない
	float mWeight;
	float mFollowHandWeight; // 差し替えた腕を今のアニメの右手へ寄せる割合
	const GameLib::Math::Matrix34* mFollowReference; // 借りているだけ。寄せる基準の右手
};

#endif
