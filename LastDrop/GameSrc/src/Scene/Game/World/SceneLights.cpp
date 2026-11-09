#include "GameSrc/Scene/Game/World/SceneLights.h"
#include "GameSrc/Scene/Game/World/GameWorld.h"
#include "GameSrc/Scene/Game/World/WorldRenderer.h"
#include "GameSrc/World/GameObject.h"
#include "GameSrc/Player/Body/HoldComponent.h"
#include "GameSrc/Component/LightComponent.h"
#include "GameSrc/Player/Camera/FirstPersonCamera.h"
#include "GameSrc/Render/Culling/DrawCulling.h"
#include "GameSrc/Render/Shadow/ShadowParams.h"

#include "GameLib/GameLib.h"
#include "GameLib/Graphics/Manager.h"
#include "GameLib/Graphics/Enum.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Functions.h"
#include "GameLib/Profiling/Profiler.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// ── 影 ──
	const int kFlashlightShadowSize = 1024; // 懐中電灯の影のマップの一辺（画素）
	const int kFlashlightShadowIndex = 0;   // ライトの一覧の影のマップの番号（1〜3 は月の CSM に空けておく）
	const bool kPerspectiveShadow = true;
	const bool kWithoutPlayer = false;      // 懐中電灯の影には自キャラを描かない（銃の影が画面の真ん中を埋めるため）
	// ── 月の影（CSM） ──
	const int kMoonShadowSize = 2048;       // 1 枚の影のマップの一辺（画素）
	const int kMoonShadowFirstIndex = 1;    // ライトの一覧の影のマップの番号（1〜3 の 3 枚）
	const bool kOrthographicShadow = false;
	const bool kWithPlayer = true;          // 月の影には自キャラも描く（足元に自分の影が見える）
	const int kEveryFrame = 1;              // 月の影の手前の枚は毎フレーム描き直す（遅れると足元の影がずれて見える）
	const float kRoundToNearest = 0.5f;     // 調整値（float）の間隔を一番近い整数のフレームにする
	const float kMinCascadeGap = 0.5f;      // F6 で端を逆に並べても枚の範囲が 0 にならないよう、前の端よりこれだけ奥へ揃える（m）
	const int kFirstCoarseLodCascade = 1;   // 月の影のこの枚から、置物を一番粗い段（LOD）で描く
}

SceneLights::SceneLights( const GameWorld* world, WorldRenderer* renderer ) :
	mWorld( world ),
	mRenderer( renderer ),
	mFlashlightShadow( kFlashlightShadowSize ),
	mFlashlightCastsShadow( false ),
	mMoonShadow( kMoonShadowSize ),
	mMoonCastsShadow( false ) {
	ASSERT( mWorld && mRenderer && "SceneLights : an argument is null." );
}

LightList& SceneLights::lights() {
	return mLights;
}

const LightList& SceneLights::lights() const {
	return mLights;
}

const char* SceneLights::shadowErrorMessage() const {
	const char* message = mFlashlightShadow.errorMessage();
	return ( message[ 0 ] != '\0' ) ? message : mMoonShadow.errorMessage();
}

void SceneLights::setup( const FirstPersonCamera& camera ) {
	Graphics::Manager gm = Graphics::Manager::instance();
	gm.enableDepthTest( true );
	gm.enableDepthWrite( true );
	// 既定は両面描画。メッシュは MeshMaterial::apply が材質ごとに裏を捨てるので、これが効くのはそれ以外の物だけ。
	gm.setCullMode( Graphics::CULL_NONE );
	// GameLib の 4 灯と環境光（固定パイプライン）は使わない。照らすのはライトの一覧だけ（ライティングのパスと半透明の板が b2 から読む）。
	// 月・補助光・空・環境光は調整値（LightingParams。初期値は夜）から毎フレーム作る（F6 ですぐ効くように）。
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
	if ( mRenderer->muzzleLight( &muzzle ) ) {
		mLights.add( muzzle );
	}
	// 街灯の柱の頭（ランタンのガラスの中）から。16 個を超えた分は、LightList が目から遠い物から外す。
	// 外れた柱もガラスは光ったまま（模型の発光）。
	// 明るさは調整値 × 今日の条件（停電の日は 0。調整値そのものは変えない）。
	float streetLightIntensity = params.streetLightIntensity * mWorld->dayCondition().streetLightScale;
	if ( streetLightIntensity > 0.f ) {
		for ( const TownPlacement& spot : mWorld->town().map().streetLights() ) {
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
	// 月の絵。切っている間は 0 のまま（今までの空）。月が地平線より下なら月は描かない。
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

void SceneLights::addMoon( const LightingParams& params, const FirstPersonCamera& camera ) {
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
		// 遠い枚ほど間を空けて描き直す。手前の枚は毎フレーム、間隔は調整値を整数に丸める。
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

void SceneLights::addFlashlight( const LightingParams& params ) {
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
	// 影。光と同じ位置・向き・角度から見た深度を、drawShadowMaps が描く。F6 で影を切っている間は付けない。
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

void SceneLights::drawShadowMaps( const FirstPersonCamera& camera ) {
	if ( mFlashlightCastsShadow ) {
		mFlashlightShadow.begin( mFlashlightShadowMatrix );
		mRenderer->drawShadowCasters( DrawCulling( mFlashlightShadowMatrix ), kWithoutPlayer );
		mFlashlightShadow.end();
	}
	if ( mMoonCastsShadow ) {
		for ( int i = 0; i < CascadedShadowMap::kCascadeCount; ++i ) {
			if ( !mMoonShadow.isDue( i ) ) {
				continue; // このフレームは描き直さない（前に描いた影と行列をそのまま読む）
			}
			const Matrix44& matrix = mMoonShadow.projectionView( i );
			mMoonShadow.map( i ).begin( matrix );
			// 2 枚目からは置物を一番粗い段で描く（遠い枚は 1 画素が大きく、形の差が影に出ない）。
			mRenderer->drawShadowCasters( DrawCulling::forOrthographicShadow( matrix, mMoonShadow.texelSize( i ), i >= kFirstCoarseLodCascade ),
				kWithPlayer );
			mMoonShadow.map( i ).end();
			PROFILE_COUNT( "Shadow.cascades", 1 ); // 描き直した月の影の枚数（間引く前は毎フレーム 3、間引いた後はふだん 1〜2）
		}
	}
	// 影のマップは光の側の行列で描いたので、G-Buffer へ描く前にカメラの行列へ戻す。
	Graphics::Manager::instance().setProjectionViewMatrix( camera.projectionViewMatrix() );
	// GPU の時間の区切り。ここまでが影のマップ（懐中電灯と月の 3 枚）。
	Profiling::gpuMark( "Shadow" );
}

float SceneLights::fogHalfDistance() const {
	return lightingParams().fogHalfDistance * mWorld->dayCondition().fogDistanceScale;
}

