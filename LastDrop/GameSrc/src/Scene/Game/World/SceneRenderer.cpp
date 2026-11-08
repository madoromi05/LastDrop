#include "GameSrc/Scene/Game/World/SceneRenderer.h"
#include "GameSrc/Scene/Game/World/GameWorld.h"
#include "GameSrc/World/GameObject.h"
#include "GameSrc/Player/Body/HoldComponent.h"
#include "GameSrc/Component/LightComponent.h"
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
	// ── 影（Shadow_Plan.txt 5 章）──
	const int kFlashlightShadowSize = 1024; // 懐中電灯の影のマップの一辺（画素）
	const int kFlashlightShadowIndex = 0;   // ライトの一覧の影のマップの番号（1〜3 は月の CSM に空けておく）
	const bool kPerspectiveShadow = true;
	const bool kWithoutPlayer = false;      // 懐中電灯の影には自キャラを描かない（銃の影が画面の真ん中を埋めるため。Q4）
	// ── 月の影（CSM。Shadow_Plan.txt 6 章）──
	const int kMoonShadowSize = 2048;       // 1 枚の影のマップの一辺（画素。Q3）
	const int kMoonShadowFirstIndex = 1;    // ライトの一覧の影のマップの番号（1〜3 の 3 枚）
	const bool kOrthographicShadow = false;
	const bool kWithPlayer = true;          // 月の影には自キャラも描く（足元に自分の影が見える。Q4）
	const int kEveryFrame = 1;              // 月の影の手前の枚は毎フレーム描き直す（遅れると足元の影がずれて見える）
	const float kRoundToNearest = 0.5f;     // 調整値（float）の間隔を一番近い整数のフレームにする
	const float kMinCascadeGap = 0.5f;      // F6 で端を逆に並べても枚の範囲が 0 にならないよう、前の端よりこれだけ奥へ揃える（m）
	const float kNoDrawDistance = 0.f;      // 霧を切っている間は、距離で間引かない（DrawCulling::forCamera）
	const int kFirstCoarseLodCascade = 1;   // 月の影のこの枚から、置物を一番粗い段（LOD）で描く
}

SceneRenderer::SceneRenderer( GameWorld* world ) :
	mWorld( world ),
	mRenderer( world, &world->effects() ),
	mTaaCamera( nullptr ),
	mFlashlightShadow( kFlashlightShadowSize ),
	mFlashlightCastsShadow( false ),
	mMoonShadow( kMoonShadowSize ),
	mMoonCastsShadow( false ) {
}

