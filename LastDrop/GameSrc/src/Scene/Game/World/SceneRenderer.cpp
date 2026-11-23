#include "GameSrc/Scene/Game/World/SceneRenderer.h"
#include "GameSrc/Scene/Game/World/GameWorld.h"
#include "GameSrc/Core/GameSettings.h"
#include "GameSrc/Player/Camera/FirstPersonCamera.h"
#include "GameSrc/Render/Culling/DrawCulling.h"
#include "GameSrc/Render/Shadow/ShadowParams.h"
#include "GameSrc/Render/PostProcess/SsrParams.h"
#include "GameSrc/Render/PostProcess/PostProcessParams.h"
#include "GameSrc/Map/TownVisibility.h"
#ifdef ZF_EDITOR
#include "GameSrc/Scene/Game/Hud/GameDebugOverlay.h"
#endif

#include "GameLib/GameLib.h"
#include "GameLib/Graphics/Manager.h"
#include "GameLib/Graphics/Enum.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Functions.h"
#include "GameLib/Profiling/Profiler.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const float kNoDrawDistance = 0.f;      // 霧を切っている間は、距離で間引かない（DrawCulling::forCamera）
}

SceneRenderer::SceneRenderer( GameWorld* world, const GameSettings* settings ) :
	mWorld( world ),
	mSettings( settings ),
	mRenderer( world, &world->effects() ),
	mTaaCamera( nullptr ),
	mLights( world, &mRenderer ) {
}

void SceneRenderer::render( const FirstPersonCamera& camera, const DamagePostEffect& damage
#ifdef ZF_EDITOR
	, GameDebugOverlay* debug
#endif
) {
	// TAA のときは、描く行列を 1 画素より細かく毎フレームずらす。カメラが置いた行列を、ずらした物で上書きし、
	// 下の G-Buffer・SSAO・SSR・霧・デカール・半透明の板にも同じ行列を渡す（全部のパスが同じずれで揃う）
	// 映る範囲（DrawCulling）と影はずらさない（1 画素より細かいずれは効かない／影は光の側から見た絵）
	mJitter.advance( antiAliasingMode() == AntiAliasing::Taa );
	const Matrix44 projectionView = mJitter.apply( camera.projectionViewMatrix(),
		mPostProcess.sceneDepth().width(), mPostProcess.sceneDepth().height() );
	Graphics::Manager::instance().setProjectionViewMatrix( projectionView );
	beginScene( damage );
	mLights.setup( camera ); // ライトの一覧と影の行列（SceneLights）
	mLights.drawShadowMaps( camera );
	// 不透明な物は G-Buffer へ描き、全画面 1 回でライティングする
	// フォワードへの切り替えは置かない（PBR の式とライトを 2 か所に書かないため）
	const GBufferView view = gbufferView();
	const bool showsGBuffer = ( view != GBufferView::Final );
	// 映り込みの表示（SSR・SSR color）は、照らした後の色を映すので、表示中もライティングまで描く（表示は endScene の後に上から写す）
	const bool showsReflection = ( view == GBufferView::Ssr || view == GBufferView::SsrColor );
	const bool drawsLighting = !showsGBuffer || showsReflection;
	drawGBuffer( camera, projectionView );
	// SSAO は G-Buffer の後・ライティングの前。G-Buffer の表示（SSAO）でも見るので、表示中も作る
	mAmbientOcclusion.draw( mGBuffer, mPostProcess.sceneDepth(), projectionView, mJitter.frameIndex() );
	Profiling::gpuMark( "SSAO" );
	if ( drawsLighting ) {
		drawLighting( projectionView );
		drawReflection( projectionView );
		drawFog( projectionView );
		resolveTaa( camera, projectionView );
	} else {
		mTaa.invalidate(); // G-Buffer の表示の間は照らした色が無いので、履歴も続かない
	}
	mTaaCamera = &camera;
	drawTranslucent( camera, projectionView );
#ifdef ZF_EDITOR
	mPostProcess.resumeScene(); // 当たり判定の表示は深度を書くことがあるので、書ける付け方へ戻す
	debug->drawCollision( *mWorld, camera );
	debug->drawLights( mLights.lights(), camera );
#endif
	// GPU の時間の区切り（Perf の欄の GPU）。世界の物ごとの印は WorldRenderer の中（Player〜Ground）
	// ここまでが弾痕・砂ぼこり・血・マズルフラッシュ・当たり判定の表示、次の印までが画面全体の処理（ブルームなど）
	Profiling::gpuMark( "Effects" );
	mPostProcess.endScene();
	if ( showsGBuffer ) {
		drawGBufferView( view, projectionView );
	}
	Profiling::gpuMark( "GBufferView" );	// 「PostProcess」「FXAA」の印は endScene の中で打つ。ここまでは G-Buffer の表示（F6 で選んだときだけ。ふだんはほぼ 0）
}

