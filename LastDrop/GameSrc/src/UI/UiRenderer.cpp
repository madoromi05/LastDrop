#include "GameSrc/UI/UiRenderer.h"

#include "GameLib/GameLib.h"
#include "GameLib/Base/Utf8.h"
#include "GameLib/Graphics/Enum.h"
#include "GameLib/Graphics/Manager.h"
#include "GameLib/Graphics/Texture.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// OS のフォントから文字の絵を作る（GameLib::Scene::Font の OS 版）。日本語の字があり、Windows 10 以降に必ずあるゴシック体。
	// 小さくても読みやすく、英数字もきれい（英数字も同じフォントで描く。混ぜると高さと線の太さが揃わない）。JapaneseText_Plan.txt Q4。
	const char* kFontName = "Yu Gothic UI";
	// 大きさごとの文字の高さ（ゲームの座標の画素。目安）。UiTextSize の並びと同じ順。
	// 実際のフォントは、これに画面の細かさ（fitFontsToScreen の倍率）を掛けた高さで作る。
	const int kCharHeights[ UI_TEXT_SIZE_NUMBER ] = {
		20, // UI_TEXT_NORMAL
		48, // UI_TEXT_LARGE
	};
	const bool kBold = true;
	const bool kItalic = false;
	const bool kProportional = true; // 文字ごとの幅（読みやすい）。中央揃えは textWidth で実際の幅を測るので困らない
	const bool kAntialias = true;

	// 積める量。画面1枚ぶんのボタン・HUD に十分な数。足りなくなったら増やす（超えると ASSERT で止まる）。
	// 文字は、確認用のゾンビの体力表示（1体に最大 12 行・約 100 文字。ゾンビは上限 10 体の想定）も入る量にしてある。
	const int kRectCapacity = 256;
	const int kVerticesPerRect = 6; // 長方形は三角形2つ
	const int kBatchCapacity = 64;
	const int kCharCapacity = 4096;
	const int kTextCapacity = 256;

	const float kHalf = 0.5f;
	// 絵の UV（絵の全体を貼る）。GameLib は TGA を上の行から並べて読むので、v = 0 が絵の上。
	const float kImageUvTopLeft = 0.f;
	const float kImageUvBottomRight = 1.f;
	const float kDefaultGlyphScale = 1.f; // 画面の大きさが分からないときの、文字の絵の細かさ（座標と同じ）
}

UiRenderer::UiRenderer() :
	mPrimitives( Scene::PrimitiveRenderer::create( kRectCapacity * kVerticesPerRect, kBatchCapacity ) ),
	mGlyphScale( 0.f ) {
	for ( int i = 0; i < UI_TEXT_SIZE_NUMBER; ++i ) {
		mStrings[ i ] = Scene::StringRenderer::create( kCharCapacity, kTextCapacity );
	}
	fitFontsToScreen();
	prepareRectState(); // 最初の描画で積む長方形に効かせる
}

void UiRenderer::fitFontsToScreen() {
	Graphics::Manager graphics;
	int logicalHeight = graphics.height();
	int backBufferHeight = graphics.backBufferHeight();
	bool sizeKnown = ( logicalHeight > 0 && backBufferHeight > 0 );
	if ( !sizeKnown && mGlyphScale > 0.f ) {
		return; // 大きさが分からないうち（最小化中など）は今のフォントのまま
	}
	// まだフォントが無いのに大きさが分からなければ、座標と同じ細かさで作っておく（文字を積めないと止まるため）。
	float scale = sizeKnown ? static_cast< float >( backBufferHeight ) / static_cast< float >( logicalHeight ) : kDefaultGlyphScale;
	if ( scale == mGlyphScale ) {
		return; // 画面の大きさが変わったときだけ作り直す（毎回作ると重い）
	}
	mGlyphScale = scale;
	for ( int i = 0; i < UI_TEXT_SIZE_NUMBER; ++i ) {
		int charHeight = static_cast< int >( static_cast< float >( kCharHeights[ i ] ) * scale + kHalf );
		// 古いフォントは、もう積んだ文字（あれば）が持っているので、この draw が終わるまで消えない。
		// 字の絵は初めて使うときに描き足される（文字の画像は後から書き足せる作り。GameLib の Texture::createWritable）。
		mFonts[ i ] = Scene::Font::create( kFontName, charHeight, kBold, kItalic, kProportional, kAntialias );
		mStrings[ i ].setFont( mFonts[ i ] );
		mStrings[ i ].setGlyphScale( scale );
	}
}

