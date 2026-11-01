#include "GameSrc/Render/GBuffer.h"
#include "GameSrc/Render/RenderPass.h"
#include "GameSrc/Render/PuddleParams.h"

#include "GameLib/Graphics/Manager.h"
#include "GameLib/Graphics/Enum.h"
#include "GameLib/Graphics/ConstantBuffer.h"
#include "GameLib/Math/Vector4.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const char* kStaticMeshShaderFile = "Resource/Shader/StaticMeshGBuffer.hlsl";
	const float kScreenScale = 1.f; // PostProcess の深度と同じ大きさ（setRenderTargets は大きさの違う描画先を一緒に付けられない）

	// 並びは Resource/Shader/GBuffer.hlsli と揃える。
	const int kAlbedoIndex = 0;
	const int kNormalIndex = 1;
	const int kMaterialIndex = 2;
	const Graphics::RenderTargetFormat kFormats[] = {
		Graphics::RENDER_TARGET_RGBA8_SRGB, // 色。8bit にリニアで入れると暗い色に段差が出るので sRGB
		Graphics::RENDER_TARGET_RGBA16F,    // 法線（-1〜1）。8bit に入らない数なので浮動小数
		Graphics::RENDER_TARGET_RGBA8,      // AO・粗さ・金属・発光（どれも 0〜1。色ではないので sRGB にしない）
	};
	// GBuffer.hlsli の cbuffer GBufferMaterial の番号（b3）。b1 はキャラの骨（Skinning.hlsli）、b2 は段階 3-b のライトの一覧に使う。
	const int kSurfaceSlot = Graphics::Manager::FIRST_USER_CONSTANT_SLOT + 2;
	// デカールが書く描画先の数（RT0 色・RT1 法線。並びの先頭から）と、深度を読む番号（Decal.hlsl の t5）。
	const int kDecalTargetNumber = 2;
	const int kDecalDepthSlot = Graphics::Manager::FIRST_USER_TEXTURE_SLOT + 3;
	// 何も描かれていない画素は、どの面の情報も 0（ライティングのパスでは深度で見分ける）。
	const Vector4 kClearValue( 0.f, 0.f, 0.f, 0.f );

	// 雨上がりの水たまり（SsrDecalMaterial_Plan.txt 6 章）。GBuffer.hlsli の gPuddleMask（t12）と cbuffer GBufferPassConstants（b4）。
	const char* kPuddleMaskFile = "Resource/puddle_mask.dds";
	const int kPuddleMaskSlot = Graphics::Manager::FIRST_USER_TEXTURE_SLOT + 10;
	const int kPassSlot = Graphics::Manager::FIRST_USER_CONSTANT_SLOT + 3;
	// 動きの種類を RT1 の a に書く数（Taa_Plan.txt 6 章。MotionKind の順。TAA・表示の Motion が読む。GBuffer.hlsli と揃える）。
	const float kMotionValues[] = { 0.f, 0.5f, 1.f };
	const float kNoCoverage = 0.f;
	const float kWhole = 1.f;
	// 水たまりを書かないときのしきい値。模様の値（0〜1）が届かないので、水たまりも濡れた縁もできない
	// （1 − 0 = 1 だと、模様の一番白い画素だけ濡れた縁が残る）。
	const float kUnreachableThreshold = 2.f;
	// GBuffer.hlsli の cbuffer GBufferPassConstants と同じ並び（16 バイトずつの区切りに合わせる）。
	struct PassConstants{
		float threshold;       // 模様の値がこれより大きい所が水たまり（1 − 面積の割合）
		float wetBand;
		float inverseTileSize; // ワールドの x・z に掛けて、模様を読む位置にする
		float puddleRoughness;
		float wetRoughness;
		float wetDarkening;
		float unused[ 2 ];
		float motion;          // 動きの種類（kMotionValues）
		float unusedMotion[ 3 ];
	};

	// 材質の画像（ORM）が無い材質に付ける白 1 画素（AO 1・粗さ 1・金属 1。シェーダは数値に掛けるので、数値がそのまま使われる）。
	const unsigned kWhitePixel = 0xffffffff;
	Graphics::Texture createWhiteTexture() {
		Graphics::Texture texture = Graphics::Texture::create( 1, 1, false );
		unsigned* pixels = nullptr;
		int pitch = 0;
		texture.lock( &pixels, &pitch );
		*pixels = kWhitePixel;
		texture.unlock( &pixels );
		return texture;
	}
}

PuddleParams& puddleParams() {
	static PuddleParams sParams; // 起動中ずっと 1 つ（F6 で書き換える）
	return sParams;
}

bool& puddlesEnabled() {
	static bool sEnabled = true; // 起動中ずっと 1 つ（F6 で書き換える）
	return sEnabled;
}

GBuffer::GBuffer() :
	mMotion( MotionKind::Static ) {
	static_assert( sizeof( kFormats ) / sizeof( kFormats[ 0 ] ) == kTargetNumber, "GBuffer : one format per target." );
	static_assert( sizeof( kMotionValues ) / sizeof( kMotionValues[ 0 ] ) == static_cast< int >( MotionKind::FirstPerson ) + 1,
		"GBuffer : one value per MotionKind." );
	for ( int i = 0; i < kTargetNumber; ++i ) {
		mTargets[ i ] = Graphics::RenderTarget::createScreenSized( kScreenScale, kFormats[ i ] );
	}
	mStaticMeshShader = Graphics::Shader::create( kStaticMeshShaderFile );
	mSurfaceConstants = Graphics::ConstantBuffer::create( sizeof( SurfaceParams ) );
	mDefaultOrm = createWhiteTexture();
	mPuddleMask = Graphics::Texture::create( kPuddleMaskFile, Graphics::TEXTURE_COLOR_SPACE_LINEAR ); // 色ではない数
	mPassConstants = Graphics::ConstantBuffer::create( sizeof( PassConstants ) );
}

