#ifndef INCLUDED_DAMAGE_INFO_H
#define INCLUDED_DAMAGE_INFO_H

#include "GameLib/Math/Vector3.h"
#include "GameSrc/Hit/BodyPart.h"

// 1発ぶんの受けたダメージ。撃った側（弾）が決める値だけを入れる
// 部位ごとの倍率は受ける側が持つので、ここには倍率を掛ける前の量を入れる（弾を増やしても、受ける側を増やしても、直すのは片方だけ）
struct DamageInfo{
	BodyPart part;                // 当たった部位
	float baseAmount;             // 倍率を掛ける前のダメージ（弾の種類で決まる）
	GameLib::Math::Vector3 point; // 当たった位置（ワールド座標。血などの演出を出す場所）
	GameLib::Math::Vector3 direction; // 弾の進む向き（ワールド・長さ1。のけぞる向き）
};

#endif
