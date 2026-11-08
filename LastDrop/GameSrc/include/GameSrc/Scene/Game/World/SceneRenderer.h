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
#include "GameSrc/Scene/Game/World/SceneLights.h"

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
// レンダ系の部品（PostProcess・G-Buffer・ライティング・SSAO・SSR・TAA）と WorldRenderer、ライトと影の係（SceneLights）を所有する。
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
	// ライトの一覧と影のマップは SceneLights（2026-11-09 に分けた。MidPrioritySplit_Plan.txt 1 章）。
	// 目から見る絵の、描くかどうか（描く距離は霧から）。
	DrawCulling cameraCulling( const FirstPersonCamera& camera ) const;

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
	// ライトの一覧と影のマップ（mWorld と mRenderer を借りるので、その後に宣言する）。
	SceneLights mLights;
};

#endif
