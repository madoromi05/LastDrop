#include "GameSrc/Render/Lighting/LightingParams.h"

#include "GameLib/Math/Functions.h"

using namespace GameLib::Math;

namespace{
	// 昼の値
	//   太陽：(300, 500, 400) の方から → 高さ 45°・方位 36.9°
	//   補助光：(-300, 200, -400) の方から → 高さ 21.8°・方位 -143.1°（夜と同じ向き）
	//   環境光：空の平均と地面をどちらも 0.4 前後にする（一様な灰色 0.4 と同じ明るさ）
	const float kDaySunElevation = 45.f;
	const float kDaySunAzimuth = 36.9f;
	const Vector3 kDaySunColor( 1.f, 0.95f, 0.9f );
	const float kDayDirectionalIntensity = 1.f;
	const Vector3 kDaySkyZenith( 0.35f, 0.45f, 0.6f );
	const Vector3 kDaySkyHorizon( 0.45f, 0.45f, 0.45f );
	const Vector3 kDayGround( 0.4f, 0.4f, 0.4f );
	// 昼の霧：町の端（約 110m）がわずかにかすむだけ。地面の霧は無し（朝もやは作らない）
	const float kDayFogHalfDistance = 200.f;
	const float kDayFogGroundDensity = 0.f;
}

LightingParams& lightingParams() {
	static LightingParams sParams; // 起動中ずっと 1 つ（F6 で書き換える）
	return sParams;
}

LightingParams dayLightingParams() {
	LightingParams day;
	day.moonElevation = kDaySunElevation; // 昼は「月」の枠に太陽を入れる
	day.moonAzimuth = kDaySunAzimuth;
	day.moonColor = kDaySunColor;
	day.moonIntensity = kDayDirectionalIntensity;
	day.fillIntensity = kDayDirectionalIntensity;
	day.skyZenithColor = kDaySkyZenith;
	day.skyHorizonColor = kDaySkyHorizon;
	day.groundColor = kDayGround;
	day.streetLightIntensity = 0.f; // 昼は街灯を消す（3-b までの昼の絵と同じにする）
	day.moonDiscIntensity = 0.f;    // 「月」の枠は太陽なので、月の絵は描かない
	day.moonHaloIntensity = 0.f;
	day.fogHalfDistance = kDayFogHalfDistance;
	day.fogGroundDensity = kDayFogGroundDensity;
	return day;
}

bool& fogEnabled() {
	static bool sEnabled = true; // 起動中ずっと 1 つ（F6 で書き換える）
	return sEnabled;
}

bool& skyDetailEnabled() {
	static bool sEnabled = true; // 起動中ずっと 1 つ（F6 で書き換える）
	return sEnabled;
}

bool& lightCullingEnabled() {
	static bool sEnabled = true; // 起動中ずっと 1 つ（F6 で書き換える）
	return sEnabled;
}

Vector3 lightDirectionFrom( float elevationDegree, float azimuthDegree ) {
	float horizontal = cos( elevationDegree );
	return Vector3( -horizontal * sin( azimuthDegree ), -sin( elevationDegree ), -horizontal * cos( azimuthDegree ) );	// 光の来る方の逆が、光の進む向き
}
