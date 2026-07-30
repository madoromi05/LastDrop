#include "Framework.h"
#include "Arena.h"
#include <limits>
using namespace std;

const float Arena::mHalfExtent = 50.0f; // stage.txt の床の半径に一致させる。
const float Arena::mWallHeight = 40.0f; // 壁・天井の高さ。ワイヤーを撃ち上げても届く高さにする。

bool Arena::clampInside( Vector3* position, float radius ) const {
	bool onGround = false;
	const float limit = mHalfExtent - radius; // 壁厚ぶんは半径で内側に寄せる。
	if (position->x < -limit) { position->x = -limit; }
	if (position->x >  limit) { position->x =  limit; }
	if (position->z < -limit) { position->z = -limit; }
	if (position->z >  limit) { position->z =  limit; }
	if (position->y < 0.0f) { // 床を突き抜けない。
		position->y = 0.0f;
		onGround = true;
	}
	if (position->y > mWallHeight) { position->y = mWallHeight; } // 天井で止める。
	return onGround;
}

namespace {
	// 1枚の軸並行平面との交点候補を評価し、より手前なら best を更新する共通処理。
	// axis：平面の法線が向く軸（0=x,1=y,2=z）／planeValue：その軸上の平面位置。
	// u,v：面の広がりを判定する残り2軸と、その許容範囲 [uMin,uMax] [vMin,vMax]。
	void testPlane(
		const Vector3& origin,
		const Vector3& dir,
		float maxDistance,
		int axis, float planeValue,
		int u, float uMin, float uMax,
		int v, float vMin, float vMax,
		float* bestT,
		Vector3* bestHit ) {
		float d = dir[ axis ];
		if (d == 0.0f) { return; }                    // 平面と平行なら交わらない。
		float t = ( planeValue - origin[ axis ] ) / d;
		if (t <= 0.0f || t > maxDistance) { return; } // 後方・遠すぎは無視。
		if (t >= *bestT) { return; }                  // 既により手前の交点がある。
		// 交点が面の矩形範囲に収まっているか確認する。
		float uHit = origin[ u ] + dir[ u ] * t;
		float vHit = origin[ v ] + dir[ v ] * t;
		if (uHit < uMin || uHit > uMax) { return; }
		if (vHit < vMin || vHit > vMax) { return; }
		*bestT = t;
		( *bestHit )[ axis ] = planeValue;
		( *bestHit )[ u ] = uHit;
		( *bestHit )[ v ] = vHit;
	}
}

bool Arena::raycast(
	const Vector3& origin,
	const Vector3& dir,
	float maxDistance,
	Vector3* outHit ) const {
	float bestT = numeric_limits< float >::max();
	Vector3 hit( 0.0f, 0.0f, 0.0f );
	const float h = mHalfExtent;
	const float wy = mWallHeight;
	// 床（y=0）・天井（y=wy）。広がりは x,z（-h..h）。
	testPlane( origin, dir, maxDistance, 1, 0.0f, 0, -h, h, 2, -h, h, &bestT, &hit );
	testPlane( origin, dir, maxDistance, 1, wy,   0, -h, h, 2, -h, h, &bestT, &hit );
	// 壁 x=±h。広がりは z（-h..h）と y（0..wy）。
	testPlane( origin, dir, maxDistance, 0, -h, 2, -h, h, 1, 0.0f, wy, &bestT, &hit );
	testPlane( origin, dir, maxDistance, 0,  h, 2, -h, h, 1, 0.0f, wy, &bestT, &hit );
	// 壁 z=±h。広がりは x（-h..h）と y（0..wy）。
	testPlane( origin, dir, maxDistance, 2, -h, 0, -h, h, 1, 0.0f, wy, &bestT, &hit );
	testPlane( origin, dir, maxDistance, 2,  h, 0, -h, h, 1, 0.0f, wy, &bestT, &hit );
	if (bestT == numeric_limits< float >::max()) {
		return false;
	}
	*outHit = hit;
	return true;
}
