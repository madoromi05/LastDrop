#include "GameSrc/Render/PostProcess.h"
#include "GameSrc/Render/PostProcessParams.h"

#include "GameLib/GameLib.h"
#include "GameLib/Graphics/Manager.h"
#include "GameLib/Math/Vector4.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const char* kPresentShaderFile = "Resource/Shader/Present.hlsl";
	const float kSceneScale = 1.f; // 中間の描画先の大きさ（画面に対する倍率）
	const Vector4 kClearColor( 0.f, 0.f, 0.f, 1.f ); // 何も描かれていない所の色（画面の消去色と同じ黒）

	// Present.hlsl の番号と揃える（t2・t3 / b1）。
	const int kSceneColorSlot = Graphics::Manager::FIRST_USER_TEXTURE_SLOT;
	const int kBloomSlot = kSceneColorSlot + 1;
	const int kPresentConstantSlot = Graphics::Manager::FIRST_USER_CONSTANT_SLOT;

	const int kColorChannels = 3; // 赤・緑・青
	// Present.hlsl の cbuffer PresentConstants と同じ並び（float 16 個＝64 バイト。16 バイトずつの区切りに合わせる）。
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
	};
	// ビネット。角が少し暗くなる程度（確認用なので控えめ。見た目の調整はポストプロセスを作り込むときに行う）。
	const float kVignetteStrength = 0.35f;
	const float kVignetteInner = 0.6f;
	const float kVignetteOuter = 1.4f;
	// 噛まれた瞬間の赤み。画面の周り（中心からこの距離の間で強まる）を、この色へ寄せる。強さ 1 でも真ん中は赤くしない（狙いを隠さない）。
	const float kFlashInner = 0.3f;
	const float kFlashOuter = 1.2f;
	const float kFlashColor[ kColorChannels ] = { 0.45f, 0.02f, 0.02f }; // 血しぶきと同じ暗い赤に寄せる
	// 噛まれた瞬間の痛み（CameraShake_Recoil_Plan.txt の 4 章）。画面の中心へ向かう線の上を読んで平均する（放射状のぼかし）。
	// kPainBlurLength: 痛み 1・画面の端で、中心までの距離の何割の長さを読むか。中心からこの距離（kPainBlurInner）より内はぼかさない。
	const float kPainBlurLength = 0.12f;
	const float kPainBlurInner = 0.25f;
	// 痛み 1 のとき、ビネットの暗さに足す量（角での暗さが 0.35 → 0.85 になる）。
	const float kPainDarken = 0.5f;
	const float kNoEffect = 0.f;
}

PostProcessParams& postProcessParams() {
	static PostProcessParams sParams; // 起動中ずっと 1 つ（F6 で書き換える）
	return sParams;
}

PostProcess::PostProcess() :
	mDamageFlash( kNoEffect ),
	mPain( kNoEffect ),
	mFade( kNoEffect ) {
	mSceneColor = Graphics::RenderTarget::createScreenSized( kSceneScale, Graphics::RENDER_TARGET_RGBA16F );
	mSceneDepth = Graphics::DepthTarget::createScreenSized( kSceneScale );
	mPresentShader = Graphics::Shader::create( kPresentShaderFile );
	mPresentConstants = Graphics::ConstantBuffer::create( sizeof( PresentConstants ) );
}

void PostProcess::beginScene() {
	Graphics::Manager gm = Graphics::Manager::instance();
	gm.setRenderTargets( &mSceneColor, 1, mSceneDepth );
	// 中間の描画先はリニア（光の量に比例した数）で持つ。ここから endScene までに描く 3D の色をリニアへ直す。
	gm.enableLinearColor( true );
	gm.clear( mSceneColor, kClearColor );
	gm.clear( mSceneDepth );
}

void PostProcess::endScene() {
	Graphics::Manager gm = Graphics::Manager::instance();
	// この後に画面へ直接描く HUD・文字は、画像の色のまま出す（今までと同じ色）。
	gm.enableLinearColor( false );
	// 画面へ戻す前に作る（ブルームは描画先を切り替えながら描くため）。
	const Graphics::RenderTarget& bloom = mBloom.apply( mSceneColor );
	gm.resetRenderTargets();
	gm.setBlendMode( Graphics::BLEND_OPAQUE ); // 写すだけ。画面に元からある色とは混ぜない
	updateConstants();
	gm.setShader( mPresentShader );
	gm.setShaderResource( kSceneColorSlot, mSceneColor );
	gm.setShaderResource( kBloomSlot, bloom );
	gm.setConstantBuffer( kPresentConstantSlot, mPresentConstants );
	gm.drawFullScreen();
	// 後から描く物（固定パイプライン）に残さない。
	gm.setShader( Graphics::Shader() );
	gm.setShaderResource( kSceneColorSlot, Graphics::RenderTarget() );
	gm.setShaderResource( kBloomSlot, Graphics::RenderTarget() );
	gm.setConstantBuffer( kPresentConstantSlot, Graphics::ConstantBuffer() );
}

const char* PostProcess::errorMessage() const {
	const char* message = mPresentShader.errorMessage();
	if ( message[ 0 ] != '\0' ) {
		return message;
	}
	return mBloom.errorMessage();
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

void PostProcess::updateConstants() {
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
	mPresentConstants.set( &constants, sizeof( constants ) );
}
