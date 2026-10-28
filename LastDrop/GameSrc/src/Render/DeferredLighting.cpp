#include "GameSrc/Render/DeferredLighting.h"
#include "GameSrc/Render/GBuffer.h"

#include "GameLib/GameLib.h"
#include "GameLib/Graphics/Manager.h"
#include "GameLib/Graphics/RenderTarget.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const char* kShaderFile = "Resource/Shader/DeferredLighting.hlsl";

	// DeferredLighting.hlsl / GBuffer.hlsli の番号と揃える（t2〜t5 / b1）。
	const int kAlbedoSlot = Graphics::Manager::FIRST_USER_TEXTURE_SLOT;
	const int kNormalSlot = kAlbedoSlot + 1;
	const int kSpecularSlot = kNormalSlot + 1;
	const int kDepthSlot = kSpecularSlot + 1;
	const int kConstantSlot = Graphics::Manager::FIRST_USER_CONSTANT_SLOT;

	// 表示の範囲。深度は目からこの距離で白（画面で見て決めた値ではない初期値）、鋭さは材質の一番大きい値（48。Resource の txt）で白。
	const float kDepthViewRange = 60.f;
	const float kSharpnessViewMax = 48.f;

	const int kMatrixFloats = 16;
	const int kParamFloats = 4;
	// DeferredLighting.hlsl の cbuffer DeferredConstants と同じ並び（16 バイトずつの区切りに合わせる）。
	struct DeferredConstants{
		float inverseProjectionView[ kMatrixFloats ]; // 行ごと（Matrix44 の m00..m33 の順）
		float view;                                   // 表示する物（GBufferView の番号）
		float depthViewRange;
		float sharpnessViewMax;
		float unused;
	};
	static_assert( sizeof( DeferredConstants ) == sizeof( float ) * ( kMatrixFloats + kParamFloats ), "DeferredConstants : keep the HLSL layout." );

	const char* const kViewNames[] = { "Final", "Albedo", "Normal", "Specular", "Sharpness", "Depth", "Position" };
	static_assert( sizeof( kViewNames ) / sizeof( kViewNames[ 0 ] ) == static_cast< int >( GBufferView::Count ), "GBufferView : one name per view." );
}

const char* gbufferViewName( GBufferView view ) {
	return kViewNames[ static_cast< int >( view ) ];
}

GBufferView& gbufferView() {
	static GBufferView sView = GBufferView::Final; // 起動中ずっと 1 つ（F6 で書き換える）
	return sView;
}

RenderPath& renderPath() {
	static RenderPath sPath = RenderPath::Deferred; // 起動中ずっと 1 つ（F6 で書き換える）
	return sPath;
}

DeferredLighting::DeferredLighting() {
	mShader = Graphics::Shader::create( kShaderFile );
	mConstants = Graphics::ConstantBuffer::create( sizeof( DeferredConstants ) );
}

void DeferredLighting::drawLighting( const GBuffer& gbuffer, Graphics::DepthTarget depth, const Matrix44& projectionView ) {
	drawFullScreen( GBufferView::Final, gbuffer, depth, projectionView );
}

void DeferredLighting::drawView( GBufferView view, const GBuffer& gbuffer, Graphics::DepthTarget depth, const Matrix44& projectionView ) {
	ASSERT( view != GBufferView::Final && "DeferredLighting : Final is not a G-Buffer view (use drawLighting)." );
	drawFullScreen( view, gbuffer, depth, projectionView );
}

void DeferredLighting::drawFullScreen( GBufferView view, const GBuffer& gbuffer, Graphics::DepthTarget depth, const Matrix44& projectionView ) {
	DeferredConstants constants;
	Matrix44 inverse;
	inverse.setInverse( projectionView );
	for ( int i = 0; i < kMatrixFloats; ++i ) {
		constants.inverseProjectionView[ i ] = ( &inverse.m00 )[ i ];
	}
	constants.view = static_cast< float >( view );
	constants.depthViewRange = kDepthViewRange;
	constants.sharpnessViewMax = kSharpnessViewMax;
	constants.unused = 0.f;
	mConstants.set( &constants, sizeof( constants ) );

	Graphics::Manager gm = Graphics::Manager::instance();
	gm.setBlendMode( Graphics::BLEND_OPAQUE ); // 照らした色・中身の数をそのまま書く。下の絵とは混ぜない
	gm.setShader( mShader );
	gm.setShaderResource( kAlbedoSlot, gbuffer.albedo() );
	gm.setShaderResource( kNormalSlot, gbuffer.normal() );
	gm.setShaderResource( kSpecularSlot, gbuffer.specular() );
	gm.setShaderResource( kDepthSlot, depth );
	gm.setConstantBuffer( kConstantSlot, mConstants );
	gm.drawFullScreen();
	// 後から描く物（固定パイプライン）に残さない。
	gm.setShader( Graphics::Shader() );
	gm.setShaderResource( kAlbedoSlot, Graphics::RenderTarget() );
	gm.setShaderResource( kNormalSlot, Graphics::RenderTarget() );
	gm.setShaderResource( kSpecularSlot, Graphics::RenderTarget() );
	gm.setShaderResource( kDepthSlot, Graphics::DepthTarget() );
	gm.setConstantBuffer( kConstantSlot, Graphics::ConstantBuffer() );
}

const char* DeferredLighting::errorMessage() const {
	return mShader.errorMessage();
}
