#ifndef INCLUDED_SCENE_LIGHTS_H
#define INCLUDED_SCENE_LIGHTS_H

#include "GameSrc/Render/Lighting/LightList.h"
#include "GameSrc/Render/Lighting/LightingParams.h"
#include "GameSrc/Render/Shadow/ShadowMap.h"
#include "GameSrc/Render/Shadow/CascadedShadowMap.h"
#include "GameLib/Math/Matrix44.h"

class GameWorld;
class WorldRenderer;
class FirstPersonCamera;
class LightComponent;

// 1 回の描画の、ライトの一覧と影のマップを作る係（SummaryPlan/個別仕様/設計と分割/MidPrioritySplit_Plan.txt 1 章）
// 一覧に足すのは：月（平行光。CSM の影 3 枚）・補助光・物の光（懐中電灯〈スポット。影 1 枚〉・銃口の光・街灯）・空と環境光と霧と月の絵
// 月・補助光・空は調整値（LightingParams。初期値は夜）から毎フレーム作る（F6 ですぐ効くように）
// 物の光は、世界の物に付いた部品（LightComponent・MuzzleFlashComponent）を全部たどって集める
// どの物が光るかは知らない。光る物を足すときは、物を組み立てる所で部品を付ける（SummaryPlan/個別仕様/設計と分割/LightComponent_Plan.txt）
// 今日の条件（停電・霧の日）は世界から読む。描く手順（G-Buffer → ライティング → …）は知らない（SceneRenderer が lights() を渡して進める）
class SceneLights{
public:
	// world: 物の光の部品・今日の条件を読む。renderer: 影を落とす物を影のマップへ描く
	// どちらも所有権は受け取らない。nullptr 不可。これより長く生きていること
	SceneLights( const GameWorld* world, WorldRenderer* renderer );
	SceneLights( const SceneLights& ) = delete;
	SceneLights& operator=( const SceneLights& ) = delete;

	void setup( const FirstPersonCamera& camera );			// 描画の設定と、今のフレームのライトの一覧を作って付ける（G-Buffer の前）。月の影の 3 枚の行列もここで決める
	void drawShadowMaps( const FirstPersonCamera& camera );	// 影のマップを描く（懐中電灯・月の 3 枚）。setup の後、G-Buffer の前。終わったら描く行列を camera へ戻す
	// 今のフレームのライトの一覧（ライティングのパスと半透明の板が b2 から読む。付け直し・外すのは描く手順の側）
	LightList& lights();
	float fogHalfDistance() const;							// 今日の霧が半分になる距離（調整値 × 今日の条件）。描く距離を霧で縮めるのにも使う
	const char* shadowErrorMessage() const;					// 影のシェーダの誤り（懐中電灯 → 月の順に、最初に見つけた物。無ければ ""）

private:
	void addMoon( const LightingParams& params, const FirstPersonCamera& camera );	// 月（平行光）を一覧へ足す。強さがあれば、影の 3 枚（CSM）の行列を決めて影のマップを付ける
	// 物に付いた光を一覧へ足す。足す順は、影を落とす光（懐中電灯）→ 銃口の光 → 残り（街灯）
	// 照らす数の上限を選ぶとき、届く範囲の内にいる光どうしは足した順で残るので、手元の光を先に足す
	void addObjectLights();
	void addShadowedLight( const LightComponent& light );							// 影を落とす種類の光を足す。影のマップは 1 枚だけなので最初の光に付け、残りは影なしで足す

	const GameWorld* mWorld;   // 借りているだけ
	WorldRenderer* mRenderer;  // 借りているだけ
	LightList mLights;
	ShadowMap mFlashlightShadow;
	bool mFlashlightCastsShadow;
	GameLib::Math::Matrix44 mFlashlightShadowMatrix;
	CascadedShadowMap mMoonShadow;
	bool mMoonCastsShadow;
};

#endif
