#include "GameSrc/Render/PostProcess/PostProcess.h"
#include "GameSrc/Render/PostProcess/PostProcessParams.h"

#include "GameLib/GameLib.h"
#include "GameLib/Graphics/Manager.h"
#include "GameLib/Math/Vector4.h"
#include "GameLib/Profiling/Profiler.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const char* kPresentShaderFile = "Resource/Shader/Present.hlsl";
	const char* kFxaaShaderFile = "Resource/Shader/Fxaa.hlsl";
	const float kSceneScale = 1.f; // 中間の描画先の大きさ（画面に対する倍率）
	const Vector4 kClearColor( 0.f, 0.f, 0.f, 1.f ); // 何も描かれていない所の色（画面の消去色と同じ黒）

	// Present.hlsl の番号と揃える（t2・t3 / b1）。
	const int kSceneColorSlot = Graphics::Manager::FIRST_USER_TEXTURE_SLOT;
	const int kBloomSlot = kSceneColorSlot + 1;
	// 色の表（Grading.hlsli の gLut。t4）。表を読んでいるときだけ付ける。
	const int kLutSlot = kSceneColorSlot + 2;
	const int kPresentConstantSlot = Graphics::Manager::FIRST_USER_CONSTANT_SLOT;
	// Fxaa.hlsl の番号と揃える（t2 / b1）。
	const int kFxaaInputSlot = Graphics::Manager::FIRST_USER_TEXTURE_SLOT;
	// 半透明の板が読むシーンの深度（SoftQuad.hlsli の t5。G-Buffer の並びの深度と同じ番号）。
	const int kSoftQuadDepthSlot = Graphics::Manager::FIRST_USER_TEXTURE_SLOT + 3;
	const int kFxaaConstantSlot = Graphics::Manager::FIRST_USER_CONSTANT_SLOT;

	// Fxaa.hlsl の cbuffer FxaaConstants と同じ並び（float 4 個）。
	struct FxaaConstants{
		float edgeThreshold;
		float edgeThresholdMin;
		float subpixel;
		float unused;
	};

	const int kColorChannels = 3; // 赤・緑・青
	// Present.hlsl の cbuffer PresentConstants と同じ並び（16 バイトずつの区切りに合わせる）。
	struct PresentConstants{
		float vignetteStrength;
		float vignetteInner;
		float vignetteOuter;
		float bloomIntensity;
		float damageFlash;
		float flashInner;
		float flashOuter;
		float fade;
		float flashColor[ kColorChannels ];
		float exposure;
		float pain;
		float painBlurLength;
		float painBlurInner;
		float painDarken;
		// カラーグレーディングとディザ（NightFinish_Plan.txt 6・7 章）
		float whiteBalance[ kColorChannels ]; // 色温度から作った r・g・b の倍率（明るさが変わらないよう割り戻し済み）
		float saturation;
		float contrast;
		float shadowsEnd;
		float highlightsStart;
		float nightDesaturation;
		float shadowsColor[ kColorChannels ];   // sRGB（シェーダでリニアへ直す）
		float ditherAmount;                     // 足すノイズの幅（画面の色の 1 段＝1/255。0 で無し）
		float midtonesColor[ kColorChannels ];
		float noiseFrame;                       // ディザのノイズのフレームの番号（Taa_Plan.txt 8 章）
		float highlightsColor[ kColorChannels ];
		float lutStrength;                      // 色の表を混ぜる強さ（表を読んでいなければ 0。MoonStarsLut_Plan.txt 6 章）
	};
	static_assert( sizeof( PresentConstants ) % 16 == 0, "PresentConstants : keep the HLSL 16-byte layout." );
	// ビネット。角が少し暗くなる程度（確認用なので控えめ。見た目の調整はポストプロセスを作り込むときに行う）。
	const float kVignetteStrength = 0.35f;
	const float kVignetteInner = 0.6f;
	const float kVignetteOuter = 1.4f;
	// 攻撃された瞬間の赤み。画面の周り（中心からこの距離の間で強まる）を、この色へ寄せる。強さ 1 でも真ん中は赤くしない（狙いを隠さない）。
	const float kFlashInner = 0.3f;
	const float kFlashOuter = 1.2f;
	const float kFlashColor[ kColorChannels ] = { 0.45f, 0.02f, 0.02f }; // 血しぶきと同じ暗い赤に寄せる
	// 攻撃された瞬間の痛み（CameraShake_Recoil_Plan.txt の 4 章）。画面の中心へ向かう線の上を読んで平均する（放射状のぼかし）。
	// kPainBlurLength: 痛み 1・画面の端で、中心までの距離の何割の長さを読むか。中心からこの距離（kPainBlurInner）より内はぼかさない。
	const float kPainBlurLength = 0.12f;
	const float kPainBlurInner = 0.25f;
	// 痛み 1 のとき、ビネットの暗さに足す量（角での暗さが 0.35 → 0.85 になる）。
	const float kPainDarken = 0.5f;
	const float kNoEffect = 0.f;

	// 色温度 1 のときの、赤と青の倍率の変わり方（1 で赤 ×1.2・青 ×0.8。−1 でその逆）。
	const float kTemperatureScale = 0.2f;
	// リニアの色の輝度の重み（Rec.709。Present.hlsl の kLuminanceWeights と同じ）。
	const Vector3 kLuminanceWeights( 0.2126f, 0.7152f, 0.0722f );
	// ディザの幅：画面の色（8bit）の 1 段。シェーダはこの幅の ±半分のノイズを足す。
	const float kDitherStep = 1.f / 255.f;

	// 色温度 temperature（−1〜1）から、r・g・b に掛ける倍率を作る。輝度で割り戻して、明るさは変えない（色だけ変える）。
	Vector3 whiteBalance( float temperature ) {
		Vector3 scale( 1.f + temperature * kTemperatureScale, 1.f, 1.f - temperature * kTemperatureScale );
		float luminance = scale.dot( kLuminanceWeights );
		return Vector3( scale.x / luminance, scale.y / luminance, scale.z / luminance );
	}

	void setColor( float* out, const Vector3& color ) {
		out[ 0 ] = color.x;
		out[ 1 ] = color.y;
		out[ 2 ] = color.z;
	}
}