void UiRenderer::addRect( const UiRect& rect, unsigned color ) {
	// 前に積んだ絵が付いたままにならないように外す（PrimitiveRenderer は、絵が変わったときだけ描くまとまりを分ける）。
	mPrimitives.setTexture( Graphics::Texture() );
	mPrimitives.addRectangle( Vector2( rect.left, rect.top ), Vector2( rect.right(), rect.bottom() ), color );
}

void UiRenderer::addImage( const UiRect& rect, const Graphics::Texture& texture, unsigned color ) {
	ASSERT( texture && "UiRenderer::addImage : texture is null." );
	mPrimitives.setTexture( texture );
	mPrimitives.addRectangle(
		Vector2( rect.left, rect.top ), Vector2( rect.right(), rect.bottom() ),
		Vector2( kImageUvTopLeft, kImageUvTopLeft ), Vector2( kImageUvBottomRight, kImageUvBottomRight ), color );
}

void UiRenderer::addText( const char* text, const Vector2& position, UiTextSize size, UiAlign align, unsigned color ) {
	ASSERT( text && size >= 0 && size < UI_TEXT_SIZE_NUMBER && "UiRenderer : bad text." );
	// 文字を積む前に合わせる（draw で合わせると、積んだ後に倍率だけが変わって文字の大きさがずれる）。
	// ウィンドウの大きさは描画の始めに変わるので、その回の最初の addText で合う。
	fitFontsToScreen();
	Vector2 topLeft = position;
	if ( align == UI_ALIGN_CENTER ) {
		topLeft.x -= textWidth( text, size ) * kHalf;
	}
	if ( align == UI_ALIGN_CENTER || align == UI_ALIGN_MIDDLE_LEFT ) {
		topLeft.y -= static_cast< float >( mFonts[ size ].charHeight() ) / mGlyphScale * kHalf;
	}
	mStrings[ size ].add( topLeft, text, color );
}

void UiRenderer::prepareRectState() {
	mPrimitives.setBlendMode( Graphics::BLEND_LINEAR ); // 半透明の板も描けるように
	mPrimitives.enableDepthTest( false );               // 3D の上に重ねる
	mPrimitives.enableDepthWrite( false );
	mPrimitives.setCullMode( Graphics::CULL_NONE );
}

void UiRenderer::draw() {
	// 長方形と絵は線形で読む（物の絵は 256 画素の絵を小さく縮めて貼るので、点で読むとギザギザになる。色だけの長方形には効かない）。
	// 文字は画素をにじませない（中央揃えは半画素ずれた所に置くことがある）。文字の絵は実際の画面とほぼ同じ細かさなので、点で読んでも粗くならない。
	// 描き終えたら 3D 用の読み方（線形）へ戻す。
	Graphics::Manager().setTextureFilter( Graphics::TEXTURE_FILTER_LINEAR );
	mPrimitives.draw();
	prepareRectState(); // 次の描画で積む長方形に効かせる（理由は prepareRectState の宣言）
	Graphics::Manager().setTextureFilter( Graphics::TEXTURE_FILTER_POINT );
	for ( int i = 0; i < UI_TEXT_SIZE_NUMBER; ++i ) {
		mStrings[ i ].draw();
	}
	Graphics::Manager().setTextureFilter( Graphics::TEXTURE_FILTER_LINEAR );
}

float UiRenderer::textWidth( const char* text, UiTextSize size ) {
	// 文字の絵がフォントのテクスチャのどこにあるか（UV）から、1文字ずつ幅を足す。
	// 文字列は UTF-8 なので、StringRenderer と同じ読み方（Utf8::decode）で 1 字ずつ取り出す（1 バイトずつ数えると日本語の幅が合わない）。
	float width = 0.f;
	int byteCount = 0;
	for ( const char* c = text; *c != '\0'; c += byteCount ) {
		int code = Utf8::decode( c, &byteCount );
		Graphics::Texture texture;
		Vector2 uvTopLeft;
		Vector2 uvBottomRight;
		if ( !mFonts[ size ].getChar( &texture, &uvTopLeft, &uvBottomRight, code ) ) {
			continue;
		}
		width += ( uvBottomRight.x - uvTopLeft.x ) * static_cast< float >( texture.width() );
	}
	return width / mGlyphScale; // 絵の画素から座標の画素へ
}
