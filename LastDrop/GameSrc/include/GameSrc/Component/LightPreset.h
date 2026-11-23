#ifndef INCLUDED_LIGHT_PRESET_H
#define INCLUDED_LIGHT_PRESET_H

#include "GameSrc/Render/Lighting/LightList.h"
#include "GameLib/Math/Vector3.h"

// 物に付ける光の種類（LightComponent へ渡す）
// 色・強さ・届く距離は部品へ写さず、光を作るたびに調整値（LightingParams）を読む（F6 で動かした値がすぐ効くように）
enum class LightPreset{
	StreetLamp,	// 街灯。柱の足元から決まった高さの点光源。始めから付いている
	Flashlight,	// 懐中電灯。光の出る所から前へ照らすスポット。始めは消えている。影を落とす
};

// 種類ごとの決まり。種類を足したら、この 4 つに足す
namespace LightPresets{
	bool startsOn( LightPreset preset );	// 始めから付いているか
	bool castsShadow( LightPreset preset );	// 影を落とすか
	bool needsSocket( LightPreset preset );	// 持たれている間だけ照らすか。true なら、光の出る所を答える相手が無い間は照らさない（地面に落とした銃の懐中電灯）
	// 光の出る所 position（ワールド）と向き direction（長さ1。点光源では使わない）から、今の調整値で光を作る
	LightSource make( LightPreset preset, const GameLib::Math::Vector3& position, const GameLib::Math::Vector3& direction );
}

#endif