void SceneRenderer::beginScene( const DamagePostEffect& damage ) {
	// 攻撃された瞬間の赤み・痛み・死んだ後の暗さは、この描画の写すときに効く
	mPostProcess.setDamageFlash( damage.flash );
	mPostProcess.setPain( damage.pain );
	mPostProcess.setFade( damage.fade );
	mPostProcess.setBrightnessExponent( mSettings->brightnessExponent() );	// 一時停止の設定画面で動かした値を、後ろの絵にすぐ見せる
	// SSAO・SSR・ディザのノイズは、TAA のときだけフレームごとに動かす（TAA が均す）。TAA でないときは番号が 0 のまま
	mPostProcess.setNoiseFrame( mJitter.frameIndex() );
	mPostProcess.beginScene();
}

void SceneRenderer::drawGBuffer( const FirstPersonCamera& camera, const Matrix44& projectionView ) {
	mGBuffer.begin( mPostProcess.sceneDepth() );
	// 視点を置いた後の行列から映る範囲を作り、映らない地面・建物を描かない
	// 動かない物 → 弾痕（デカール）→ 動く物の順。デカールは画素に見えている面へ書くので、
	// 後から描くキャラがデカールを上書きして、キャラには弾痕が乗らない
	{
		PROFILE_SCOPE( "World.draw" );
		const DrawCulling culling = cameraCulling( camera );
		mRenderer.drawStatic( culling, camera.eyePosition() );
		mGBuffer.beginDecals( mPostProcess.sceneDepth() );
		mRenderer.drawDecals( projectionView );
		Profiling::gpuMark( "Decals" );
		mGBuffer.endDecals( mPostProcess.sceneDepth() );
		// 動く物は動きの種類を G-Buffer へ書き分ける（TAA が前のフレームのどこを読むかを分ける）
		mGBuffer.setMotion( MotionKind::FirstPerson );
		mRenderer.drawFirstPerson();
		mGBuffer.setMotion( MotionKind::Moving );
		mRenderer.drawCharacters( culling );
	}
	mGBuffer.end();
}

void SceneRenderer::drawLighting( const Matrix44& projectionView ) {
	mPostProcess.bindSceneColor(); // 深度はシェーダから読むので、描画先からは外す
	mLights.lights().bindShadowMaps(); // 描画先を変えると t2 以降が外れるので、付け直す（LightList::bindShadowMaps）
	mDeferredLighting.drawLighting( mGBuffer, mAmbientOcclusion.result(), mPostProcess.sceneDepth(), projectionView );
	Profiling::gpuMark( "Lighting" );
}

void SceneRenderer::drawReflection( const Matrix44& projectionView ) {
	// 映り込みはライティングの後・霧の前（照らした色を映し、映り込みにも霧が掛かる）
	// 作るパスはシーンの色を読むので、自分の描画先へ描いてから、シーンの色へ付け直して混ぜる
	if ( ssrEnabled() ) {
		mReflection.buildHiZ( mPostProcess.sceneDepth() ); // F6 の Hi-Z を切っている間は作らない
		Profiling::gpuMark( "Hi-Z" );
		mReflection.draw( mGBuffer, mPostProcess.sceneColor(), mPostProcess.sceneDepth(), projectionView, mJitter.frameIndex() );
		Profiling::gpuMark( "SSR" );
		mPostProcess.bindSceneColor();
		mDeferredLighting.drawReflection( mGBuffer, mAmbientOcclusion.result(), mReflection.result(), mPostProcess.sceneDepth(),
			projectionView );
	} else {
		mReflection.clear(); // 表示（SSR）が前の絵を出さないように
		Profiling::gpuMark( "Hi-Z" ); // 切っていても打つ（計測の列を揃えるため）
		Profiling::gpuMark( "SSR" );
	}
	Profiling::gpuMark( "SSR composite" );
}