void SceneRenderer::render( const FirstPersonCamera& camera, const DamagePostEffect& damage
#ifdef ZF_EDITOR
	, GameDebugOverlay* debug
#endif
) {
	// TAA のときは、描く行列を 1 画素より細かく毎フレームずらす（Taa_Plan.txt 5 章）。カメラが置いた行列を、ずらした物で上書きし、
	// 下の G-Buffer・SSAO・SSR・霧・デカール・半透明の板にも同じ行列を渡す（全部のパスが同じずれで揃う）。
	// 映る範囲（DrawCulling）と影はずらさない（1 画素より細かいずれは効かない／影は光の側から見た絵）。
	mJitter.advance( antiAliasingMode() == AntiAliasing::Taa );
	const Matrix44 projectionView = mJitter.apply( camera.projectionViewMatrix(),
		mPostProcess.sceneDepth().width(), mPostProcess.sceneDepth().height() );
	Graphics::Manager::instance().setProjectionViewMatrix( projectionView );
	// 攻撃された瞬間の赤み・痛み・死んだ後の暗さは、この描画の写すときに効く。
	mPostProcess.setDamageFlash( damage.flash );
	mPostProcess.setPain( damage.pain );
	mPostProcess.setFade( damage.fade );
	// SSAO・SSR・ディザのノイズは、TAA のときだけフレームごとに動かす（TAA が均す。Taa_Plan.txt 8 章）。TAA でないときは番号が 0 のまま。
	mPostProcess.setNoiseFrame( mJitter.frameIndex() );
	mPostProcess.beginScene();
	setupLights( camera );
	drawShadowMaps( camera );
	// 不透明な物は G-Buffer へ描き、全画面 1 回でライティングする（GBuffer_Plan.txt 5 章）。
	// フォワードへの切り替えは段階 3 の始めに消した（PbrNightLighting_Plan.txt Q1。PBR の式とライトを 2 か所に書かない）。
	const GBufferView view = gbufferView();
	const bool showsGBuffer = ( view != GBufferView::Final );
	// 映り込みの表示（SSR・SSR color）は、照らした後の色を映すので、表示中もライティングまで描く（表示は endScene の後に上から写す）。
	const bool showsReflection = ( view == GBufferView::Ssr || view == GBufferView::SsrColor );
	const bool drawsLighting = !showsGBuffer || showsReflection;
	mGBuffer.begin( mPostProcess.sceneDepth() );
	// 視点を置いた後の行列から映る範囲を作り、映らない地面・建物を描かない。
	// 動かない物 → 弾痕（デカール）→ 動く物の順（SsrDecalMaterial_Plan.txt 4 章・Q7）。デカールは画素に見えている面へ書くので、
	// 後から描くキャラがデカールを上書きして、キャラには弾痕が乗らない。
	{
		PROFILE_SCOPE( "World.draw" );
		const DrawCulling culling = cameraCulling( camera );
		mRenderer.drawStatic( culling, camera.eyePosition() );
		mGBuffer.beginDecals( mPostProcess.sceneDepth() );
		mRenderer.drawDecals( projectionView );
		Profiling::gpuMark( "Decals" );
		mGBuffer.endDecals( mPostProcess.sceneDepth() );
		// 動く物は動きの種類を G-Buffer へ書き分ける（TAA が前のフレームのどこを読むかを分ける。Taa_Plan.txt 6 章）。
		mGBuffer.setMotion( MotionKind::FirstPerson );
		mRenderer.drawFirstPerson();
		mGBuffer.setMotion( MotionKind::Moving );
		mRenderer.drawCharacters( culling );
	}
	mGBuffer.end();
	// SSAO は G-Buffer の後・ライティングの前（SsaoTranslucent_Plan.txt 4 章）。G-Buffer の表示（SSAO）でも見るので、表示中も作る。
	mAmbientOcclusion.draw( mGBuffer, mPostProcess.sceneDepth(), projectionView, mJitter.frameIndex() );
	Profiling::gpuMark( "SSAO" );
	if ( drawsLighting ) {
		mPostProcess.bindSceneColor(); // 深度はシェーダから読むので、描画先からは外す
		mLights.bindShadowMaps(); // 描画先を変えると t2 以降が外れるので、付け直す（LightList::bindShadowMaps）
		mDeferredLighting.drawLighting( mGBuffer, mAmbientOcclusion.result(), mPostProcess.sceneDepth(), projectionView );
		Profiling::gpuMark( "Lighting" );
		// 映り込みはライティングの後・霧の前（照らした色を映し、映り込みにも霧が掛かる。SsrDecalMaterial_Plan.txt 4 章）。
		// 作るパスはシーンの色を読むので、自分の描画先へ描いてから、シーンの色へ付け直して混ぜる。
		if ( ssrEnabled() ) {
			mReflection.buildHiZ( mPostProcess.sceneDepth() ); // F6 の Hi-Z を切っている間は作らない（HiZSsr_Plan.txt Q5）
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
		// 霧は半透明の板より前（板は自分で同じ霧を掛けるので、二重に掛けない。NightFinish_Plan.txt 4 章）。
		if ( fogEnabled() ) {
			mDeferredLighting.drawFog( mPostProcess.sceneDepth(), projectionView );
		}
		Profiling::gpuMark( "Fog" );
		// TAA は霧の後・半透明の板の前（板は動きが取れず、マズルフラッシュに光の尾が出るため。Taa_Plan.txt Q3）。
		// 見ているカメラが替わった（観察用カメラ F4 の切り替え）フレームは、前の履歴が別の場所の絵なので捨てる（Q7）。
		if ( &camera != mTaaCamera ) {
			mTaa.invalidate();
		}
		if ( antiAliasingMode() == AntiAliasing::Taa ) {
			mTaa.resolve( mPostProcess.sceneColor(), mGBuffer, mPostProcess.sceneDepth(), projectionView, camera.projectionViewMatrix() );
		} else {
			mTaa.invalidate(); // TAA に戻したとき、古い履歴を混ぜないように
		}
		Profiling::gpuMark( "TAA" );
	} else {
		mTaa.invalidate(); // G-Buffer の表示の間は照らした色が無いので、履歴も続かない
	}
	mTaaCamera = &camera;
	// 砂ぼこりなどの半透明の板は、G-Buffer と同じ深度で隠れるようシーンへ描く。板は深度を書かないので読み取り専用で付け、
	// 同じ深度を読んで壁・地面に刺さった所を薄くする（SsaoTranslucent_Plan.txt 7 章）。弾痕は G-Buffer のデカールになった（上）。
	mPostProcess.resumeSceneReadOnlyDepth();
	mLights.bindShadowMaps();   // 板も懐中電灯の影を受ける（同じ evaluateLight を通る）
	mRenderer.drawImpactDust( projectionView, camera.viewDirection(), camera.eyePosition() ); // 弾痕の上に舞う
	mRenderer.drawBloodSpray( projectionView, camera.viewDirection(), camera.eyePosition() ); // 砂ぼこりの後（同じ作り）
	mRenderer.drawMuzzleFlash( projectionView, camera.viewDirection() ); // 加算なので順番は問わない
	mLights.unbind(); // ライトの一覧を読むのは、ここまで（ライティングのパスと半透明の板）
#ifdef ZF_EDITOR
	mPostProcess.resumeScene(); // 当たり判定の表示は深度を書くことがあるので、書ける付け方へ戻す
	debug->drawCollision( *mWorld, camera );
	debug->drawLights( mLights, camera );
#endif
	// GPU の時間の区切り（Perf の欄の GPU）。世界の物ごとの印は WorldRenderer の中（Player〜Ground）。
	// ここまでが弾痕・砂ぼこり・血・マズルフラッシュ・当たり判定の表示、次の印までが画面全体の処理（ブルームなど）。
	Profiling::gpuMark( "Effects" );
	mPostProcess.endScene();
	if ( showsGBuffer ) {
		// ToneMap を通さず中身の数のまま見るので、画面へ写した後に上から描く。
		// 影の表示（Cascade・Shadow maps）はライトの一覧の定数と影のマップを読むので、外した物をもう一度付けて描く。
		mLights.rebindConstants();
		mLights.bindShadowMaps();
		mDeferredLighting.drawView( view, mGBuffer, mAmbientOcclusion.result(), mReflection.result(), mReflection.hiZ(),
			mPostProcess.sceneDepth(), projectionView );
		mLights.unbind();
	}
	// 「PostProcess」「FXAA」の印は endScene の中で打つ。ここまでは G-Buffer の表示（F6 で選んだときだけ。ふだんはほぼ 0）。
	Profiling::gpuMark( "GBufferView" );
}

const char* SceneRenderer::firstShaderError( const char* skinningError ) const {
	// 画面に出せるのは1行なので、写す処理（PostProcess）→ キャラ（SkinningPipeline）→ G-Buffer → G-Buffer の表示の順に、
	// 先に見つかったものを出す。
	const char* const shaderErrors[] = {
		mPostProcess.errorMessage(),
		skinningError,
		mGBuffer.errorMessage(),
		mDeferredLighting.errorMessage(),
		mAmbientOcclusion.errorMessage(),
		mReflection.errorMessage(),
		mTaa.errorMessage(),
		mFlashlightShadow.errorMessage(),
		mMoonShadow.errorMessage(),
	};
	for ( const char* message : shaderErrors ) {
		if ( message[ 0 ] != '\0' ) {
			return message;
		}
	}
	return "";
}

void SceneRenderer::setupLights( const FirstPersonCamera& camera ) {
	Graphics::Manager gm = Graphics::Manager::instance();
	gm.enableDepthTest( true );
	gm.enableDepthWrite( true );
	// 既定は両面描画。メッシュは MeshMaterial::apply が材質ごとに裏を捨てる（2026-10-27）ので、これが効くのはそれ以外の物だけ。
	gm.setCullMode( Graphics::CULL_NONE );
	// GameLib の 4 灯と環境光（固定パイプライン）は使わない。照らすのはライトの一覧だけ（ライティングのパスと半透明の板が b2 から読む）。
	// 月・補助光・空・環境光は調整値（LightingParams。初期値は夜。PbrNightLighting_Plan.txt 7 章）から毎フレーム作る（F6 ですぐ効くように）。
	const LightingParams& params = lightingParams();
	mLights.clear();
	const ShadowParams& shadow = shadowParams();
	mLights.setShadowSettings( shadow.normalOffsetTexels, shadow.lightOffsetTexels, shadow.strength );
	addMoon( params, camera );
	if ( params.fillIntensity > 0.f ) {
		mLights.add( directionalLight( lightDirectionFrom( params.fillElevation, params.fillAzimuth ), params.fillColor, params.fillIntensity ) );
	}
	// 自分の手元の光（懐中電灯・マズルフラッシュ）は、街灯より先に足す（16 個を選ぶとき、目からの距離が同じなら足した順で残るため）。
	addFlashlight( params );
	LightSource muzzle;
	if ( mRenderer.muzzleLight( &muzzle ) ) {
		mLights.add( muzzle );
	}
	// 交差点の街灯。16 個を超えた分は、LightList が目から遠い物から外す（PbrNightLighting_Plan.txt 8 章）。
	// 明るさは調整値 × 今日の条件（停電の日は 0。調整値そのものは変えない）。
	float streetLightIntensity = params.streetLightIntensity * mWorld->dayCondition().streetLightScale;
	if ( streetLightIntensity > 0.f ) {
		for ( const TownPlacement& spot : mWorld->map().streetLights() ) {
			Vector3 position( spot.x, spot.y + params.streetLightHeight, spot.z );
			mLights.add( pointLight( position, params.streetLightColor, streetLightIntensity, params.streetLightRange ) );
		}
	}
	EnvironmentLight environment;
	environment.skyZenithColor = params.skyZenithColor;
	environment.skyHorizonColor = params.skyHorizonColor;
	environment.groundColor = params.groundColor;
	environment.skyIntensity = params.skyIntensity;
	environment.ambientIntensity = params.ambientIntensity;
	// 霧を切っている間は 0 のまま（霧なし）。半透明の板もライトの一覧の霧の値を読むので、板の霧も一緒に消える。
	if ( fogEnabled() ) {
		environment.fogHalfDistance = fogHalfDistance(); // 霧の日は短い
		environment.fogGroundDensity = params.fogGroundDensity;
		environment.fogHeightHalf = params.fogHeightHalf;
	}
	// 月の絵（MoonStarsLut_Plan.txt 4 章）。切っている間は 0 のまま（今までの空）。月が地平線より下なら月は描かない。
	if ( skyDetailEnabled() ) {
		environment.moonDirection.setNeg( lightDirectionFrom( params.moonElevation, params.moonAzimuth ) ); // 光の進む向きの逆が、月のある方
		environment.moonColor = params.moonColor;
		environment.moonRadius = params.moonRadius;
		environment.moonDiscIntensity = ( params.moonElevation > 0.f ) ? params.moonDiscIntensity : 0.f;
		environment.moonHaloIntensity = params.moonHaloIntensity;
		environment.moonHaloFalloff = params.moonHaloFalloff;
		environment.pixelAngle = camera.fieldOfView() / static_cast< float >( gm.height() );
	}
	mLights.setEnvironment( environment );
	mLights.bind( camera.eyePosition() );
}

void SceneRenderer::addMoon( const LightingParams& params, const FirstPersonCamera& camera ) {
	Vector3 direction = lightDirectionFrom( params.moonElevation, params.moonAzimuth );
	LightSource moon = directionalLight( direction, params.moonColor, params.moonIntensity );
	mMoonCastsShadow = shadowsEnabled() && ( params.moonIntensity > 0.f );
	if ( mMoonCastsShadow ) {
		// 枚の受け持ちの端。F6 で逆に並べても範囲が 0 にならないよう、前の端より奥へ揃える。
		const ShadowParams& shadow = shadowParams();
		float ends[ CascadedShadowMap::kCascadeCount ] = { shadow.moonCascadeEnd0, shadow.moonCascadeEnd1, shadow.moonCascadeEnd2 };
		for ( int i = 1; i < CascadedShadowMap::kCascadeCount; ++i ) {
			ends[ i ] = max( ends[ i ], ends[ i - 1 ] + kMinCascadeGap );
		}
		Graphics::Manager gm = Graphics::Manager::instance();
		float aspect = static_cast< float >( gm.width() ) / static_cast< float >( gm.height() );
		// 遠い枚ほど間を空けて描き直す（OptimizationA_Plan.txt 7 章）。手前の枚は毎フレーム、間隔は調整値を整数に丸める。
		CascadedShadowMap::Refresh refresh;
		refresh.intervals[ 0 ] = kEveryFrame;
		refresh.intervals[ 1 ] = static_cast< int >( shadow.moonCascadeInterval1 + kRoundToNearest );
		refresh.intervals[ 2 ] = static_cast< int >( shadow.moonCascadeInterval2 + kRoundToNearest );
		refresh.redrawAngleDegree = shadow.moonRedrawAngle;
		refresh.redrawDistance = shadow.moonRedrawDistance;
		mMoonShadow.update( camera.eyePosition(), camera.viewDirection(), camera.fieldOfView(), aspect, direction, ends, shadow.moonCasterMargin,
			refresh );
		for ( int i = 0; i < CascadedShadowMap::kCascadeCount; ++i ) {
			mLights.setShadowMap( kMoonShadowFirstIndex + i, mMoonShadow.map( i ).depth(), mMoonShadow.projectionView( i ),
				mMoonShadow.texelSize( i ), kOrthographicShadow );
		}
		mLights.setCascades( camera.viewDirection(), ends, shadow.moonCascadeBlend );
		moon.shadowMap = kMoonShadowFirstIndex;
		moon.shadowMapCount = CascadedShadowMap::kCascadeCount;
	} else {
		mMoonShadow.invalidate(); // 影を戻したときに、止めていた間の古い影を使わないよう全部描き直す
	}
	mLights.add( moon );
}

void SceneRenderer::addFlashlight( const LightingParams& params ) {
	mFlashlightCastsShadow = false;
	const GameObject* player = mWorld->player();
	const HoldComponent* hold = player ? player->getComponent< HoldComponent >() : nullptr;
	const LightComponent* light = hold ? hold->light() : nullptr; // 銃に付いたライト（素手なら無い）
	Vector3 muzzlePosition;
	Vector3 muzzleDirection;
	// 銃をしまっている間は銃口が無い（muzzle が false）ので照らさない。付けた状態は自キャラが覚えている。
	if ( !light || !light->isOn() || !hold->muzzle( &muzzlePosition, &muzzleDirection ) ) {
		return;
	}
	Vector3 position;
	position.setMadd( muzzlePosition, muzzleDirection, params.flashlightOffset );
	// F6 で内側を外側より大きくしても止まらないよう、内側は外側までに抑える。
	float innerAngle = min( params.flashlightInnerAngle, params.flashlightOuterAngle );
	LightSource flashlight = spotLight( position, muzzleDirection, params.flashlightColor, params.flashlightIntensity, params.flashlightRange,
		innerAngle, params.flashlightOuterAngle );
	flashlight.nearDistance = params.flashlightNearDistance;
	// 影（Shadow_Plan.txt 5 章）。光と同じ位置・向き・角度から見た深度を、drawShadowMaps が描く。F6 で影を切っている間は付けない。
	if ( shadowsEnabled() ) {
		mFlashlightShadowMatrix = spotShadowProjectionView( position, muzzleDirection, params.flashlightOuterAngle,
			shadowParams().flashlightNearClip, params.flashlightRange );
		mLights.setShadowMap( kFlashlightShadowIndex, mFlashlightShadow.depth(), mFlashlightShadowMatrix,
			spotShadowTexelSizeAtOneMeter( params.flashlightOuterAngle, mFlashlightShadow.size() ), kPerspectiveShadow );
		flashlight.shadowMap = kFlashlightShadowIndex;
		mFlashlightCastsShadow = true;
	}
	mLights.add( flashlight );
}

void SceneRenderer::drawShadowMaps( const FirstPersonCamera& camera ) {
	if ( mFlashlightCastsShadow ) {
		mFlashlightShadow.begin( mFlashlightShadowMatrix );
		mRenderer.drawShadowCasters( DrawCulling( mFlashlightShadowMatrix ), kWithoutPlayer );
		mFlashlightShadow.end();
	}
	if ( mMoonCastsShadow ) {
		for ( int i = 0; i < CascadedShadowMap::kCascadeCount; ++i ) {
			if ( !mMoonShadow.isDue( i ) ) {
				continue; // このフレームは描き直さない（前に描いた影と行列をそのまま読む）
			}
			const Matrix44& matrix = mMoonShadow.projectionView( i );
			mMoonShadow.map( i ).begin( matrix );
			// 2 枚目からは置物を一番粗い段で描く（OptimizationC_Plan.txt Q8。遠い枚は 1 画素が大きく、形の差が影に出ない）。
			mRenderer.drawShadowCasters( DrawCulling::forOrthographicShadow( matrix, mMoonShadow.texelSize( i ), i >= kFirstCoarseLodCascade ),
				kWithPlayer );
			mMoonShadow.map( i ).end();
			PROFILE_COUNT( "Shadow.cascades", 1 ); // 描き直した月の影の枚数（OptimizationA_Plan.txt 5 章。間引く前は毎フレーム 3、間引いた後はふだん 1〜2）
		}
	}
	// 影のマップは光の側の行列で描いたので、G-Buffer へ描く前にカメラの行列へ戻す。
	Graphics::Manager::instance().setProjectionViewMatrix( camera.projectionViewMatrix() );
	// GPU の時間の区切り。ここまでが影のマップ（懐中電灯と月の 3 枚）。
	Profiling::gpuMark( "Shadow" );
}

float SceneRenderer::fogHalfDistance() const {
	return lightingParams().fogHalfDistance * mWorld->dayCondition().fogDistanceScale;
}

DrawCulling SceneRenderer::cameraCulling( const FirstPersonCamera& camera ) const {
	float drawDistance = fogEnabled() ? DrawCulling::drawDistanceForFog( fogHalfDistance() ) : kNoDrawDistance; // 霧の日は描く距離も縮む
	// 小ささの画素は、G-Buffer へ描く絵（シーンの深度）の高さで数える。
	// 隠れ（PVS）は目から見る絵だけ（影の culling には渡さない。OptimizationC_Plan.txt Q5）。
	const TownVisibility* visibility = pvsEnabled() ? &mWorld->visibility() : nullptr;
	return DrawCulling::forCamera( camera.projectionViewMatrix(), camera.eyePosition(), drawDistance, camera.fieldOfView(),
		mPostProcess.sceneDepth().height(), visibility );
}
