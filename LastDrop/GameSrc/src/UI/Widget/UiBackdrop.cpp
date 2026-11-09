#include "GameSrc/UI/Widget/UiBackdrop.h"
#include "GameSrc/UI/UiRenderer.h"

#include "GameLib/GameLib.h"
#include "Framework.h"

using namespace GameLib;

UiBackdrop::UiBackdrop( const char* filename, unsigned color, float alignX ) :
	// 画面は sRGB ではなく、UI は ToneMap の後にそのまま描くので、色の変換をせずに読む（UiRenderer::addImage の決まり）。
	mTexture( Graphics::Texture::create( filename, Graphics::TEXTURE_COLOR_SPACE_LINEAR ) ),
	mColor( color ),
	mAlignX( alignX ) {
	ASSERT( filename && "UiBackdrop : filename is null." );
	ASSERT( 0.f <= alignX && alignX <= 1.f && "UiBackdrop : alignX must be 0..1." );
}

void UiBackdrop::draw( UiRenderer& renderer ) const {
	if ( !mTexture.isReady() || mTexture.isError() ) {
		return; // 読み込み中（読めなかったら、背景なしの黒のまま）
	}
	ASSERT( mTexture.originalWidth() == mTexture.width() && mTexture.originalHeight() == mTexture.height()
		&& "UiBackdrop : texture size must be a power of two." );
	const float kHalf = 0.5f;
	const float screenWidth = static_cast< float >( Framework::instance().width() );
	const float screenHeight = static_cast< float >( Framework::instance().height() );
	const float imageAspect = static_cast< float >( mTexture.width() ) / static_cast< float >( mTexture.height() );
	// 縦を画面に合わせ、それで横が足りなければ横を合わせる（画面を必ず覆う）。
	// 横のはみ出しは mAlignX の割合で左右に分け、縦のはみ出しは上下に半分ずつ分ける。
	UiRect rect;
	rect.height = screenHeight;
	rect.width = screenHeight * imageAspect;
	if ( rect.width < screenWidth ) {
		rect.width = screenWidth;
		rect.height = screenWidth / imageAspect;
	}
	rect.left = ( screenWidth - rect.width ) * mAlignX;
	rect.top = ( screenHeight - rect.height ) * kHalf;
	renderer.addImage( rect, mTexture, mColor );
}
