#include "GameSrc/Render/PostProcess/AmbientOcclusion.h"
#include "GameSrc/Render/Lighting/GBuffer.h"
#include "GameSrc/Render/PostProcess/SsaoParams.h"
#include "GameSrc/Render/ShaderSlots.h"
#include "GameSrc/Render/ShaderConstants.h"

#include "GameLib/GameLib.h"
#include "GameLib/Graphics/Manager.h"
#include "GameLib/Math/Vector4.h"

#include <cmath>

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const char* kShaderFile = "Resource/Shader/Ssao.hlsl";
	const char* kBlurShaderFile = "Resource/Shader/SsaoBlur.hlsl";

	const float kResolutionScale = 0.5f;	// 描画先の大きさ（画面に対する倍率）。Ssao.hlsl の kHalfResolutionStep と揃える
	const float kMaxScreenRadius = 0.1f;	// 画面の上の半径の上限（画面の高さに対する割合。5-3）。手元の銃・腕で点が画面中に散らばらないように。調整しない

	const int kConstantSlot = Graphics::Manager::FIRST_USER_CONSTANT_SLOT;	// Ssao.hlsl の b1。G-Buffer の法線・深度の番号は ShaderSlots
	const int kBlurSourceSlot = Graphics::Manager::FIRST_USER_TEXTURE_SLOT;	// SsaoBlur.hlsl の番号と揃える（t2 / b1）

	// SsaoBlur.hlsl の cbuffer SsaoBlurConstants と同じ並び（float 4 つ＝16 バイト）。xy が 1 画素ずつ進む向き
	struct SsaoBlurConstants{
		float direction[ 2 ];
		float unused[ 2 ];
	};
	const SsaoBlurConstants kHorizontal = { { 1.f, 0.f }, { 0.f, 0.f } };
	const SsaoBlurConstants kVertical = { { 0.f, 1.f }, { 0.f, 0.f } };

	const Vector4 kNoOcclusionColor( 1.f, 10000.f, 0.f, 1.f );	// SSAO を切っている間の結果（r：AO 1 ＝遮られていない、g：Ssao.hlsl の kSkyDistance と同じ遠さ）

	const int kParamFloats = 8;
	// Ssao.hlsl の cbuffer SsaoConstants と同じ並び（16 バイトずつの区切りに合わせる）
	struct SsaoConstants{
		float inverseProjectionView[ ShaderConstants::kMatrixFloats ]; // 行ごと（Matrix44 の m00..m33 の順）
		float projectionView[ ShaderConstants::kMatrixFloats ];
		float radius;
		float bias;
		float intensity;
		float power;
		float sampleCount;
		float maxScreenRadius;
		float noiseFrame; // ノイズのフレームの番号（TAA のとき毎フレーム変わる）
		float unused;
	};
	static_assert( sizeof( SsaoConstants ) == sizeof( float ) * ( ShaderConstants::kMatrixFloats * 2 + kParamFloats ), "SsaoConstants : keep the HLSL layout." );
}

SsaoParams& ssaoParams() {
	static SsaoParams sParams; // 起動中ずっと 1 つ（F6 で書き換える）
	return sParams;
}

bool& ssaoEnabled() {
	static bool sEnabled = true; // 起動中ずっと 1 つ（F6 で書き換える）
	return sEnabled;
}

bool& ssaoBlurEnabled() {
	static bool sEnabled = true; // 起動中ずっと 1 つ（F6 で書き換える）
	return sEnabled;
}

AmbientOcclusion::AmbientOcclusion() {
	mOcclusion = Graphics::RenderTarget::createScreenSized( kResolutionScale, Graphics::RENDER_TARGET_RG16F );
	mBlurScratch = Graphics::RenderTarget::createScreenSized( kResolutionScale, Graphics::RENDER_TARGET_RG16F );
	mShader = Graphics::Shader::create( kShaderFile );
	mBlurShader = Graphics::Shader::create( kBlurShaderFile );
	mConstants = Graphics::ConstantBuffer::create( sizeof( SsaoConstants ) );
	mBlurHorizontal = Graphics::ConstantBuffer::create( sizeof( SsaoBlurConstants ) );
	mBlurHorizontal.set( &kHorizontal, sizeof( kHorizontal ) );
	mBlurVertical = Graphics::ConstantBuffer::create( sizeof( SsaoBlurConstants ) );
	mBlurVertical.set( &kVertical, sizeof( kVertical ) );
}

