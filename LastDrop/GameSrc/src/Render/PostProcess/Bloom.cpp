#include "GameSrc/Render/PostProcess/Bloom.h"
#include "GameSrc/Render/PostProcess/PostProcessParams.h"

#include "GameLib/GameLib.h"
#include "GameLib/Graphics/Manager.h"
#include "GameLib/Graphics/DepthTarget.h"

using namespace GameLib;

namespace{
	const char* kExtractShaderFile = "Resource/Shader/BloomExtract.hlsl";
	const char* kDownShaderFile = "Resource/Shader/BloomDown.hlsl";
	const char* kUpShaderFile = "Resource/Shader/BloomUp.hlsl";

	// 段の大きさ。最初の段は画面の半分で、そこから段ごとに半分にする。
	const float kFirstLevelScale = 0.5f;
	const float kLevelShrink = 0.5f;

	// Bloom
	const int kSourceSlot = Graphics::Manager::FIRST_USER_TEXTURE_SLOT;
	const int kConstantSlot = Graphics::Manager::FIRST_USER_CONSTANT_SLOT;

	// BloomExtract.hlsl の cbuffer BloomConstants と同じ並び（float 4つ＝16バイト）。
	struct BloomConstants{
		float threshold;
		float padding[ 3 ];
	};

	// source を読む shader で、destination 全体を描く。ブレンドは呼んだ側で決めておく。
	void drawPass( const Graphics::Shader& shader, const Graphics::RenderTarget& source, const Graphics::RenderTarget& destination ) {
		Graphics::Manager gm = Graphics::Manager::instance();
		gm.setRenderTargets( &destination, 1, Graphics::DepthTarget() ); // 描画範囲が destination の大きさになる
		gm.setShader( shader );
		gm.setShaderResource( kSourceSlot, source ); // 描画先の切り替えで外れるので、切り替えた後に付ける
		gm.drawFullScreen();
	}
}

Bloom::Bloom() {
	float scale = kFirstLevelScale;
	for ( int i = 0; i < kLevelCount; ++i ) {
		mLevels[ i ] = Graphics::RenderTarget::createScreenSized( scale, Graphics::RENDER_TARGET_RGBA16F );
		scale *= kLevelShrink;
	}
	mExtractShader = Graphics::Shader::create( kExtractShaderFile );
	mDownShader = Graphics::Shader::create( kDownShaderFile );
	mUpShader = Graphics::Shader::create( kUpShaderFile );
	mConstants = Graphics::ConstantBuffer::create( sizeof( BloomConstants ) );
}

const Graphics::RenderTarget& Bloom::apply( const Graphics::RenderTarget& sceneColor ) {
	Graphics::Manager gm = Graphics::Manager::instance();
	// しきい値は F6 で動かせるので、毎回書く（16 バイトだけ）。
	// 昼の明るい面も 1 を少し超えることがあるので、画面全体がぼやけるようなら上げる（Bloom_Plan.txt 9 章）。
	BloomConstants constants = { postProcessParams().bloomThreshold, { 0.f, 0.f, 0.f } };
	mConstants.set( &constants, sizeof( constants ) );
	gm.setConstantBuffer( kConstantSlot, mConstants );

	// 取り出しと縮小は、前のフレームの絵を残さないよう上書きで描く（全画面なので消去は要らない）。
	gm.setBlendMode( Graphics::BLEND_OPAQUE );
	drawPass( mExtractShader, sceneColor, mLevels[ 0 ] );
	for ( int i = 1; i < kLevelCount; ++i ) {
		drawPass( mDownShader, mLevels[ i - 1 ], mLevels[ i ] );
	}
	// 小さい段から順に、1 つ大きい段へ加算する。最後に [0] へ全部の段が足される。
	// 足し先の段は縮めるときに書いた絵がそのまま残っていて、それを土台にする（Bloom_Plan.txt 2 章）。
	gm.setBlendMode( Graphics::BLEND_ADDITIVE );
	for ( int i = kLevelCount - 1; i > 0; --i ) {
		drawPass( mUpShader, mLevels[ i ], mLevels[ i - 1 ] );
	}

	// 後から描く物に残さない。
	gm.setShaderResource( kSourceSlot, Graphics::RenderTarget() );
	gm.setConstantBuffer( kConstantSlot, Graphics::ConstantBuffer() );
	return mLevels[ 0 ];
}

const char* Bloom::errorMessage() const {
	const Graphics::Shader* shaders[] = { &mExtractShader, &mDownShader, &mUpShader };
	for ( const Graphics::Shader* shader : shaders ) {
		const char* message = shader->errorMessage();
		if ( message[ 0 ] != '\0' ) {
			return message;
		}
	}
	return "";
}
