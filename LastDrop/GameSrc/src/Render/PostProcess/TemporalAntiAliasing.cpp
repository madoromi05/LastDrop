#include "GameSrc/Render/PostProcess/TemporalAntiAliasing.h"
#include "GameSrc/Render/Lighting/GBuffer.h"
#include "GameSrc/Render/PostProcess/TaaParams.h"

#include "GameLib/GameLib.h"
#include "GameLib/Graphics/Manager.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const char* kShaderFile = "Resource/Shader/Taa.hlsl";
	const char* kCopyShaderFile = "Resource/Shader/TaaCopy.hlsl";
	const float kScreenScale = 1.f; // シーンの色と同じ大きさ

	// Taa.hlsl の番号と揃える（GBuffer.hlsli の t3 法線＝動きの種類・t5 深度、t6 今の色・t7 前の履歴 / b1）。
	const int kNormalSlot = Graphics::Manager::FIRST_USER_TEXTURE_SLOT + 1;
	const int kDepthSlot = Graphics::Manager::FIRST_USER_TEXTURE_SLOT + 3;
	const int kCurrentSlot = Graphics::Manager::FIRST_USER_TEXTURE_SLOT + 4;
	const int kHistorySlot = Graphics::Manager::FIRST_USER_TEXTURE_SLOT + 5;
	const int kConstantSlot = Graphics::Manager::FIRST_USER_CONSTANT_SLOT;
	// TaaCopy.hlsl の番号（t2 新しい履歴）。
	const int kCopySourceSlot = Graphics::Manager::FIRST_USER_TEXTURE_SLOT;

	const float kUseHistory = 0.f;     // Taa.hlsl の gTaaHistoryWeight.w：履歴を使う
	const float kDiscardHistory = 1.f; // 履歴を捨てる（今の色だけ）

	const int kMatrixFloats = 16;
	const int kRowFloats = 4;
	// Taa.hlsl の cbuffer TaaConstants と同じ並び（16 バイトずつの区切りに合わせる）。
	struct TaaConstants{
		float inverseProjectionView[ kMatrixFloats ];  // 行ごと（Matrix44 の m00..m33 の順）
		float previousProjectionView[ kMatrixFloats ];
		float historyWeight;
		float movingHistoryWeight;
		float firstPersonHistoryWeight;
		float discardHistory;
		float clipGamma;
		float movingClipGamma;
		float firstPersonClipGamma;
		float unused;
	};
	static_assert( sizeof( TaaConstants ) == sizeof( float ) * ( kMatrixFloats * 2 + kRowFloats * 2 ), "TaaConstants : keep the HLSL layout." );

	void copyMatrix( const Matrix44& matrix, float* destination ) {
		for ( int i = 0; i < kMatrixFloats; ++i ) {
			destination[ i ] = ( &matrix.m00 )[ i ];
		}
	}
}

TaaParams& taaParams() {
	static TaaParams sParams; // 起動中ずっと 1 つ（F6 で書き換える）
	return sParams;
}

TemporalAntiAliasing::TemporalAntiAliasing() :
	mCurrent( 0 ),
	mValid( false ),
	mWidth( 0 ),
	mHeight( 0 ) {
	for ( int i = 0; i < kHistoryNumber; ++i ) {
		mHistory[ i ] = Graphics::RenderTarget::createScreenSized( kScreenScale, Graphics::RENDER_TARGET_RGBA16F );
	}
	mShader = Graphics::Shader::create( kShaderFile );
	mCopyShader = Graphics::Shader::create( kCopyShaderFile );
	mConstants = Graphics::ConstantBuffer::create( sizeof( TaaConstants ) );
}

void TemporalAntiAliasing::resolve( const Graphics::RenderTarget& sceneColor, const GBuffer& gbuffer, Graphics::DepthTarget depth,
	const Matrix44& jitteredProjectionView, const Matrix44& projectionView ) {
	// 画面の大きさが変わったら、履歴の中身は使えない（作り直された描画先は空）。
	const int width = sceneColor.width();
	const int height = sceneColor.height();
	const bool usesHistory = mValid && ( width == mWidth ) && ( height == mHeight );

	const TaaParams& params = taaParams();
	TaaConstants constants;
	Matrix44 inverse;
	inverse.setInverse( jitteredProjectionView );
	copyMatrix( inverse, constants.inverseProjectionView );
	copyMatrix( mPreviousProjectionView, constants.previousProjectionView );
	constants.historyWeight = params.historyWeight;
	constants.movingHistoryWeight = params.movingHistoryWeight;
	constants.firstPersonHistoryWeight = params.firstPersonHistoryWeight;
	constants.discardHistory = usesHistory ? kUseHistory : kDiscardHistory;
	constants.clipGamma = params.clipGamma;
	constants.movingClipGamma = params.movingClipGamma;
	constants.firstPersonClipGamma = params.firstPersonClipGamma;
	constants.unused = 0.f;
	mConstants.set( &constants, sizeof( constants ) );

	const int next = ( mCurrent + 1 ) % kHistoryNumber;
	Graphics::Manager gm = Graphics::Manager::instance();
	// 今の色と前の履歴を混ぜて、新しい履歴へ書く。
	gm.setRenderTargets( &mHistory[ next ], 1, Graphics::DepthTarget() );
	gm.setBlendMode( Graphics::BLEND_OPAQUE );
	gm.setShader( mShader );
	gm.setShaderResource( kNormalSlot, gbuffer.normal() ); // 描画先の切り替えで外れるので、切り替えた後に付ける
	gm.setShaderResource( kDepthSlot, depth );
	gm.setShaderResource( kCurrentSlot, sceneColor );
	gm.setShaderResource( kHistorySlot, mHistory[ mCurrent ] );
	gm.setConstantBuffer( kConstantSlot, mConstants );
	gm.drawFullScreen();
	// 新しい履歴を、シーンの色へ写し戻す。
	gm.setRenderTargets( &sceneColor, 1, Graphics::DepthTarget() ); // 付けていた読み込み元（t2 以降）はここで外れる
	gm.setShader( mCopyShader );
	gm.setShaderResource( kCopySourceSlot, mHistory[ next ] );
	gm.drawFullScreen();
	// 後から描く物（固定パイプライン）に残さない。
	gm.setShader( Graphics::Shader() );
	gm.setShaderResource( kCopySourceSlot, Graphics::RenderTarget() );
	gm.setConstantBuffer( kConstantSlot, Graphics::ConstantBuffer() );

	mCurrent = next;
	mValid = true;
	mWidth = width;
	mHeight = height;
	mPreviousProjectionView = projectionView;
}

void TemporalAntiAliasing::invalidate() {
	mValid = false;
}

const char* TemporalAntiAliasing::errorMessage() const {
	const char* message = mShader.errorMessage();
	if ( message[ 0 ] != '\0' ) {
		return message;
	}
	return mCopyShader.errorMessage();
}
