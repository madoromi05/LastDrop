#include "GameSrc/Render/PostProcess/ScreenSpaceReflection.h"
#include "GameSrc/Render/Lighting/GBuffer.h"
#include "GameSrc/Render/PostProcess/SsrParams.h"

#include "GameLib/GameLib.h"
#include "GameLib/Graphics/Manager.h"
#include "GameLib/Math/Vector4.h"

#include <cmath>

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const char* kShaderFile = "Resource/Shader/Ssr.hlsl";
	const char* kBlurShaderFile = "Resource/Shader/SsrBlur.hlsl";

	const float kResolutionScale = 0.5f;	// 描画先の大きさ（画面に対する倍率）。SsrCommon.hlsli の kSsrHalfStep と揃える

	// Ssr.hlsl の番号と揃える（GBuffer.hlsli の t3 法線・t4 材質・t5 深度、t6 シーンの色 / b1）
	const int kNormalSlot = Graphics::Manager::FIRST_USER_TEXTURE_SLOT + 1;
	const int kMaterialSlot = Graphics::Manager::FIRST_USER_TEXTURE_SLOT + 2;
	const int kDepthSlot = Graphics::Manager::FIRST_USER_TEXTURE_SLOT + 3;
	const int kSceneColorSlot = Graphics::Manager::FIRST_USER_TEXTURE_SLOT + 4;
	const int kHiZSlot = Graphics::Manager::FIRST_USER_TEXTURE_SLOT + 5;	// Ssr.hlsl の t7
	const int kConstantSlot = Graphics::Manager::FIRST_USER_CONSTANT_SLOT;
	const int kBlurSourceSlot = Graphics::Manager::FIRST_USER_TEXTURE_SLOT;	// SsrBlur.hlsl の番号（t2 ぼかす元・t5 深度 / b1）

	// 映り込み無し（確からしさ 0）。SSR を切っている間の結果
	const Vector4 kNoReflection( 0.f, 0.f, 0.f, 0.f );
	const float kHorizontal[ 2 ] = { 1.f, 0.f };
	const float kVertical[ 2 ] = { 0.f, 1.f };

	const int kMatrixFloats = 16;
	const int kRowFloats = 4;
	// Ssr.hlsl の cbuffer SsrConstants と同じ並び（16 バイトずつの区切りに合わせる）
	struct SsrConstants{
		float inverseProjectionView[ kMatrixFloats ]; // 行ごと（Matrix44 の m00..m33 の順）
		float projectionView[ kMatrixFloats ];
		float maxRoughness;
		float steps;
		float maxDistance;
		float thickness;
		float edgeFade;
		float intensity;
		float noiseFrame; // ノイズのフレームの番号（TAA のとき毎フレーム変わる）
		float stochastic; // 粗い面の反射の向きをばらすか（1 / 0）
		float hiZLevels;        // Hi-Z の段の数
		float hiZMaxIterations; // Hi-Z で進む回数の上限
		float useHiZ;           // Hi-Z で進むか（1 / 0）
		float unused2;
	};
	static_assert( sizeof( SsrConstants ) == sizeof( float ) * ( kMatrixFloats * 2 + kRowFloats * 3 ), "SsrConstants : keep the HLSL layout." );
	// SsrBlur.hlsl の cbuffer SsrBlurConstants と同じ並び
	struct SsrBlurConstants{
		float inverseProjectionView[ kMatrixFloats ];
		float viewDepthRow[ kRowFloats ]; // 射影×ビューの 4 行目（m30..m33）
		float direction[ 2 ];
		float roughAware; // 粗さで読む間隔を広げるか（1 / 0。ばらすときだけ）
		float unused;
	};
	static_assert( sizeof( SsrBlurConstants ) == sizeof( float ) * ( kMatrixFloats + kRowFloats * 2 ), "SsrBlurConstants : keep the HLSL layout." );

	void copyMatrix( const Matrix44& matrix, float* destination ) {
		for ( int i = 0; i < kMatrixFloats; ++i ) {
			destination[ i ] = ( &matrix.m00 )[ i ];
		}
	}
}

SsrParams& ssrParams() {
	static SsrParams sParams; // 起動中ずっと 1 つ（F6 で書き換える）
	return sParams;
}

bool& ssrEnabled() {
	static bool sEnabled = true; // 起動中ずっと 1 つ（F6 で書き換える）
	return sEnabled;
}

bool& ssrStochasticEnabled() {
	static bool sEnabled = true; // 起動中ずっと 1 つ（F6 で書き換える）
	return sEnabled;
}

ScreenSpaceReflection::ScreenSpaceReflection() {
	mReflection = Graphics::RenderTarget::createScreenSized( kResolutionScale, Graphics::RENDER_TARGET_RGBA16F );
	mBlurScratch = Graphics::RenderTarget::createScreenSized( kResolutionScale, Graphics::RENDER_TARGET_RGBA16F );
	mShader = Graphics::Shader::create( kShaderFile );
	mBlurShader = Graphics::Shader::create( kBlurShaderFile );
	mConstants = Graphics::ConstantBuffer::create( sizeof( SsrConstants ) );
	mBlurConstants = Graphics::ConstantBuffer::create( sizeof( SsrBlurConstants ) );
}

void ScreenSpaceReflection::buildHiZ( Graphics::DepthTarget depth ) {
	if ( hiZEnabled() ) {
		mHiZ.draw( depth );
	}
}