PostProcessParams& postProcessParams() {
	static PostProcessParams sParams; // 起動中ずっと 1 つ（F6 で書き換える）
	return sParams;
}

bool& colorGradingEnabled() {
	static bool sEnabled = true; // 起動中ずっと 1 つ（F6 で書き換える）
	return sEnabled;
}

bool& ditherEnabled() {
	static bool sEnabled = true; // 起動中ずっと 1 つ（F6 で書き換える）
	return sEnabled;
}

ColorLutControl& colorLutControl() {
	static ColorLutControl sControl; // 起動中ずっと 1 つ（F6 のボタンが書き、PostProcess が下ろす）
	return sControl;
}

const char* antiAliasingName( AntiAliasing mode ) {
	static const char* const kNames[] = { "Off", "FXAA", "TAA" };
	static_assert( sizeof( kNames ) / sizeof( kNames[ 0 ] ) == static_cast< int >( AntiAliasing::Count ), "AntiAliasing : one name per mode." );
	return kNames[ static_cast< int >( mode ) ];
}

AntiAliasing& antiAliasingMode() {
	static AntiAliasing sMode = AntiAliasing::Taa; // 起動中ずっと 1 つ（F6 で書き換える。Taa_Plan.txt Q4）
	return sMode;
}

PostProcess::PostProcess() :
	mDamageFlash( kNoEffect ),
	mPain( kNoEffect ),
	mFade( kNoEffect ),
	mNoiseFrame( 0 ) {
	mSceneColor = Graphics::RenderTarget::createScreenSized( kSceneScale, Graphics::RENDER_TARGET_RGBA16F );
	mSceneDepth = Graphics::DepthTarget::createScreenSized( kSceneScale );
	mPresentShader = Graphics::Shader::create( kPresentShaderFile );
	mPresentConstants = Graphics::ConstantBuffer::create( sizeof( PresentConstants ) );
	// 画面と同じ 8bit。中身は画面の色（sRGB の数）のまま持つ（_SRGB の形式にしない。FXAA は画面の色の明るさの差で輪郭を探すため）。
	mFxaaInput = Graphics::RenderTarget::createScreenSized( kSceneScale, Graphics::RENDER_TARGET_RGBA8 );
	mFxaaShader = Graphics::Shader::create( kFxaaShaderFile );
	mFxaaConstants = Graphics::ConstantBuffer::create( sizeof( FxaaConstants ) );
}

