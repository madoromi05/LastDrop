#ifndef INCLUDED_LIGHTING_PARAMS_H
#define INCLUDED_LIGHTING_PARAMS_H

#include "GameLib/Math/Vector3.h"

// 夜の光（月・補助光・空・環境光・街灯・懐中電灯）の調整値。仕様は SummaryPlan/個別仕様/グラフィック/PbrNightLighting_Plan.txt の 7・8・9・11 章。
// 起動中に F6 のエディタ（Tuning）で動かせる。決まった値は Copy as C++ でここの初期値へ戻す（コードの値が正）。
// 初期値は夜（遊びの値）。昼の値は dayLightingParams()（見比べ・不具合探し用。F6 → Render の Day ボタン）。
// 色はどれも画像の色と同じ数（sRGB）。シェーダでリニアへ直してから強さを掛ける。
// 向きは「光が来る方」を、高さ（地平線から上へ何度）と方位（+Z から +X へ何度）で書く。
// どれも見た目を確かめる前の初期値。夜の暗さは「明かりが無くても 15m 先のゾンビの輪郭が分かる」（Q2）に合わせて画面で決める。
struct LightingParams{
	// ── 月（平行光）── 町全体をうっすら照らす。青白く弱い
	float moonElevation = 35.f;
	float moonAzimuth = 40.f;
	GameLib::Math::Vector3 moonColor = GameLib::Math::Vector3( 0.62f, 0.72f, 1.f );
	float moonIntensity = 0.25f;

	// ── 補助光（平行光）── 昼に影側を真っ黒にしない光。夜は使わない（強さ 0）
	float fillElevation = 21.8f;
	float fillAzimuth = -143.1f;
	GameLib::Math::Vector3 fillColor = GameLib::Math::Vector3( 0.35f, 0.35f, 0.4f );
	float fillIntensity = 0.f;

	// ── 空 ── 何も描かれていない所に、地平線の色から天頂の色へのグラデーションを描く（霧の色も地平線の色。下の霧）
	GameLib::Math::Vector3 skyZenithColor = GameLib::Math::Vector3( 0.02f, 0.03f, 0.07f );
	GameLib::Math::Vector3 skyHorizonColor = GameLib::Math::Vector3( 0.09f, 0.11f, 0.17f );
	float skyIntensity = 1.f;  // 描く空の明るさ（環境光には効かない）

	// ── 月の絵（MoonStarsLut_Plan.txt 4 章。星は 2026-10-31 に削除）── 空に足す絵だけ。町を照らす月の光（上の moon〜）の強さは変えない
	// 月は月の光の来る方（moonElevation・moonAzimuth）に描く（影の向きとずれない）。色は moonColor
	float moonRadius = 0.75f;        // 見かけの半径（度。本物の 0.25° は画面で小さすぎるので大きく）
	float moonDiscIntensity = 3.f;   // 円板の明るさ（トーンマップの前の数。ブルームのしきい値 1 を少し超えて、少しにじむ）
	float moonHaloIntensity = 0.05f; // 周りのにじみの明るさ（円板の明るさに対する割合。夜の薄い霧で光が散ったように見せる）
	float moonHaloFalloff = 8.f;     // にじみの広がり（月の半径の何倍で 1/e まで薄まるか）
	// ── 環境光 ── 上からは空の色、下からは地面の色（半球ライト）。金属に映る色も同じ空と地面
	GameLib::Math::Vector3 groundColor = GameLib::Math::Vector3( 0.05f, 0.05f, 0.06f );
	float ambientIntensity = 1.f;

	// ── 霧（NightFinish_Plan.txt 5 章）── 町の奥を空へ溶かし、足元をうっすら霞ませる。色は空の地平線の色（描いている空と同じ明るさ）
	// 距離の霧は 2 乗の指数（近くは薄いまま奥で急に濃くなる）。40m で半分のとき 15m 9%・80m 94%（夜の暗さの基準 15m を崩さない。Q2）
	float fogHalfDistance = 40.f;    // 距離の霧が半分になる距離（m）
	float fogGroundDensity = 0.015f; // 車道の面での高さの霧の濃さ（1/m）。這うゾンビが 10m 先で見えること（Q3）を上限にする
	float fogHeightHalf = 1.f;       // 高さの霧が半分に薄まる高さ（m）

	// ── 街灯（点光源。柱 1 本に 1 つ。TownMap::streetLights）── 道の明るい所・暗い所を作る（PbrNightLighting_Plan.txt 8 章）
	// 色はナトリウム灯風の暖かいオレンジ。強さは「真下（高さの距離）の地面で強さ / 高さ² の明るさ」になる
	// 2026-11-09 に柱の模型（ランタン型 3.86m）を置き、光をガラスの中へ移した。前より低く数も多い（16 → 38 本）ので、
	// 届く距離と強さを下げた（初期値。画面で合わせる。StreetLamp_Plan.txt 5 章）。
	float streetLightHeight = 3.29f; // 光の高さ（m。柱の足元〈歩道の面〉から。ランタンのガラスの真ん中。模型を変えたら合わせる）
	float streetLightRange = 12.f;   // 届く距離（m。光から。真下から横へ届くのは √(距離² − 高さ²) ≒ 11.5m）
	GameLib::Math::Vector3 streetLightColor = GameLib::Math::Vector3( 1.f, 0.72f, 0.42f );
	float streetLightIntensity = 12.f;

