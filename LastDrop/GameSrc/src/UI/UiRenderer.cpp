#include "GameSrc/UI/UiRenderer.h"

#include "GameLib/GameLib.h"
#include "GameLib/Graphics/Enum.h"
#include "GameLib/Graphics/Manager.h"
#include "GameLib/Graphics/Texture.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// OS のフォントから文字の絵を作る（GameLib::Scene::Font の OS 版）。英数字が読みやすい、どの Windows にもあるもの。
	const char* kFontName = "Arial";
	// 大きさごとの文字の高さ（画素。目安）。UiTextSize の並びと同じ順。
	const int kCharHeights[ UI_TEXT_SIZE_NUMBER ] = {
		20, // UI_TEXT_NORMAL
		48, // UI_TEXT_LARGE
	};
	const bool kBold = true;
	const bool kItalic = false;
	const bool kProportional = true; // 文字ごとの幅（読みやすい）。中央揃えは textWidth で実際の幅を測るので困らない
	const bool kAntialias = true;

	// 積める量。画面1枚ぶんのボタン・HUD に十分な数。足りなくなったら増やす（超えると ASSERT で止まる）。
	const int kRectCapacity = 256;
	const int kVerticesPerRect = 6; // 長方形は三角形2つ
	const int kBatchCapacity = 64;
	const int kCharCapacity = 1024;
	const int kTextCapacity = 64;

	const float kHalf = 0.5f;
	// 最初に文字の絵を作っておく範囲（ASCII の表示できる文字すべて。空白〜チルダ）。
	const int kFirstPrintableChar = 0x20;
	const int kLastPrintableChar = 0x7e;
}

UiRenderer::UiRenderer() :
	mPrimitives( Scene::PrimitiveRenderer::create( kRectCapacity * kVerticesPerRect, kBatchCapacity ) ) {
	for ( int i = 0; i < UI_TEXT_SIZE_NUMBER; ++i ) {
		mFonts[ i ] = Scene::Font::create( kFontName, kCharHeights[ i ], kBold, kItalic, kProportional, kAntialias );
		mStrings[ i ] = Scene::StringRenderer::create( kCharCapacity, kTextCapacity );
		mStrings[ i ].setFont( mFonts[ i ] );
		prepareGlyphs( mFonts[ i ] );
	}
}

void UiRenderer::addRect( const UiRect& rect, unsigned color ) {
	mPrimitives.addRectangle( Vector2( rect.left, rect.top ), Vector2( rect.right(), rect.bottom() ), color );
}

void UiRenderer::addText( const char* text, const Vector2& position, UiTextSize size, UiAlign align, unsigned color ) {
	ASSERT( text && size >= 0 && size < UI_TEXT_SIZE_NUMBER && "UiRenderer : bad text." );
	Vector2 topLeft = position;
	if ( align == UI_ALIGN_CENTER ) {
		topLeft.x -= textWidth( text, size ) * kHalf;
		topLeft.y -= static_cast< float >( mFonts[ size ].charHeight() ) * kHalf;
	}
	mStrings[ size ].add( topLeft, text, color );
}

void UiRenderer::draw() {
	mPrimitives.setBlendMode( Graphics::BLEND_LINEAR ); // 半透明の板も描けるように
	mPrimitives.enableDepthTest( false );               // 3D の上に重ねる
	mPrimitives.enableDepthWrite( false );
	mPrimitives.setCullMode( Graphics::CULL_NONE );
	// 文字は画素をにじませない（中央揃えは半画素ずれた所に置くことがある）。描き終えたら 3D 用の読み方へ戻す。
	Graphics::Manager().setTextureFilter( Graphics::TEXTURE_FILTER_POINT );
	mPrimitives.draw();
	for ( int i = 0; i < UI_TEXT_SIZE_NUMBER; ++i ) {
		mStrings[ i ].draw();
	}
	Graphics::Manager().setTextureFilter( Graphics::TEXTURE_FILTER_LINEAR );
}

void UiRenderer::prepareGlyphs( Scene::Font& font ) {
	// OS のフォントは、文字を初めて使うときにその文字の絵をテクスチャへ描き足す。ところが GameLib の DX11 版のテクスチャは、
	// 一度 GPU へ送ると CPU 側の画素を捨てるので、その後に新しい文字を描き足そうとすると止まる（TextureImpl::lock の ASSERT）。
	// HUD の案内のように途中から新しい文字が出る画面があるので、最初の描画より前に、使う文字をすべて作っておく。
	Graphics::Texture texture;
	Vector2 uvTopLeft;
	Vector2 uvBottomRight;
	for ( int code = kFirstPrintableChar; code <= kLastPrintableChar; ++code ) {
		font.getChar( &texture, &uvTopLeft, &uvBottomRight, code );
	}
}

float UiRenderer::textWidth( const char* text, UiTextSize size ) {
	// 文字の絵がフォントのテクスチャのどこにあるか（UV）から、1文字ずつ幅を足す。
	float width = 0.f;
	for ( const char* c = text; *c != '\0'; ++c ) {
		Graphics::Texture texture;
		Vector2 uvTopLeft;
		Vector2 uvBottomRight;
		if ( !mFonts[ size ].getChar( &texture, &uvTopLeft, &uvBottomRight, static_cast< unsigned char >( *c ) ) ) {
			continue;
		}
		width += ( uvBottomRight.x - uvTopLeft.x ) * static_cast< float >( texture.width() );
	}
	return width;
}