void ScreenSpaceReflection::draw( const GBuffer& gbuffer, const Graphics::RenderTarget& sceneColor, Graphics::DepthTarget depth,
	const Matrix44& projectionView, int noiseFrame ) {
	const SsrParams& params = ssrParams();
	SsrConstants constants;
	Matrix44 inverse;
	inverse.setInverse( projectionView );
	copyMatrix( inverse, constants.inverseProjectionView );
	copyMatrix( projectionView, constants.projectionView );
	constants.maxRoughness = params.maxRoughness;
	constants.steps = std::floor( params.steps + 0.5f ); // 歩数は整数（F6 のスライダーは小数で動くため丸める）
	constants.maxDistance = params.maxDistance;
	constants.thickness = params.thickness;
	constants.edgeFade = params.edgeFade;
	constants.intensity = params.intensity;
	constants.noiseFrame = static_cast< float >( noiseFrame );
	constants.stochastic = ssrStochasticEnabled() ? 1.f : 0.f;
	constants.hiZLevels = static_cast< float >( mHiZ.mipLevels() );
	constants.hiZMaxIterations = std::floor( params.hiZMaxIterations + 0.5f ); // 整数（歩数と同じ理由）
	constants.useHiZ = hiZEnabled() ? 1.f : 0.f;
	constants.unused2 = 0.f;
	mConstants.set( &constants, sizeof( constants ) );

	Graphics::Manager gm = Graphics::Manager::instance();
	gm.setRenderTargets( &mReflection, 1, Graphics::DepthTarget() ); // 描画範囲が半分の大きさになる
	gm.setBlendMode( Graphics::BLEND_OPAQUE ); // 全画面を上書き（前のフレームの結果とは混ぜない）
	gm.setShader( mShader );
	gm.setShaderResource( kNormalSlot, gbuffer.normal() ); // 描画先の切り替えで外れるので、切り替えた後に付ける
	gm.setShaderResource( kMaterialSlot, gbuffer.material() );
	gm.setShaderResource( kDepthSlot, depth );
	gm.setShaderResource( kSceneColorSlot, sceneColor );
	if ( hiZEnabled() ) {
		gm.setShaderResource( kHiZSlot, mHiZ.result() );
	}
	gm.setConstantBuffer( kConstantSlot, mConstants );
	gm.drawFullScreen();
	gm.setShaderResource( kHiZSlot, Graphics::RenderTarget() );
	gm.setShaderResource( kNormalSlot, Graphics::RenderTarget() );
	gm.setShaderResource( kSceneColorSlot, Graphics::RenderTarget() );
	// 横 → 縦（7-4）。縦の結果は作った先へ戻すので、結果はいつも mReflection。ぼかしも粗さを読む
	blur( mReflection, mBlurScratch, gbuffer, depth, inverse, projectionView, kHorizontal[ 0 ], kHorizontal[ 1 ] );
	blur( mBlurScratch, mReflection, gbuffer, depth, inverse, projectionView, kVertical[ 0 ], kVertical[ 1 ] );
	// 後から描く物（固定パイプライン）に残さない
	gm.setShader( Graphics::Shader() );
	gm.setShaderResource( kMaterialSlot, Graphics::RenderTarget() );
	gm.setShaderResource( kDepthSlot, Graphics::DepthTarget() );
	gm.setConstantBuffer( kConstantSlot, Graphics::ConstantBuffer() );
}

void ScreenSpaceReflection::blur( const Graphics::RenderTarget& source, const Graphics::RenderTarget& destination,
	const GBuffer& gbuffer, Graphics::DepthTarget depth, const Matrix44& inverse, const Matrix44& projectionView, float directionX, float directionY ) {
	SsrBlurConstants constants;
	copyMatrix( inverse, constants.inverseProjectionView );
	const int kDepthRowStart = kMatrixFloats - kRowFloats; // m30..m33
	for ( int i = 0; i < kRowFloats; ++i ) {
		constants.viewDepthRow[ i ] = ( &projectionView.m00 )[ kDepthRowStart + i ];
	}
	constants.direction[ 0 ] = directionX;
	constants.direction[ 1 ] = directionY;
	constants.roughAware = ssrStochasticEnabled() ? 1.f : 0.f;
	constants.unused = 0.f;
	mBlurConstants.set( &constants, sizeof( constants ) );

	Graphics::Manager gm = Graphics::Manager::instance();
	gm.setShaderResource( kBlurSourceSlot, Graphics::RenderTarget() ); // 前の回の読み元（今回の描画先）を外してから付け替える
	gm.setRenderTargets( &destination, 1, Graphics::DepthTarget() );
	gm.setShader( mBlurShader );
	gm.setShaderResource( kBlurSourceSlot, source ); // 描画先の切り替えで外れるので、切り替えた後に付ける
	gm.setShaderResource( kMaterialSlot, gbuffer.material() );
	gm.setShaderResource( kDepthSlot, depth );
	gm.setConstantBuffer( kConstantSlot, mBlurConstants );
	gm.drawFullScreen();
	gm.setShaderResource( kBlurSourceSlot, Graphics::RenderTarget() );
}

void ScreenSpaceReflection::clear() {
	Graphics::Manager::instance().clear( mReflection, kNoReflection );
}

const Graphics::RenderTarget& ScreenSpaceReflection::result() const {
	return mReflection;
}

const Graphics::RenderTarget& ScreenSpaceReflection::hiZ() const {
	return mHiZ.result();
}

const char* ScreenSpaceReflection::errorMessage() const {
	const char* message = mShader.errorMessage();
	if ( message[ 0 ] != '\0' ) {
		return message;
	}
	message = mBlurShader.errorMessage();
	if ( message[ 0 ] != '\0' ) {
		return message;
	}
	return mHiZ.errorMessage();
}