	// ── 懐中電灯（スポット。銃に付けて T で付ける。PbrNightLighting_Plan.txt 9 章）── 夜の FPS で一番効く光（段階 4 で影も付ける）
	// 光は銃口の少し前に置き、銃口の向きへ照らす（照準のぶれで銃が揺れると光も揺れる）。銃より前に置くので、銃そのものは照らさない
	float flashlightOffset = 0.05f;   // 銃口から前へ（m。後ろに置くと、点光源の近くの明るさで銃身が白く飛ぶ）
	float flashlightRange = 25.f;     // 届く距離（m）
	float flashlightInnerAngle = 12.f; // 中心からこの角度までは暗くならない（度。半角）
	float flashlightOuterAngle = 22.f; // 中心からこの角度で 0（度。半角）
	GameLib::Math::Vector3 flashlightColor = GameLib::Math::Vector3( 1.f, 0.96f, 0.88f ); // 白に少し暖かみ（LED より電球寄り）
	float flashlightIntensity = 200.f; // 10m 先で 強さ / 100 の明るさ（距離の 2 乗で弱まる）
	// 近さの下限（m。LightSource::nearDistance）。これより近い面は、この距離と同じ明るさ（強さ / 下限²）で止める。
	// 本物の懐中電灯は反射鏡で光の束を作るので、すぐ近くでは距離の 2 乗どおりに明るくならない。
	float flashlightNearDistance = 5.0f;

	// 項目ごとに v( 名前, 値, 下限, 上限, 単位 ) を呼ぶ（AimSwayParams と同じ約束）。
	template< class V > void visit( V& v ) {
		v( "moonElevation", moonElevation, 0.f, 90.f, "deg" );
		v( "moonAzimuth", moonAzimuth, -180.f, 180.f, "deg" );
		v( "moonColor", moonColor, 0.f, 1.f, "" );
		v( "moonIntensity", moonIntensity, 0.f, 4.f, "x" );
		v( "fillElevation", fillElevation, 0.f, 90.f, "deg" );
		v( "fillAzimuth", fillAzimuth, -180.f, 180.f, "deg" );
		v( "fillColor", fillColor, 0.f, 1.f, "" );
		v( "fillIntensity", fillIntensity, 0.f, 4.f, "x" );
		v( "skyZenithColor", skyZenithColor, 0.f, 1.f, "" );
		v( "skyHorizonColor", skyHorizonColor, 0.f, 1.f, "" );
		v( "skyIntensity", skyIntensity, 0.f, 8.f, "x" );
		v( "moonRadius", moonRadius, 0.1f, 5.f, "deg" );            // 0 は不可（半径で割るため）
		v( "moonDiscIntensity", moonDiscIntensity, 0.f, 20.f, "x" );
		v( "moonHaloIntensity", moonHaloIntensity, 0.f, 1.f, "x" );
		v( "moonHaloFalloff", moonHaloFalloff, 1.f, 32.f, "x" );     // 0 は不可（広がりで割るため）
		v( "groundColor", groundColor, 0.f, 1.f, "" );
		v( "ambientIntensity", ambientIntensity, 0.f, 8.f, "x" );
		v( "fogHalfDistance", fogHalfDistance, 5.f, 1000.f, "m" );    // 0 は不可（半分の距離で割るため）
		v( "fogGroundDensity", fogGroundDensity, 0.f, 0.2f, "/m" );
		v( "fogHeightHalf", fogHeightHalf, 0.1f, 20.f, "m" );          // 0 は不可（半分の高さで割るため）
		v( "streetLightHeight", streetLightHeight, 1.f, 12.f, "m" );
		v( "streetLightRange", streetLightRange, 1.f, 40.f, "m" );   // 0 は不可（届く距離で割るため）
		v( "streetLightColor", streetLightColor, 0.f, 1.f, "" );
		v( "streetLightIntensity", streetLightIntensity, 0.f, 100.f, "x" );
		v( "flashlightOffset", flashlightOffset, 0.f, 0.5f, "m" );
		v( "flashlightRange", flashlightRange, 1.f, 60.f, "m" );     // 0 は不可（届く距離で割るため）
		v( "flashlightInnerAngle", flashlightInnerAngle, 0.f, 60.f, "deg" ); // outer より大きくしないこと（LightList の spotLight）
		v( "flashlightOuterAngle", flashlightOuterAngle, 1.f, 80.f, "deg" );
		v( "flashlightColor", flashlightColor, 0.f, 1.f, "" );
		v( "flashlightIntensity", flashlightIntensity, 0.f, 400.f, "x" );
		v( "flashlightNearDistance", flashlightNearDistance, 0.05f, 5.f, "m" ); // 0 は不可（下限の 2 乗で割るため）
	}
};

// 起動中ずっと 1 つだけの調整値（LightingParams.cpp が持つ。初期値は夜）。
LightingParams& lightingParams();
// 昼の値（段階 3-b までの太陽・補助光・環境光 0.4 と同じ明るさ。空は青）。見比べ用で、遊びの中では使わない。
LightingParams dayLightingParams();
// 霧を掛けるか（F6 → Render の Fog。見比べと計測用。NightFinish_Plan.txt 9・11 章）。false の間は霧のパスを描かず、半透明の板にも掛けない。
// 起動中ずっと 1 つ（LightingParams.cpp が持つ。初期値 true。Release では true のまま）。
bool& fogEnabled();
// 月の絵を空に描くか（F6 → Render の Sky detail。見比べと計測用。MoonStarsLut_Plan.txt 8・10 章）。false の間は今までの空のグラデーションだけ。
// 起動中ずっと 1 つ（LightingParams.cpp が持つ。初期値 true。Release では true のまま）。
bool& skyDetailEnabled();
// 高さと方位（度）から、その方から来る光の進む向き（長さ 1。LightList の平行光に渡す形）。
GameLib::Math::Vector3 lightDirectionFrom( float elevationDegree, float azimuthDegree );

#endif
