#ifndef INCLUDED_FIRST_PERSON_BODY_COMPONENT_H
#define INCLUDED_FIRST_PERSON_BODY_COMPONENT_H

#include "GameLib/Math/Vector3.h"
#include "GameSrc/World/Component.h"
#include "GameSrc/Player/FirstPersonBody.h"

class SkinnedMeshData;
class SkinnedModelComponent;

// 一人称の体の見せ方（視線に合わせた背骨の曲げ・覗くときに銃を目の前へ寄せる・撃った反動で銃を跳ねさせる・頭を消す）。
// 中身は FirstPersonBody（委譲）。start で同じ物の見た目（SkinnedModelComponent）へ、描く直前の姿勢の書き換えとして付ける。
// 何をどれだけ効かせるか（ピッチ・覗き具合・反動…）は、自キャラを動かす部品（PlayerComponent・HoldComponent）が毎ティック渡す。
class FirstPersonBodyComponent : public Component{
public:
	// data: 骨を引く形。所有権は受け取らない。nullptr 不可。この部品より長く生きていること。
	explicit FirstPersonBodyComponent( const SkinnedMeshData* data );

	FirstPersonBody& body();
	const FirstPersonBody& body() const;
	// 目の位置（ワールド座標。カメラの視点）。頭の骨に付いていくので、歩き・走りの揺れがそのまま出る。
	// 見た目の update の後に呼ぶ（その update で見せたポーズの頭の位置を返す）。
	GameLib::Math::Vector3 eyePosition() const;
	// 照準のぶれ（度。FirstPersonCamera::setWeaponSway と同じ角度）。次の update で銃の見た目に入る（1 ティック遅れ。反動の見た目と同じ）。
	void setWeaponSway( float yawDegree, float pitchDegree );

	// Component（SkinnedModelComponent が要る）
	virtual void start();

private:
	FirstPersonBody mBody;
	const SkinnedModelComponent* mModel;
};

#endif
