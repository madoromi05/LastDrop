#include "GameSrc/Render/DeferredLighting.h"
#include "GameSrc/Render/GBuffer.h"

#include "GameLib/GameLib.h"
#include "GameLib/Graphics/Manager.h"
#include "GameLib/Graphics/RenderTarget.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const char* kShaderFile = "Resource/Shader/DeferredLighting.hlsl";
	const char* kFogShaderFile = "Resource/Shader/Fog.hlsl";

	// DeferredLighting.hlsl / GBuffer.hlsli の番号と揃える（t2〜t5 / b1）。
	const int kAlbedoSlot = Graphics::Manager::FIRST_USER_TEXTURE_SLOT;
	const int kNormalSlot = kAlbedoSlot + 1;
	const int kMaterialSlot = kNormalSlot + 1;
	const int kDepthSlot = kMaterialSlot + 1;
	// SSAO の結果（t10。t6〜t9 は影のマップ。LightList.hlsli）。AmbientOcclusion::result。
	const int kOcclusionSlot = Graphics::Manager::FIRST_USER_TEXTURE_SLOT + 8;
	const int kConstantSlot = Graphics::Manager::FIRST_USER_CONSTANT_SLOT;

	// 深度の表示で、目からこの距離を白にする（画面で見て決めた値ではない初期値）。
	const float kDepthViewRange = 60.f;

	const int kMatrixFloats = 16;
	const int kRowFloats = 4;
	const int kParamFloats = 4 + kRowFloats;
	// DeferredConstants.hlsli の cbuffer DeferredConstants と同じ並び（16 バイトずつの区切りに合わせる）。
	struct DeferredConstants{
		float inverseProjectionView[ kMatrixFloats ]; // 行ごと（Matrix44 の m00..m33 の順）
		float view;                                   // 表示する物（GBufferView の番号）
		float depthViewRange;
		float unused[ 2 ];
		float viewDepthRow[ kRowFloats ];             // 射影×ビューの 4 行目（位置との内積が目からの奥行き。SSAO の拡大に使う）
	};
	static_assert( sizeof( DeferredConstants ) == sizeof( float ) * ( kMatrixFloats + kParamFloats ), "DeferredConstants : keep the HLSL layout." );

	const char* const kViewNames[] = { "Final", "Albedo", "Normal", "Roughness", "Metallic", "Emission", "Depth", "Position", "Cascade", "Shadow maps", "Fog", "SSAO" };
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
	mConstants = Graphics::ConstantBuffer::create( sizeof( DeferredConstants ) );
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
	gm.setShaderResource( kDepthSlot, depth );
	gm.drawFullScreen();
	// 後から描く物（固定パイプライン）に残さない。混ぜ方もライティングの後と同じ（不透明）へ戻す。
	gm.setBlendMode( Graphics::BLEND_OPAQUE );
	gm.setShader( Graphics::Shader() );
	gm.setShaderResource( kDepthSlot, Graphics::DepthTarget() );
	gm.setConstantBuffer( kConstantSlot, Graphics::ConstantBuffer() );
}

void DeferredLighting::drawView( GBufferView view, const GBuffer& gbuffer, const Graphics::RenderTarget& occlusion,
	Graphics::DepthTarget depth, const Matrix44& projectionView ) {
	ASSERT( view != GBufferView::Final && "DeferredLighting : Final is not a G-Buffer view (use drawLighting)." );
	drawFullScreen( view, gbuffer, occlusion, depth, projectionView );
}

void DeferredLighting::setConstants( GBufferView view, const Matrix44& projectionView ) {
	DeferredConstants constants;
	Matrix44 inverse;
	inverse.setInverse( projectionView );
	for ( int i = 0; i < kMatrixFloats; ++i ) {
		constants.inverseProjectionView[ i ] = ( &inverse.m00 )[ i ];
	}
	constants.view = static_cast< float >( view );
	constants.depthViewRange = kDepthViewRange;
	constants.unused[ 0 ] = 0.f;
	constants.unused[ 1 ] = 0.f;
	const int kDepthRowStart = kMatrixFloats - kRowFloats; // m30..m33
	for ( int i = 0; i < kRowFloats; ++i ) {
		constants.viewDepthRow[ i ] = ( &projectionView.m00 )[ kDepthRowStart + i ];
	}
	mConstants.set( &constants, sizeof( constants ) );
	Graphics::Manager::instance().setConstantBuffer( kConstantSlot, mConstants );
}

void DeferredLighting::drawFullScreen( GBufferView view, const GBuffer& gbuffer, const Graphics::RenderTarget& occlusion,
	Graphics::DepthTarget depth, const Matrix44& projectionView ) {
	setConstants( view, projectionView );
	Graphics::Manager gm = Graphics::Manager::instance();
	gm.setBlendMode( Graphics::BLEND_OPAQUE ); // 照らした色・中身の数をそのまま書く。下の絵とは混ぜない
	gm.setShader( mShader );
	gm.setShaderResource( kAlbedoSlot, gbuffer.albedo() );
	gm.setShaderResource( kNormalSlot, gbuffer.normal() );
	gm.setShaderResource( kMaterialSlot, gbuffer.material() );
	gm.setShaderResource( kDepthSlot, depth );
	gm.setShaderResource( kOcclusionSlot, occlusion ); // ライティング（環境光）と表示の SSAO が読む
	gm.drawFullScreen();
	// 後から描く物（固定パイプライン）に残さない。
	gm.setShader( Graphics::Shader() );
	gm.setShaderResource( kAlbedoSlot, Graphics::RenderTarget() );
	gm.setShaderResource( kNormalSlot, Graphics::RenderTarget() );
	gm.setShaderResource( kMaterialSlot, Graphics::RenderTarget() );
	gm.setShaderResource( kDepthSlot, Graphics::DepthTarget() );
	gm.setShaderResource( kOcclusionSlot, Graphics::RenderTarget() );
	gm.setConstantBuffer( kConstantSlot, Graphics::ConstantBuffer() );
}

const char* DeferredLighting::errorMessage() const {
	const char* message = mShader.errorMessage();
	if ( message[ 0 ] != '\0' ) {
		return message;
	}
	return mFogShader.errorMessage();
}
