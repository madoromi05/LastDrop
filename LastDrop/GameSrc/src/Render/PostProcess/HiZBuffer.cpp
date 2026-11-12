#include "GameSrc/Render/PostProcess/HiZBuffer.h"

#include "GameLib/GameLib.h"
#include "GameLib/Graphics/Manager.h"

#include <algorithm>

using namespace GameLib;

namespace{
	const char* kShaderFile = "Resource/Shader/HiZBuild.hlsl";

	// 描画先の大きさ（画面に対する倍率）。SSR と同じ半分（SsrCommon.hlsli の kSsrHalfStep・ScreenSpaceReflection.cpp の kResolutionScale）
	const float kResolutionScale = 0.5f;

	// HiZBuild.hlsl の t2（読み元）/ b1
	const int kSourceSlot = Graphics::Manager::FIRST_USER_TEXTURE_SLOT;
	const int kConstantSlot = Graphics::Manager::FIRST_USER_CONSTANT_SLOT;

	// HiZBuild.hlsl の cbuffer HiZBuildConstants と同じ並び
	struct HiZBuildConstants{
		float targetWidth;
		float targetHeight;
		float unused[ 2 ];
	};
}

bool& hiZEnabled() {
	static bool sEnabled = true; // 起動中ずっと 1 つ（F6 で書き換える）
	return sEnabled;
}

int& hiZViewLevel() {
	static int sLevel = 0; // 起動中ずっと 1 つ（F6 で書き換える）
	return sLevel;
}

HiZBuffer::HiZBuffer() {
	mHiZ = Graphics::RenderTarget::createScreenSized( kResolutionScale, Graphics::RENDER_TARGET_R32F, Graphics::RenderTarget::FULL_MIP_CHAIN );
	mShader = Graphics::Shader::create( kShaderFile );
	mConstants = Graphics::ConstantBuffer::create( sizeof( HiZBuildConstants ) );
}

void HiZBuffer::draw( Graphics::DepthTarget depth ) {
	Graphics::Manager gm = Graphics::Manager::instance();
	gm.setBlendMode( Graphics::BLEND_OPAQUE );
	gm.setShader( mShader );
	// 段 0 はシーンの深度から。描画先の切り替えで読み元が外れるので、切り替えた後に付ける
	gm.setRenderTargetMip( mHiZ, 0 );
	gm.setShaderResource( kSourceSlot, depth );
	setTargetSize( 0 );
	gm.drawFullScreen();
	// 段 i は段 i−1 から（同じ描画先の別の段を読む）。段の数は画面の大きさで変わるので、描画先を付けた後に読む
	for ( int mip = 1; mip < mHiZ.mipLevels(); ++mip ) {
		gm.setRenderTargetMip( mHiZ, mip );
		gm.setShaderResource( kSourceSlot, mHiZ, mip - 1 );
		setTargetSize( mip );
		gm.drawFullScreen();
	}
	// 後から描く物（固定パイプライン）に残さない
	gm.setShaderResource( kSourceSlot, Graphics::RenderTarget() );
	gm.setShader( Graphics::Shader() );
	gm.setConstantBuffer( kConstantSlot, Graphics::ConstantBuffer() );
}

void HiZBuffer::setTargetSize( int mip ) {
	HiZBuildConstants constants;
	constants.targetWidth = static_cast< float >( std::max( mHiZ.width() >> mip, 1 ) );
	constants.targetHeight = static_cast< float >( std::max( mHiZ.height() >> mip, 1 ) );
	constants.unused[ 0 ] = 0.f;
	constants.unused[ 1 ] = 0.f;
	mConstants.set( &constants, sizeof( constants ) );
	Graphics::Manager::instance().setConstantBuffer( kConstantSlot, mConstants );
}

const Graphics::RenderTarget& HiZBuffer::result() const {
	return mHiZ;
}

int HiZBuffer::mipLevels() const {
	return mHiZ.mipLevels();
}

const char* HiZBuffer::errorMessage() const {
	return mShader.errorMessage();
}