void SceneRenderer::drawFog( const Matrix44& projectionView ) {
	// 霧は半透明の板より前（板は自分で同じ霧を掛けるので、二重に掛けない）
	if ( fogEnabled() ) {
		mDeferredLighting.drawFog( mPostProcess.sceneDepth(), projectionView );
	}
	Profiling::gpuMark( "Fog" );
}

void SceneRenderer::resolveTaa( const FirstPersonCamera& camera, const Matrix44& projectionView ) {
	// TAA は霧の後・半透明の板の前（板は動きが取れず、マズルフラッシュに光の尾が出るため）
	// 見ているカメラが替わったフレーム（初めて描くとき）は、前の履歴が別の場所の絵なので捨てる
	if ( &camera != mTaaCamera ) {
		mTaa.invalidate();
	}
	if ( antiAliasingMode() == AntiAliasing::Taa ) {
		mTaa.resolve( mPostProcess.sceneColor(), mGBuffer, mPostProcess.sceneDepth(), projectionView, camera.projectionViewMatrix() );
	} else {
		mTaa.invalidate(); // TAA に戻したとき、古い履歴を混ぜないように
	}
	Profiling::gpuMark( "TAA" );
}

void SceneRenderer::drawTranslucent( const FirstPersonCamera& camera, const Matrix44& projectionView ) {
	// 砂ぼこりなどの半透明の板は、G-Buffer と同じ深度で隠れるようシーンへ描く。板は深度を書かないので読み取り専用で付け、
	// 同じ深度を読んで壁・地面に刺さった所を薄くする。弾痕は G-Buffer のデカールで描く（drawGBuffer）
	mPostProcess.resumeSceneReadOnlyDepth();
	mLights.lights().bindShadowMaps();   // 板も懐中電灯の影を受ける（同じ evaluateLight を通る）
	mRenderer.drawImpactDust( projectionView, camera.viewDirection(), camera.eyePosition() ); // 弾痕の上に舞う
	mRenderer.drawBloodSpray( projectionView, camera.viewDirection(), camera.eyePosition() ); // 砂ぼこりの後（同じ作り）
	mRenderer.drawMuzzleFlash( projectionView, camera.viewDirection() ); // 加算なので順番は問わない
	mLights.lights().unbind(); // ライトの一覧を読むのは、ここまで（ライティングのパスと半透明の板）
}

void SceneRenderer::drawGBufferView( GBufferView view, const Matrix44& projectionView ) {
	// ToneMap を通さず中身の数のまま見るので、画面へ写した後に上から描く
	// 影の表示（Cascade・Shadow maps）はライトの一覧の定数と影のマップを読むので、外した物をもう一度付けて描く
	mLights.lights().rebindConstants();
	mLights.lights().bindShadowMaps();
	mDeferredLighting.drawView( view, mGBuffer, mAmbientOcclusion.result(), mReflection.result(), mReflection.hiZ(),
		mPostProcess.sceneDepth(), projectionView );
	mLights.lights().unbind();
}

const char* SceneRenderer::firstShaderError( const char* skinningError ) const {
	// 画面に出せるのは1行なので、写す処理（PostProcess）→ キャラ（SkinningPipeline）→ G-Buffer → G-Buffer の表示の順に、
	// 先に見つかったものを出す
	const char* const shaderErrors[] = {
		mPostProcess.errorMessage(),
		skinningError,
		mGBuffer.errorMessage(),
		mDeferredLighting.errorMessage(),
		mAmbientOcclusion.errorMessage(),
		mReflection.errorMessage(),
		mTaa.errorMessage(),
		mLights.shadowErrorMessage(), // 懐中電灯 → 月の影
	};
	for ( const char* message : shaderErrors ) {
		if ( message[ 0 ] != '\0' ) {
			return message;
		}
	}
	return "";
}

DrawCulling SceneRenderer::cameraCulling( const FirstPersonCamera& camera ) const {
	float drawDistance = fogEnabled() ? DrawCulling::drawDistanceForFog( mLights.fogHalfDistance() ) : kNoDrawDistance; // 霧の日は描く距離も縮む
	// 小ささの画素は、G-Buffer へ描く絵（シーンの深度）の高さで数える
	// 隠れ（PVS）は目から見る絵だけ（影の culling には渡さない）
	return DrawCulling::forCamera( camera.projectionViewMatrix(), camera.eyePosition(), drawDistance, camera.fieldOfView(),
		mPostProcess.sceneDepth().height(), mWorld->town().visibilityIfEnabled() );
}
