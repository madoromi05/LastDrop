#include "PostProcess.h"

#include "GameLib/GameLib.h"
#include "GameLib/Graphics/Manager.h"
#include "GameLib/Math/Vector4.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const char* kPresentShaderFile = "Resource/Shader/Present.hlsl";
	const float kSceneScale = 1.f; // 中間の描画先の大きさ（画面に対する倍率）
	const Vector4 kClearColor( 0.f, 0.f, 0.f, 1.f ); // 何も描かれていない所の色（画面の消去色と同じ黒）

	// Present.hlsl の番号と揃える（t2 / b1）。
	const int kSceneColorSlot = Graphics::Manager::FIRST_USER_TEXTURE_SLOT;
	const int kPresentConstantSlot = Graphics::Manager::FIRST_USER_CONSTANT_SLOT;

	// Present.hlsl の cbuffer PresentConstants と同じ並び（float 4つ＝16バイト）。
	struct PresentConstants{
		float vignetteStrength;
		float vignetteInner;
		float vignetteOuter;
		float padding;
	};
	// ビネット。角が少し暗くなる程度（確認用なので控えめ。見た目の調整はポストプロセスを作り込むときに行う）。
	const float kVignetteStrength = 0.35f;
	const float kVignetteInner = 0.6f;
	const float kVignetteOuter = 1.4f;
}

PostProcess::PostProcess() {
	mSceneColor = Graphics::RenderTarget::createScreenSized( kSceneScale, Graphics::RENDER_TARGET_RGBA16F );
	mSceneDepth = Graphics::DepthTarget::createScreenSized( kSceneScale );
	mPresentShader = Graphics::Shader::create( kPresentShaderFile );
	mPresentConstants = Graphics::ConstantBuffer::create( sizeof( PresentConstants ) );

	PresentConstants constants = { kVignetteStrength, kVignetteInner, kVignetteOuter, 0.f };
	mPresentConstants.set( &constants, sizeof( constants ) );
}

void PostProcess::beginScene() {
	Graphics::Manager gm = Graphics::Manager::instance();
	gm.setRenderTargets( &mSceneColor, 1, mSceneDepth );
	gm.clear( mSceneColor, kClearColor );
	gm.clear( mSceneDepth );
}

void PostProcess::endScene() {
	Graphics::Manager gm = Graphics::Manager::instance();
	gm.resetRenderTargets();
	gm.setBlendMode( Graphics::BLEND_OPAQUE ); // 写すだけ。画面に元からある色とは混ぜない
	gm.setShader( mPresentShader );
	gm.setShaderResource( kSceneColorSlot, mSceneColor );
	gm.setConstantBuffer( kPresentConstantSlot, mPresentConstants );
	gm.drawFullScreen();
	// 後から描く物（固定パイプライン）に残さない。
	gm.setShader( Graphics::Shader() );
	gm.setShaderResource( kSceneColorSlot, Graphics::RenderTarget() );
	gm.setConstantBuffer( kPresentConstantSlot, Graphics::ConstantBuffer() );
}

const char* PostProcess::errorMessage() const {
	return mPresentShader.errorMessage();
}
