#ifndef INCLUDED_LIGHT_LIST_H
#define INCLUDED_LIGHT_LIST_H

#include "GameLib/Math/Vector3.h"
#include "GameLib/Graphics/ConstantBuffer.h"
#include "GameLib/Graphics/DepthTarget.h"
#include "GameLib/Math/Matrix44.h"
#include <vector>

class ViewFrustum;

// ライトの種類（PbrNightLighting_Plan.txt 6 章）。番号は Resource/Shader/LightList.hlsli と揃える
enum class LightKind{
	Directional, // 平行光（月・太陽）。向きだけ。減衰しない
	Point,       // 点光源。距離の 2 乗で弱まり、届く距離でなめらかに 0
	Spot,        // スポット。点光源の減衰 × 内側と外側の角度の間でなめらかに暗く
};

// 1 つのライト。作るときは下の directionalLight / pointLight / spotLight を使う（種類ごとに要る値だけ渡す）
struct LightSource{
	LightKind kind = LightKind::Point;
	GameLib::Math::Vector3 position;  // 点・スポット（ワールド）
	GameLib::Math::Vector3 direction; // 光の進む向き（長さ 1）。平行光・スポット
	GameLib::Math::Vector3 color;     // 画像の色と同じ数（sRGB）。シェーダでリニアへ直してから強さを掛ける
	float intensity = 0.f;            // 強さ（光の単位は使わない。見た目で決める。Pbr.hlsli の「π 掛け済み」の扱い）
	float range = 0.f;                // 点・スポット：届く距離（m）。0 より大きいこと
	float innerAngleDegree = 0.f;     // スポット：中心からこの角度までは減らない（半角）
	float outerAngleDegree = 0.f;     // スポット：中心からこの角度で 0（半角。inner より大きいこと）
	// 点・スポット：近さの下限（m。0 より大きいこと）。これより近い面は、この距離にあるものとして明るさを計算する（距離の 2 乗で弱まるので、
	// すぐ近くの面が何百倍にも明るくなり白く飛ぶのを止める）。初期値 10cm は、銃口の 5cm 前のマズルフラッシュで銃が真っ白にならない距離
	float nearDistance = 0.1f;
	int shadowMap = -1;              // 影を付けるなら、LightList::setShadowMap で設定した影のマップの番号（無ければ −1）
	int shadowMapCount = 1;           // 影のマップの枚数。2 以上なら shadowMap から続く枚を距離で選ぶ（月の CSM。LightList::setCascades）
};

// direction：光の進む向き（長さ 1）
LightSource directionalLight( const GameLib::Math::Vector3& direction, const GameLib::Math::Vector3& color, float intensity );
LightSource pointLight( const GameLib::Math::Vector3& position, const GameLib::Math::Vector3& color, float intensity, float range );
// direction：照らす向き（長さ 1）。角度は中心からの半角（度）
LightSource spotLight( const GameLib::Math::Vector3& position, const GameLib::Math::Vector3& direction,
	const GameLib::Math::Vector3& color, float intensity, float range, float innerAngleDegree, float outerAngleDegree );

// 空と環境光と霧（PbrNightLighting_Plan.txt 7 章・NightFinish_Plan.txt 5 章）。色は sRGB（シェーダでリニアへ直す）
//   空：何も描かれていない所に、地平線の色から天頂の色へのグラデーションを描く（明るさ skyIntensity）
//   環境光：上からは空の色・下からは地面の色（半球ライト。強さ ambientIntensity）。金属に映る色も同じ
//   霧：距離の霧（2 乗の指数）＋高さの霧。色は空の地平線の色（明るさ skyIntensity）。初期値は霧なし
struct EnvironmentLight{
	GameLib::Math::Vector3 skyZenithColor;
	GameLib::Math::Vector3 skyHorizonColor;
	GameLib::Math::Vector3 groundColor;
	float skyIntensity = 0.f;
	float ambientIntensity = 0.f;
	float fogHalfDistance = 0.f;  // 距離の霧が半分になる距離（m）。0 以下なら距離の霧なし
	float fogGroundDensity = 0.f; // 高さ 0 での高さの霧の濃さ（1/m）。0 なら高さの霧なし
	float fogHeightHalf = 0.f;    // 高さの霧が半分に薄まる高さ（m）。0 以下なら高さで薄まらない
	// 空に描く月の絵。初期値は描かない
	GameLib::Math::Vector3 moonDirection; // 目から月の方への向き（長さ 1。光の進む向きの逆）
	GameLib::Math::Vector3 moonColor;
	float moonRadius = 0.f;        // 見かけの半径（度）
	float moonDiscIntensity = 0.f; // 円板の明るさ。0 なら月を描かない（水たまりにも映らない）
	float moonHaloIntensity = 0.f; // にじみの明るさ（円板の明るさに対する割合）
	float moonHaloFalloff = 1.f;   // にじみの広がり（月の半径の何倍で 1/e）。0 より大きいこと
	float pixelAngle = 0.f;        // 画面の 1 画素の角度（度。縦の画角 ÷ 画面の高さ）。月の模様を読む段（ミップ）を選ぶ
};

