#include "GameSrc/Render/Lighting/DeferredLighting.h"
#include "GameSrc/Render/Lighting/GBuffer.h"
#include "GameSrc/Render/PostProcess/HiZBuffer.h"
#include "GameSrc/Render/ShaderSlots.h"
#include "GameSrc/Render/ShaderConstants.h"

#include "GameLib/GameLib.h"
#include "GameLib/Graphics/Manager.h"
#include "GameLib/Graphics/RenderTarget.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const char* kShaderFile = "Resource/Shader/DeferredLighting.hlsl";
	const char* kFogShaderFile = "Resource/Shader/Fog.hlsl";
	const char* kReflectionShaderFile = "Resource/Shader/SsrComposite.hlsl";

	// テクスチャの番号は ShaderSlots（G-Buffer・SSAO の結果・SSR の結果・月の模様・表示の Hi-Z）。月の模様は空を描くライティングのパスでだけ付ける
	const char* kMoonTextureFile = "Resource/Texture/Map/moon.dds";
	const int kConstantSlot = Graphics::Manager::FIRST_USER_CONSTANT_SLOT;	// DeferredConstants.hlsli の b1

	const float kDepthViewRange = 60.f;	// 深度の表示で、目からこの距離を白にする（画面で見て決めた値ではない初期値）

	const int kParamFloats = 4 + ShaderConstants::kRowFloats;
	// DeferredConstants.hlsli の cbuffer DeferredConstants と同じ並び（16 バイトずつの区切りに合わせる）
	struct DeferredConstants{
		float inverseProjectionView[ ShaderConstants::kMatrixFloats ]; // 行ごと（Matrix44 の m00..m33 の順）
		float view;                                   // 表示する物（GBufferView の番号）
		float depthViewRange;
		float hiZViewLevel;                           // 表示の Hi-Z で見る段
		float unused;
		float viewDepthRow[ ShaderConstants::kRowFloats ];             // 射影×ビューの 4 行目（位置との内積が目からの奥行き。SSAO の拡大に使う）
	};
	static_assert( sizeof( DeferredConstants ) == sizeof( float ) * ( ShaderConstants::kMatrixFloats + kParamFloats ), "DeferredConstants : keep the HLSL layout." );

	const char* const kViewNames[] = { "Final", "Albedo", "Normal", "Roughness", "Metallic", "Emission", "Depth", "Position", "Cascade", "Shadow maps", "Fog", "SSAO", "SSR", "SSR color", "Motion", "Hi-Z" };
	static_assert( sizeof( kViewNames ) / sizeof( kViewNames[ 0 ] ) == static_cast< int >( GBufferView::Count ), "GBufferView : one name per view." );
}

const char* gbufferViewName( GBufferView view ) {
	return kViewNames[ static_cast< int >( view ) ];
}

GBufferView& gbufferView() {
	static GBufferView sView = GBufferView::Final; // 起動中ずっと 1 つ（F6 で書き換える）
	return sView;
}

DeferredLighting::DeferredLighting() {
	mShader = Graphics::Shader::create( kShaderFile );
	mFogShader = Graphics::Shader::create( kFogShaderFile );
	mReflectionShader = Graphics::Shader::create( kReflectionShaderFile );
	mConstants = Graphics::ConstantBuffer::create( sizeof( DeferredConstants ) );
	mMoonTexture = Graphics::Texture::create( kMoonTextureFile, Graphics::TEXTURE_COLOR_SPACE_SRGB ); // 画像の色（tools/texgen/moon_gen.py）
}

void DeferredLighting::drawLighting( const GBuffer& gbuffer, const Graphics::RenderTarget& occlusion, Graphics::DepthTarget depth,
	const Matrix44& projectionView ) {
	drawFullScreen( GBufferView::Final, gbuffer, occlusion, depth, projectionView );
}

void DeferredLighting::drawFog( Graphics::DepthTarget depth, const Matrix44& projectionView ) {
	setConstants( GBufferView::Final, projectionView );
	Graphics::Manager gm = Graphics::Manager::instance();
	gm.setBlendMode( Graphics::BLEND_LINEAR ); // 霧の量（アルファ）の割合で、ライティングした色を霧の色へ寄せる
	gm.setShader( mFogShader );
	gm.setShaderResource( ShaderSlots::kSceneDepth, depth );
	gm.drawFullScreen();
	// 後から描く物（固定パイプライン）に残さない。混ぜ方もライティングの後と同じ（不透明）へ戻す
	gm.setBlendMode( Graphics::BLEND_OPAQUE );
	gm.setShader( Graphics::Shader() );
	gm.setShaderResource( ShaderSlots::kSceneDepth, Graphics::DepthTarget() );
	gm.setConstantBuffer( kConstantSlot, Graphics::ConstantBuffer() );
}

