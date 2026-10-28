#include "GameSrc/Render/GBuffer.h"
#include "GameSrc/Render/RenderPass.h"

#include "GameLib/Graphics/Manager.h"
#include "GameLib/Graphics/Enum.h"
#include "GameLib/Math/Vector4.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const char* kStaticMeshShaderFile = "Resource/Shader/StaticMeshGBuffer.hlsl";
	const float kScreenScale = 1.f; // PostProcess の深度と同じ大きさ（setRenderTargets は大きさの違う描画先を一緒に付けられない）

	// 並びは Resource/Shader/GBuffer.hlsli と揃える。
	const int kAlbedoIndex = 0;
	const int kNormalIndex = 1;
	const int kSpecularIndex = 2;
	const Graphics::RenderTargetFormat kFormats[] = {
		Graphics::RENDER_TARGET_RGBA8_SRGB, // 色。8bit にリニアで入れると暗い色に段差が出るので sRGB
		Graphics::RENDER_TARGET_RGBA16F,    // 法線（-1〜1）と鋭さ（8〜48）。8bit に入らない数なので浮動小数
		Graphics::RENDER_TARGET_RGBA8_SRGB, // スペキュラの色。0.02（リニアで 0.0015）のような小さい数を 0 にしないため sRGB
	};
	// 何も描かれていない画素は、どの面の情報も 0（ライティングのパスでは深度で見分ける）。
	const Vector4 kClearValue( 0.f, 0.f, 0.f, 0.f );
}

GBuffer::GBuffer() {
	static_assert( sizeof( kFormats ) / sizeof( kFormats[ 0 ] ) == kTargetNumber, "GBuffer : one format per target." );
	for ( int i = 0; i < kTargetNumber; ++i ) {
		mTargets[ i ] = Graphics::RenderTarget::createScreenSized( kScreenScale, kFormats[ i ] );
	}
	mStaticMeshShader = Graphics::Shader::create( kStaticMeshShaderFile );
}

void GBuffer::begin( Graphics::DepthTarget depth ) {
	Graphics::Manager gm = Graphics::Manager::instance();
	gm.setRenderTargets( mTargets, kTargetNumber, depth );
	for ( int i = 0; i < kTargetNumber; ++i ) {
		gm.clear( mTargets[ i ], kClearValue );
	}
	// G-Buffer のアルファは色の透明度ではなくデータ（RT0・RT2 は予備の 0、RT1 は鋭さ 8〜48）なので、混ぜずにそのまま書く。
	// 前に描いた物（半透明の板・UI）のブレンドが残っていると、アルファ 0 の RT0 は何も書かれず、RT1 は鋭さ倍に膨らむ（2026-10-28）。
	gm.setBlendMode( Graphics::BLEND_OPAQUE );
	RenderPass::beginGBuffer( mStaticMeshShader );
}

void GBuffer::end() {
	RenderPass::endGBuffer();
	// 後から描く物（固定パイプライン）に残さない。
	Graphics::Manager::instance().setShader( Graphics::Shader() );
}

const Graphics::RenderTarget& GBuffer::albedo() const {
	return mTargets[ kAlbedoIndex ];
}

const Graphics::RenderTarget& GBuffer::normal() const {
	return mTargets[ kNormalIndex ];
}

const Graphics::RenderTarget& GBuffer::specular() const {
	return mTargets[ kSpecularIndex ];
}

const char* GBuffer::errorMessage() const {
	return mStaticMeshShader.errorMessage();
}