// 1 フレームに照らすライトと空・環境光を集めて、シェーダの定数（b2。Resource/Shader/LightList.hlsli）へ渡す
// 使い方：clear → add を何回か（月・街灯・マズルフラッシュ…）と setEnvironment → bind（目の位置）→ ライティングのパス・半透明の板を描く → unbind
// シェーダが一度に照らせるのは kMaxLights 個まで。超えたら、平行光は全部残し、残りは目に近い順に選ぶ
// （届く距離の外にいるほど後回し。遠い街灯から消える）
// 作るのは Graphics::Manager ができた後（シーンの中）
class LightList{
public:
	static const int kMaxLights = 30; // LightList.hlsli の kMaxLights と揃える
	static const int kMaxShadowMaps = 4; // LightList.hlsli の kMaxShadowMaps と揃える（懐中電灯 1 ＋ 月の 3 枚）
	static const int kCascadeCount = 3;  // 月の CSM の枚数（LightList.hlsli の gCascadeEnds と揃える。CascadedShadowMap::kCascadeCount と同じ）

	LightList();
	LightList( const LightList& ) = delete;
	LightList& operator=( const LightList& ) = delete;

	void clear();
	void add( const LightSource& );
	void setEnvironment( const EnvironmentLight& );	// 空と環境光。clear では消えない（最後に設定した値のまま。GameScene は毎フレーム調整値から設定し直す）
	// 影のマップ。index：0〜kMaxShadowMaps−1（ライトの shadowMap に入れる番号。シェーダの t6 + index）
	// projectionView：その影のマップを描いた光の側の射影×ビュー。texelSize：影のマップ 1 画素の大きさ（m）
	// perspective：true なら透視（スポット。texelSize は光から 1m の所の大きさで、距離を掛けて使う）、false なら正射影（月）
	// clear で全部外れる
	void setShadowMap( int index, GameLib::Graphics::DepthTarget depth, const GameLib::Math::Matrix44& projectionView,
		float texelSize, bool perspective );
	void setShadowSettings( float normalOffsetTexels, float lightOffsetTexels, float strength );	// 影を読む位置のずらしと濃さ（ShadowParams の値）。clear では消えない
	// 月の CSM の枚の選び方。viewDirection：カメラの向き（長さ 1。奥行きはこの向きに沿って測る）
	// cascadeEnds：各枚の受け持ちの奥の端（m）。blendRatio：各枚の受け持ちの奥の何割を、次の枚と混ぜるか（境目を目立たせない）
	void setCascades( const GameLib::Math::Vector3& viewDirection, const float ( &cascadeEnds )[ kCascadeCount ], float blendRatio );
	// 照らすライトを選んで定数を書き、b2 に付ける。eye：目の位置（選ぶ順番に使う）。projectionView：描くカメラの射影×ビュー
	// culling：true なら、映る範囲に届かないライトを外し、シェーダも届く距離の外の画素で計算しない（F6 の Light culling）
	void bind( const GameLib::Math::Vector3& eye, const GameLib::Math::Matrix44& projectionView, bool culling );
	// 影のマップを t6〜 に付ける。描画先を変える（Manager::setRenderTargets）と t2 以降は外れるので、
	// 影を読む物（ライティングのパス・半透明の板）を描く前に、そのつど呼ぶ
	void bindShadowMaps() const;
	void rebindConstants() const;																	// 最後に bind で書いた定数を、選び直さずにもう一度 b2 に付ける（unbind の後に、F6 の影の表示を描くとき）
	void unbind();																					// b2 と影のマップを外す（後から描く物・ほかのシーンに残さない）

	// 足したライト（足した順）と、最後の bind で照らす物に選ばれたか（F6 の Zombie info の表示用。LightDebugDrawer）
	const std::vector< LightSource >& lights() const;
	bool isSelected( int index ) const;

private:
	void select( const GameLib::Math::Vector3& eye, const ViewFrustum* view );	// mLights から照らす物を選び、mSelected に番号を入れる（平行光 → 目に近い順）。view が nullptr でなければ、その範囲に届かない物は入れない

	std::vector< LightSource > mLights;
	EnvironmentLight mEnvironment;
	GameLib::Graphics::DepthTarget mShadowDepths[ kMaxShadowMaps ]; // 空なら使っていない
	GameLib::Math::Matrix44 mShadowMatrices[ kMaxShadowMaps ];
	float mShadowTexelSizes[ kMaxShadowMaps ];
	bool mShadowPerspective[ kMaxShadowMaps ];
	float mShadowNormalOffset;  // 影を読む位置のずらし（画素）と影の濃さ（setShadowSettings）
	float mShadowLightOffset;
	float mShadowStrength;
	GameLib::Math::Vector3 mCascadeViewDirection; // setCascades の値
	float mCascadeEnds[ kCascadeCount ];
	float mCascadeBlendRatio;
	std::vector< int > mSelected; // 毎フレーム作り直す（確保し直さないよう持っておく）
	GameLib::Graphics::ConstantBuffer mConstants;
};

#endif
