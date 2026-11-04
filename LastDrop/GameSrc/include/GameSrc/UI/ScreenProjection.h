#ifndef INCLUDED_SCREEN_PROJECTION_H
#define INCLUDED_SCREEN_PROJECTION_H

#include "GameLib/Math/Vector2.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix44.h"

// 世界の位置を、UiRenderer が使う画面の位置（画素。左上が原点、下が +y）へ直す。状態は持たない。
// 世界の物の上に UI を出す所（ゾンビの頭上の体力のバー・使える物のマーク）が同じ直し方をするので、ここ 1 か所にまとめる。
class ScreenProjection{
public:
	// projectionView: カメラの射影×ビュー（FirstPersonCamera::projectionViewMatrix）。world: 世界の位置。out: 出力先。nullptr 不可。
	// カメラの後ろなら false（out は変えない）。画面の外でも true（呼ぶ側が画面に入るかを決める）。
	static bool worldToScreen( const GameLib::Math::Matrix44& projectionView, const GameLib::Math::Vector3& world, GameLib::Math::Vector2* out );
};

#endif