void AmbientOcclusion::draw( const GBuffer& gbuffer, Graphics::DepthTarget depth, const Matrix44& projectionView, int noiseFrame ) {
	Graphics::Manager gm = Graphics::Manager::instance();
	if ( !ssaoEnabled() ) {
		gm.clear( mOcclusion, kNoOcclusionColor );
		return;
	}
	const SsaoParams& params = ssaoParams();
	SsaoConstants constants;
	Matrix44 inverse;
	inverse.setInverse( projectionView );
	ShaderConstants::copyMatrix44( inverse, constants.inverseProjectionView );
	ShaderConstants::copyMatrix44( projectionView, constants.projectionView );
	constants.radius = params.radius;
	constants.bias = params.bias;
	constants.intensity = params.intensity;
	constants.power = params.power;
	constants.sampleCount = std::floor( params.sampleCount + 0.5f ); // 点の数は整数（F6 のスライダーは小数で動くため丸める）
	constants.maxScreenRadius = kMaxScreenRadius;
	constants.noiseFrame = static_cast< float >( noiseFrame );
	constants.unused = 0.f;
	mConstants.set( &constants, sizeof( constants ) );

	gm.setRenderTargets( &mOcclusion, 1, Graphics::DepthTarget() ); // 描画範囲が半分の大きさになる
	gm.setBlendMode( Graphics::BLEND_OPAQUE ); // 全画面を上書き（前のフレームの結果とは混ぜない）
	gm.setShader( mShader );
	gm.setShaderResource( ShaderSlots::kGBufferNormal, gbuffer.normal() ); // 描画先の切り替えで外れるので、切り替えた後に付ける
	gm.setShaderResource( ShaderSlots::kSceneDepth, depth );
	gm.setConstantBuffer( kConstantSlot, mConstants );
	gm.drawFullScreen();
	gm.setShaderResource( ShaderSlots::kGBufferNormal, Graphics::RenderTarget() );
	gm.setShaderResource( ShaderSlots::kSceneDepth, Graphics::DepthTarget() );
	// 横 → 縦（6-1）。縦の結果は作った先へ戻すので、結果はいつも mOcclusion
	if ( ssaoBlurEnabled() ) {
		blur( mOcclusion, mBlurScratch, mBlurHorizontal );
		blur( mBlurScratch, mOcclusion, mBlurVertical );
	}
	// 後から描く物（固定パイプライン）に残さない
	gm.setShader( Graphics::Shader() );
	gm.setConstantBuffer( kConstantSlot, Graphics::ConstantBuffer() );
}

void AmbientOcclusion::blur( const Graphics::RenderTarget& source, const Graphics::RenderTarget& destination,
	const Graphics::ConstantBuffer& direction ) {
	Graphics::Manager gm = Graphics::Manager::instance();
	gm.setShaderResource( kBlurSourceSlot, Graphics::RenderTarget() ); // 前の回の読み元（今回の描画先）を外してから付け替える
	gm.setRenderTargets( &destination, 1, Graphics::DepthTarget() );
	gm.setShader( mBlurShader );
	gm.setShaderResource( kBlurSourceSlot, source ); // 描画先の切り替えで外れるので、切り替えた後に付ける
	gm.setConstantBuffer( kConstantSlot, direction );
	gm.drawFullScreen();
	gm.setShaderResource( kBlurSourceSlot, Graphics::RenderTarget() );
}

const Graphics::RenderTarget& AmbientOcclusion::result() const {
	return mOcclusion;
}

const char* AmbientOcclusion::errorMessage() const {
	const char* message = mShader.errorMessage();
	if ( message[ 0 ] != '\0' ) {
		return message;
	}
	return mBlurShader.errorMessage();
}
