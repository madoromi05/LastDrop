#ifndef INCLUDED_WEAPON_PLACE_POSE_H
#define INCLUDED_WEAPON_PLACE_POSE_H

#include "GameSrc/Render/SkinnedMesh/BonePoseModifier.h"

struct FirstPersonRig;

// 一人称の体の書き換えの段 3：手に持つ銃を置く（どれも銃が無ければ何もしない）。この順に掛ける：
//  a. 両手持ち … 右手首を曲げて、銃の先を左手（支える手）へ向ける。銃は右手の骨に付けているので、そのままだと
//     アニメによっては左手が銃から離れる（走りで約8cm）。Mixamo のアニメは両手の間に銃がある前提なので、
//     銃を両手の間に置き直すのがアニメの意図に最も近い（走りでは左手が腕の長さより遠く、左腕の IK では届かない）
//  b. 胸元へ抱える … 走るときは、銃を胸元へ引き寄せ、銃口を下・体の内側へ向ける（Tarkov の走りのような抱え方）
//     持っている走りのアニメは肩の高さで前に構えたままなので、覗くときと同じく両腕をまとめて動かす
//  c. 覗く（ADS） … 銃のサイト線が目と視線に重なるよう、両腕（肩から先）をまとめて動かす（手続き的 ADS）
//     覗く専用のアニメが無くても、持っているアニメのまま覗ける。両手ごと動かすので手は銃から離れない
// b・c は両腕をまとめて動かすだけなので、先に a で両手持ちにしておけば左手は銃に付いたまま
// c は最後に目へ合わせ直すので、走り⇔覗くの切り替え中に両方が効いていても銃は目へ向かう
// 目の位置は前の段（EyePoint）が決めたものを読む
class WeaponPlacePose : public BonePoseModifier{
public:
	// rig: 骨の番号・持ち方・ピッチ・目の位置を読む。所有権は受け取らない。nullptr 不可。この段より長く生きていること
	explicit WeaponPlacePose( const FirstPersonRig* rig );

	void setHoldWeight( float weight );  // 両手持ちの効き具合（FirstPersonBody::setHoldWeight）
	void setCarryWeight( float weight ); // 胸元への寄せ具合（FirstPersonBody::setCarryWeight）
	void setAimWeight( float weight );   // 覗き具合（FirstPersonBody::setAimWeight）

	virtual void apply( std::vector< GameLib::Math::Matrix34 >* pose ) override;

private:
	void holdWithBothHands( std::vector< GameLib::Math::Matrix34 >* pose ) const;
	void carryAtChest( std::vector< GameLib::Math::Matrix34 >* pose ) const;
	void alignSightToEye( std::vector< GameLib::Math::Matrix34 >* pose ) const;

	const FirstPersonRig* mRig;
	float mHoldWeight;
	float mCarryWeight;
	float mAimWeight;
};

#endif
