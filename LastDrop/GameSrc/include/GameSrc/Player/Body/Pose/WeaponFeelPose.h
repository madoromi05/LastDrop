#ifndef INCLUDED_WEAPON_FEEL_POSE_H
#define INCLUDED_WEAPON_FEEL_POSE_H

#include "GameSrc/Render/SkinnedMesh/BonePoseModifier.h"

struct FirstPersonRig;

// 一人称の体の書き換えの段 4：置き終えた銃を、手触りのぶん動かす（銃が無ければ何もしない）。この順に掛ける：
//  a. 照準のぶれ（AimSway）のぶん、目の少し後ろ（肩の付け根あたり）を中心に両腕と銃を回す。目とカメラは動かさないので、
//     覗いているとサイトが画面の中で少し動いて見える。弾の向き（FirstPersonCamera::weaponDirection）も同じ角度だけぶれている。
//  b. 撃った反動のぶん、銃を後ろへ引いて銃口を跳ね上げる。両腕をまとめて動かすので手は銃から離れない。
//     目は動かさない（照準と弾の向きは変わらず、銃の見た目だけが跳ねる）。
// 銃を置く段（WeaponPlacePose）の後に掛ける。前に掛けると、覗く補正で合わせ直されて消える。
class WeaponFeelPose : public BonePoseModifier{
public:
	// rig: 骨の番号・持ち方・ピッチ・目の位置を読む。所有権は受け取らない。nullptr 不可。この段より長く生きていること。
	explicit WeaponFeelPose( const FirstPersonRig* rig );

	void setSway( float yawDegree, float pitchDegree ); // 照準のぶれ（FirstPersonBody::setSway）
	void setRecoil( float kick );                       // 反動の量（FirstPersonBody::setRecoil）
	void setAimWeight( float weight );                  // 覗き具合（反動の大きさを、腰だめと覗くの間で取る）

	virtual void apply( std::vector< GameLib::Math::Matrix34 >* pose ) override;

private:
	void swayWeapon( std::vector< GameLib::Math::Matrix34 >* pose ) const;
	void kickWeapon( std::vector< GameLib::Math::Matrix34 >* pose ) const;

	const FirstPersonRig* mRig;
	float mSwayYaw;   // 照準のぶれ（度）
	float mSwayPitch;
	float mRecoil;
	float mAimWeight;
};

#endif