void PostProcess::beginScene() {
	Graphics::Manager gm = Graphics::Manager::instance();
	gm.setRenderTargets( &mSceneColor, 1, mSceneDepth );
	// 中間の描画先はリニア（光の量に比例した数）で持つ。ここから endScene までに描く 3D の色をリニアへ直す。
	gm.enableLinearColor( true );
	gm.clear( mSceneColor, kClearColor );
	gm.clear( mSceneDepth );
}

void PostProcess::resumeScene() {
	Graphics::Manager::instance().setRenderTargets( &mSceneColor, 1, mSceneDepth );
}

void PostProcess::resumeSceneReadOnlyDepth() {
	Graphics::Manager gm = Graphics::Manager::instance();
	gm.setRenderTargets( &mSceneColor, 1, mSceneDepth, true );
	gm.setShaderResource( kSoftQuadDepthSlot, mSceneDepth ); // 描画先の切り替えで外れるので、切り替えた後に付ける
}

void PostProcess::bindSceneColor() {
	Graphics::Manager::instance().setRenderTargets( &mSceneColor, 1, Graphics::DepthTarget() );
}

const Graphics::RenderTarget& PostProcess::sceneColor() const {
	return mSceneColor;
}

Graphics::DepthTarget PostProcess::sceneDepth() const {
	return mSceneDepth;
}

void PostProcess::endScene() {
	Graphics::Manager gm = Graphics::Manager::instance();
	// この後に画面へ直接描く HUD・文字は、画像の色のまま出す（今までと同じ色）。
	gm.enableLinearColor( false );
	updateColorLut();
	// 画面へ戻す前に作る（ブルームは描画先を切り替えながら描くため）。
	const Graphics::RenderTarget& bloom = mBloom.apply( mSceneColor );
	// FXAA を掛けるなら、写した結果をいったん 8bit の中間へ置く（Present.hlsl は a に輝度も書く）。掛けないなら画面へ直接。
	const bool usesFxaa = ( antiAliasingMode() == AntiAliasing::Fxaa );
	if ( usesFxaa ) {
		gm.setRenderTargets( &mFxaaInput, 1, Graphics::DepthTarget() );
	} else {
		gm.resetRenderTargets();
	}
	gm.setBlendMode( Graphics::BLEND_OPAQUE ); // 写すだけ。画面に元からある色とは混ぜない
	updateConstants( colorGradingEnabled() );
	gm.setShader( mPresentShader );
	gm.setShaderResource( kSceneColorSlot, mSceneColor );
	gm.setShaderResource( kBloomSlot, bloom );
	if ( mColorLut.isLoaded() ) {
		gm.setShaderResource( kLutSlot, mColorLut.texture() );
	}
	gm.setConstantBuffer( kPresentConstantSlot, mPresentConstants );
	gm.drawFullScreen();
	// 後から描く物（固定パイプライン）に残さない。
	gm.setShader( Graphics::Shader() );
	gm.setShaderResource( kSceneColorSlot, Graphics::RenderTarget() );
	gm.setShaderResource( kBloomSlot, Graphics::RenderTarget() );
	gm.setShaderResource( kLutSlot, Graphics::Texture() );
	gm.setConstantBuffer( kPresentConstantSlot, Graphics::ConstantBuffer() );
	// GPU の時間の区切り。ここまでがブルームと写す処理（グレーディング・ディザ込み）、次の印までが FXAA。
	Profiling::gpuMark( "PostProcess" );
	if ( usesFxaa ) {
		drawFxaa();
	}
	Profiling::gpuMark( "FXAA" );
}