void GBuffer::begin( Graphics::DepthTarget depth ) {
	Graphics::Manager gm = Graphics::Manager::instance();
	gm.setRenderTargets( mTargets, kTargetNumber, depth );
	for ( int i = 0; i < kTargetNumber; ++i ) {
		gm.clear( mTargets[ i ], kClearValue );
	}
	// G-Buffer のアルファは色の透明度ではなくデータ（RT0・RT1 は予備の 0、RT2 は発光の強さ）なので、混ぜずにそのまま書く。
	// 前に描いた物（半透明の板・UI）のブレンドが残っていると、アルファ 0 の RT0 は何も書かれない（2026-10-28 の真っ暗の不具合）。
	gm.setBlendMode( Graphics::BLEND_OPAQUE );
	// 材質値の入れ物は描く間ずっと付けておき、中身だけ MeshMaterial::apply（RenderPass::setSurface）が物ごとに書き換える。
	gm.setConstantBuffer( kSurfaceSlot, mSurfaceConstants );
	mMotion = MotionKind::Static;
	setPassConstants();
	RenderPass::beginGBuffer( mStaticMeshShader, &mSurfaceConstants, &mDefaultOrm );
}

void GBuffer::setMotion( MotionKind motion ) {
	mMotion = motion;
	setPassConstants();
}

void GBuffer::beginDecals( Graphics::DepthTarget depth ) {
	Graphics::Manager gm = Graphics::Manager::instance();
	gm.setRenderTargets( mTargets, kDecalTargetNumber, Graphics::DepthTarget() ); // RT0・RT1 だけ（RT2 の粗さ・金属・AO は面のまま）
	gm.setShaderResource( kDecalDepthSlot, depth ); // 描画先の切り替えで外れるので、切り替えた後に付ける
}

void GBuffer::endDecals( Graphics::DepthTarget depth ) {
	Graphics::Manager gm = Graphics::Manager::instance();
	gm.setShaderResource( kDecalDepthSlot, Graphics::DepthTarget() ); // 深度を描画先へ戻す前に、読み込み元から外す
	gm.setRenderTargets( mTargets, kTargetNumber, depth );
	gm.setBlendMode( Graphics::BLEND_OPAQUE ); // begin と同じ（G-Buffer のアルファはデータなので混ぜない）
	setPassConstants(); // 描画先の付け直しで外れた水たまりの模様を付け直す
	// 材質の画像（t11）も外れている。RenderPass は「前と同じ画像なら付けない」ので、忘れさせて次の物で付け直させる。
	// 忘れないと、後から描く自キャラ・銃・ゾンビは画像の無い t11 を読み、粗さ 0（つるつる）・AO 0 になる
	// （2026-10-31 に見つけた。街灯のハイライトが小さく強く出て、金色に光って見えた）。
	RenderPass::forgetSentSurface();
}

void GBuffer::end() {
	RenderPass::endGBuffer();
	// 後から描く物（固定パイプライン）に残さない。
	Graphics::Manager gm = Graphics::Manager::instance();
	gm.setShader( Graphics::Shader() );
	gm.setConstantBuffer( kSurfaceSlot, Graphics::ConstantBuffer() );
	gm.setConstantBuffer( kPassSlot, Graphics::ConstantBuffer() );
	gm.setShaderResource( kPuddleMaskSlot, Graphics::Texture() );
}

void GBuffer::setPassConstants() {
	Graphics::Manager gm = Graphics::Manager::instance();
	// 模様の読み込みが終わるまでは、面積 0（どこも水たまりにならない）として書く。isReady は読み込みを進めるので毎回呼ぶ。
	const bool maskReady = mPuddleMask.isReady();
	const PuddleParams& params = puddleParams();
	const float coverage = ( puddlesEnabled() && maskReady ) ? params.coverage : kNoCoverage;
	PassConstants constants;
	constants.threshold = ( coverage > kNoCoverage ) ? kWhole - coverage : kUnreachableThreshold;
	constants.wetBand = params.wetBand;
	constants.inverseTileSize = kWhole / params.tileSize;
	constants.puddleRoughness = params.puddleRoughness;
	constants.wetRoughness = params.wetRoughness;
	constants.wetDarkening = params.wetDarkening;
	constants.unused[ 0 ] = 0.f;
	constants.unused[ 1 ] = 0.f;
	constants.motion = kMotionValues[ static_cast< int >( mMotion ) ];
	constants.unusedMotion[ 0 ] = 0.f;
	constants.unusedMotion[ 1 ] = 0.f;
	constants.unusedMotion[ 2 ] = 0.f;
	mPassConstants.set( &constants, sizeof( constants ) );
	gm.setConstantBuffer( kPassSlot, mPassConstants );
	if ( maskReady ) {
		gm.setShaderResource( kPuddleMaskSlot, mPuddleMask ); // 描画先を付けた後に付ける（setRenderTargets が t2 以降を外すため）
	}
}

const Graphics::RenderTarget& GBuffer::albedo() const {
	return mTargets[ kAlbedoIndex ];
}

const Graphics::RenderTarget& GBuffer::normal() const {
	return mTargets[ kNormalIndex ];
}

const Graphics::RenderTarget& GBuffer::material() const {
	return mTargets[ kMaterialIndex ];
}

const char* GBuffer::errorMessage() const {
	return mStaticMeshShader.errorMessage();
}