void DeferredLighting::drawReflection( const GBuffer& gbuffer, const Graphics::RenderTarget& occlusion,
	const Graphics::RenderTarget& reflection, Graphics::DepthTarget depth, const Matrix44& projectionView ) {
	setConstants( GBufferView::Final, projectionView );
	Graphics::Manager gm = Graphics::Manager::instance();
	gm.setBlendMode( Graphics::BLEND_ADDITIVE ); // アルファ 1 で足す（空の分を引く負の数も、RGBA16F なのでそのまま足せる）
	gm.setShader( mReflectionShader );
	gm.setShaderResource( ShaderSlots::kGBufferAlbedo, gbuffer.albedo() );
	gm.setShaderResource( ShaderSlots::kGBufferNormal, gbuffer.normal() );
	gm.setShaderResource( ShaderSlots::kGBufferMaterial, gbuffer.material() );
	gm.setShaderResource( ShaderSlots::kSceneDepth, depth );
	gm.setShaderResource( ShaderSlots::kOcclusion, occlusion );
	gm.setShaderResource( ShaderSlots::kReflection, reflection );
	gm.drawFullScreen();
	// 後から描く物（固定パイプライン）に残さない。混ぜ方もライティングの後と同じ（不透明）へ戻す
	gm.setBlendMode( Graphics::BLEND_OPAQUE );
	gm.setShader( Graphics::Shader() );
	gm.setShaderResource( ShaderSlots::kGBufferAlbedo, Graphics::RenderTarget() );
	gm.setShaderResource( ShaderSlots::kGBufferNormal, Graphics::RenderTarget() );
	gm.setShaderResource( ShaderSlots::kGBufferMaterial, Graphics::RenderTarget() );
	gm.setShaderResource( ShaderSlots::kSceneDepth, Graphics::DepthTarget() );
	gm.setShaderResource( ShaderSlots::kOcclusion, Graphics::RenderTarget() );
	gm.setShaderResource( ShaderSlots::kReflection, Graphics::RenderTarget() );
	gm.setConstantBuffer( kConstantSlot, Graphics::ConstantBuffer() );
}

void DeferredLighting::drawView( GBufferView view, const GBuffer& gbuffer, const Graphics::RenderTarget& occlusion,
	const Graphics::RenderTarget& reflection, const Graphics::RenderTarget& hiZ, Graphics::DepthTarget depth, const Matrix44& projectionView ) {
	ASSERT( view != GBufferView::Final && "DeferredLighting : Final is not a G-Buffer view (use drawLighting)." );
	Graphics::Manager gm = Graphics::Manager::instance();
	gm.setShaderResource( ShaderSlots::kReflection, reflection ); // 表示の SSR・SSR color が読む
	gm.setShaderResource( ShaderSlots::kHiZView, hiZ );           // 表示の Hi-Z が読む
	drawFullScreen( view, gbuffer, occlusion, depth, projectionView );
	gm.setShaderResource( ShaderSlots::kReflection, Graphics::RenderTarget() );
	gm.setShaderResource( ShaderSlots::kHiZView, Graphics::RenderTarget() );
}

void DeferredLighting::setConstants( GBufferView view, const Matrix44& projectionView ) {
	DeferredConstants constants;
	Matrix44 inverse;
	inverse.setInverse( projectionView );
	ShaderConstants::copyMatrix44( inverse, constants.inverseProjectionView );
	constants.view = static_cast< float >( view );
	constants.depthViewRange = kDepthViewRange;
	constants.hiZViewLevel = static_cast< float >( hiZViewLevel() );
	constants.unused = 0.f;
	ShaderConstants::copyDepthRow( projectionView, constants.viewDepthRow );
	mConstants.set( &constants, sizeof( constants ) );
	Graphics::Manager::instance().setConstantBuffer( kConstantSlot, mConstants );
}

void DeferredLighting::drawFullScreen( GBufferView view, const GBuffer& gbuffer, const Graphics::RenderTarget& occlusion,
	Graphics::DepthTarget depth, const Matrix44& projectionView ) {
	setConstants( view, projectionView );
	Graphics::Manager gm = Graphics::Manager::instance();
	gm.setBlendMode( Graphics::BLEND_OPAQUE ); // 照らした色・中身の数をそのまま書く。下の絵とは混ぜない
	gm.setShader( mShader );
	gm.setShaderResource( ShaderSlots::kGBufferAlbedo, gbuffer.albedo() );
	gm.setShaderResource( ShaderSlots::kGBufferNormal, gbuffer.normal() );
	gm.setShaderResource( ShaderSlots::kGBufferMaterial, gbuffer.material() );
	gm.setShaderResource( ShaderSlots::kSceneDepth, depth );
	gm.setShaderResource( ShaderSlots::kOcclusion, occlusion ); // ライティング（環境光）と表示の SSAO が読む
	if ( mMoonTexture.isReady() ) {
		gm.setShaderResource( ShaderSlots::kMoonTexture, mMoonTexture ); // 読み込みが終わるまでは付けない（月は形が無いので描かれない）
	}
	gm.drawFullScreen();
	// 後から描く物（固定パイプライン）に残さない
	gm.setShader( Graphics::Shader() );
	gm.setShaderResource( ShaderSlots::kGBufferAlbedo, Graphics::RenderTarget() );
	gm.setShaderResource( ShaderSlots::kGBufferNormal, Graphics::RenderTarget() );
	gm.setShaderResource( ShaderSlots::kGBufferMaterial, Graphics::RenderTarget() );
	gm.setShaderResource( ShaderSlots::kSceneDepth, Graphics::DepthTarget() );
	gm.setShaderResource( ShaderSlots::kMoonTexture, Graphics::Texture() );
	gm.setShaderResource( ShaderSlots::kOcclusion, Graphics::RenderTarget() );
	gm.setConstantBuffer( kConstantSlot, Graphics::ConstantBuffer() );
}

const char* DeferredLighting::errorMessage() const {
	const char* message = mShader.errorMessage();
	if ( message[ 0 ] != '\0' ) {
		return message;
	}
	message = mFogShader.errorMessage();
	if ( message[ 0 ] != '\0' ) {
		return message;
	}
	return mReflectionShader.errorMessage();
}