void PostProcess::drawFxaa() {
	Graphics::Manager gm = Graphics::Manager::instance();
	gm.resetRenderTargets(); // 中間を描画先から外してから読む（同じ物を描画先とシェーダの両方には付けられない）
	const PostProcessParams& params = postProcessParams();
	FxaaConstants constants;
	constants.edgeThreshold = params.fxaaEdgeThreshold;
	constants.edgeThresholdMin = params.fxaaEdgeThresholdMin;
	constants.subpixel = params.fxaaSubpixel;
	constants.unused = 0.f;
	mFxaaConstants.set( &constants, sizeof( constants ) );
	gm.setBlendMode( Graphics::BLEND_OPAQUE );
	gm.setShader( mFxaaShader );
	gm.setShaderResource( kFxaaInputSlot, mFxaaInput );
	gm.setConstantBuffer( kFxaaConstantSlot, mFxaaConstants );
	gm.drawFullScreen();
	// 後から描く物（固定パイプライン）に残さない。
	gm.setShader( Graphics::Shader() );
	gm.setShaderResource( kFxaaInputSlot, Graphics::RenderTarget() );
	gm.setConstantBuffer( kFxaaConstantSlot, Graphics::ConstantBuffer() );
}

void PostProcess::updateColorLut() {
	ColorLutControl& control = colorLutControl();
	if ( control.reloadRequested ) {
		mColorLut.reload();
		control.reloadRequested = false;
	}
	if ( control.exportRequested ) {
		// F6 の Color grading を切っていても、今の調整の値で焼く（切った値で焼くと何も変えない表になるため）。
		updateConstants( true );
		mColorLut.exportBaked( mPresentConstants );
		control.exportRequested = false;
	}
	mColorLut.update();
	control.loaded = mColorLut.isLoaded();
}

const char* PostProcess::errorMessage() const {
	const char* message = mPresentShader.errorMessage();
	if ( message[ 0 ] != '\0' ) {
		return message;
	}
	message = mFxaaShader.errorMessage();
	if ( message[ 0 ] != '\0' ) {
		return message;
	}
	message = mBloom.errorMessage();
	if ( message[ 0 ] != '\0' ) {
		return message;
	}
	return mColorLut.errorMessage();
}

void PostProcess::setDamageFlash( float amount ) {
	mDamageFlash = amount;
}

void PostProcess::setPain( float amount ) {
	mPain = amount;
}

void PostProcess::setFade( float amount ) {
	mFade = amount;
}

void PostProcess::setNoiseFrame( int frame ) {
	mNoiseFrame = frame;
}

void PostProcess::updateConstants( bool gradingOn ) {
	PresentConstants constants;
	constants.vignetteStrength = kVignetteStrength;
	constants.vignetteInner = kVignetteInner;
	constants.vignetteOuter = kVignetteOuter;
	constants.bloomIntensity = postProcessParams().bloomIntensity;
	constants.damageFlash = mDamageFlash;
	constants.flashInner = kFlashInner;
	constants.flashOuter = kFlashOuter;
	constants.fade = mFade;
	for ( int i = 0; i < kColorChannels; ++i ) {
		constants.flashColor[ i ] = kFlashColor[ i ];
	}
	constants.exposure = postProcessParams().exposure;
	constants.pain = mPain;
	constants.painBlurLength = kPainBlurLength;
	constants.painBlurInner = kPainBlurInner;
	constants.painDarken = kPainDarken;
	// グレーディングを切っている間は、初期値（何も変えない値）を渡す。シェーダは同じ式のまま（分かれ道を作らない）。
	const PostProcessParams neutral;
	const PostProcessParams& grade = gradingOn ? postProcessParams() : neutral;
	setColor( constants.whiteBalance, whiteBalance( grade.temperature ) );
	constants.saturation = grade.saturation;
	constants.contrast = grade.contrast;
	constants.shadowsEnd = grade.shadowsEnd;
	constants.highlightsStart = grade.highlightsStart;
	constants.nightDesaturation = grade.nightDesaturation;
	setColor( constants.shadowsColor, grade.shadowsColor );
	constants.ditherAmount = ditherEnabled() ? kDitherStep : kNoEffect;
	setColor( constants.midtonesColor, grade.midtonesColor );
	constants.noiseFrame = static_cast< float >( mNoiseFrame );
	setColor( constants.highlightsColor, grade.highlightsColor );
	constants.lutStrength = mColorLut.isLoaded() ? postProcessParams().lutStrength : kNoEffect;
	mPresentConstants.set( &constants, sizeof( constants ) );
}
