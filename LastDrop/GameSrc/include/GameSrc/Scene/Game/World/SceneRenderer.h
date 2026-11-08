#ifndef INCLUDED_SCENE_RENDERER_H
#define INCLUDED_SCENE_RENDERER_H

#include "GameSrc/Scene/Game/World/WorldRenderer.h"
#include "GameSrc/Render/PostProcess/PostProcess.h"
#include "GameSrc/Render/Lighting/GBuffer.h"
#include "GameSrc/Render/Lighting/DeferredLighting.h"
#include "GameSrc/Render/PostProcess/AmbientOcclusion.h"
#include "GameSrc/Render/PostProcess/ScreenSpaceReflection.h"
#include "GameSrc/Render/PostProcess/TemporalJitter.h"
#include "GameSrc/Render/PostProcess/TemporalAntiAliasing.h"
#include "GameSrc/Render/Lighting/LightList.h"
#include "GameSrc/Render/Shadow/ShadowMap.h"
#include "GameSrc/Render/Shadow/CascadedShadowMap.h"
#include "GameSrc/Render/Lighting/LightingParams.h"
#include "GameLib/Math/Matrix44.h"

class GameWorld;
class FirstPersonCamera;
class DrawCulling;
#ifdef ZF_EDITOR
class GameDebugOverlay;
#endif

// 1 回の描画の、世界を画面へ写すところ（ディファードのパイプライン）を受け持つ。
// GameScene は「どのカメラで描くか」を決めてここへ渡すだけ。UI・HUD・状態（一時停止/カバン/漁り）は GameScene に残る。
// 中でやること（GBuffer_Plan.txt〜Taa_Plan.txt）: ライトと影のマップを作る → 不透明を G-Buffer へ → SSAO → ライティング →
//   映り込み（SSR）→ 霧 → TAA → 砂ぼこり・血・マズルフラッシュ → （Debug の当たり判定）→ 画面へ写す → G-Buffer の表示。
// レンダ系の部品（PostProcess・G-Buffer・ライティング・SSAO・SSR・TAA・ライトの一覧・影のマップ）と WorldRenderer を所有する。
class SceneRenderer{
public:
	// 攻撃された・死んだ後の見せ方のうち、画面全体に掛ける分（PlayerDamageView から。PostProcess へ渡す）。
	struct DamagePostEffect{
		float flash; // 攻撃された瞬間の赤み
		float pain;  // 痛みのにじみ
		float fade;  // 死んだ後の暗転
	};
	// world: 描く世界。所有しない。nullptr 不可。この描画より長く生きていること。
	explicit SceneRenderer( GameWorld* world );
	// このフレームのディファード描画を全部行う。camera: GameScene が置いた、描くカメラ（観察用カメラの間はそちら）。
	// damage: 画面全体の赤み・痛み・暗さ。debug: 当たり判定・ライトの表示（Debug/Profile だけ。nullptr 可）。
	void render( const FirstPersonCamera& camera, const DamagePostEffect& damage
#ifdef ZF_EDITOR
		, GameDebugOverlay* debug
#endif
	);
	// パイプラインのシェーダの誤りを、写す→キャラ→G-Buffer→表示の順で最初に見つけた物だけ返す（無ければ ""）。
	// skinningError: キャラのスキニング（GameResources が持つ）の誤り。順番の都合でここへ渡す。
	const char* firstShaderError( const char* skinningError ) const;

private:
	// ── setupLights〜cameraCulling は render から呼ぶ（旧 GameScene から移動）──
	// 描画の設定と、今のフレームのライトの一覧（月・補助光・懐中電灯・マズルフラッシュ・街灯・空・環境光）を作って付ける。
	void setupLights( const FirstPersonCamera& camera );
	// 月（平行光）を一覧へ足す。強さがあれば、影の 3 枚（CSM）の行列を決めて影のマップを付ける。
	void addMoon( const LightingParams& params, const FirstPersonCamera& camera );
	// 懐中電灯が付いていて銃を持っていれば、銃口の前から照らすスポットを一覧へ足す。
	void addFlashlight( const LightingParams& params );
	// 影のマップを描く（懐中電灯・月の 3 枚）。setupLights の後、G-Buffer の前。終わったら描く行列を camera へ戻す。
	void drawShadowMaps( const FirstPersonCamera& camera );
	// 目から見る絵の、描くかどうか（描く距離は霧から）。
	DrawCulling cameraCulling( const FirstPersonCamera& camera ) const;
	// 今日の霧が半分になる距離（調整値 × 今日の条件）。
	float fogHalfDistance() const;

	GameWorld* mWorld;    // 借りているだけ
	WorldRenderer mRenderer; // 世界を描く（mWorld を読む）
	PostProcess mPostProcess;
	GBuffer mGBuffer;
	DeferredLighting mDeferredLighting;
	AmbientOcclusion mAmbientOcclusion;
	ScreenSpaceReflection mReflection;
	TemporalJitter mJitter;
	TemporalAntiAliasing mTaa;
	const FirstPersonCamera* mTaaCamera; // 前のフレームに見ていたカメラ（替わったら TAA の履歴を捨てる）。目印だけで読まない
	LightList mLights;
	ShadowMap mFlashlightShadow;
	bool mFlashlightCastsShadow;
	GameLib::Math::Matrix44 mFlashlightShadowMatrix;
	CascadedShadowMap mMoonShadow;
	bool mMoonCastsShadow;
};

#endif
