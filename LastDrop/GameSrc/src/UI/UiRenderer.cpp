#include "GameSrc/UI/UiRenderer.h"

#include "GameLib/GameLib.h"
#include "GameLib/Base/Utf8.h"
#include "GameLib/Graphics/Enum.h"
#include "GameLib/Graphics/Manager.h"
#include "GameLib/Graphics/Texture.h"
#include <cmath>

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// OS のフォントから文字の絵を作る（GameLib::Scene::Font の OS 版）。どちらも日本語の字があり、Windows 10 以降に必ずある
	// 1 つの大きさの中では英数字も同じフォントで描く（混ぜると高さと線の太さが揃わない）
	const char* kGothic = "Yu Gothic UI"; // 小さくても読みやすい。HUD・設定画面
	const char* kMincho = "Yu Mincho";    // 静かで重い見た目（RE:2 風のメニュー）
	// 大きさごとの書体。UiTextSize の並びと同じ順
	// charHeight はゲームの座標の画素（目安）。実際のフォントは、これに画面の細かさ（fitFontsToScreen の倍率）を掛けた高さで作る
	struct TextStyle{
		const char* fontName;
		int charHeight;
		bool bold;
	};
	const TextStyle kTextStyles[ UI_TEXT_SIZE_NUMBER ] = {
		{ kGothic, 20, true },  // UI_TEXT_NORMAL
		{ kGothic, 48, true },  // UI_TEXT_LARGE
		{ kMincho, 48, false }, // UI_TEXT_TITLE   （游明朝はモックの字より幅が広いので、84px / 720 の 2/3 より少し小さく。背景の街灯にかからない幅）
		{ kMincho, 32, false }, // UI_TEXT_HEADING （見出しは画数の多い日本語なので、40px / 720 の 2/3 より大きく）
		{ kMincho, 18, false }, // UI_TEXT_MENU   （26px / 720）
		{ kMincho, 12, false }, // UI_TEXT_CAPTION （18px / 720）
	};
	const bool kItalic = false;
	const bool kProportional = true; // 文字ごとの幅（読みやすい）。中央揃えは textWidth で実際の幅を測るので困らない
	const bool kAntialias = true;

	// 積める量（長方形換算）。画面1枚ぶんのボタン・HUD に十分な数。足りなくなったら増やす（超えると ASSERT で止まる）
	// 文字は、確認用のゾンビの体力表示（1体に最大 12 行・約 100 文字。ゾンビは上限 10 体の想定）も入る量にしてある
	// 心電図は折れ線のつなぎ目ごとに丸い点（ディスク＝三角形 kDiscSegments 枚）を積むので、その分の余裕も見込む
	const int kRectCapacity = 512;
	const int kVerticesPerRect = 6; // 長方形は三角形2つ
	const int kBatchCapacity = 64;
	const int kDiscSegments = 8; // 丸い点を何枚の三角形で描くか（多いほど滑らかだが重い。UI の小さな点なので 8 で十分）
	const int kCharCapacity = 4096;
	const int kTextCapacity = 256;

	const float kHalf = 0.5f;
	// 絵の UV（絵の全体を貼る）。GameLib は TGA を上の行から並べて読むので、v = 0 が絵の上
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
	// まだフォントが無いのに大きさが分からなければ、座標と同じ細かさで作っておく（文字を積めないと止まるため）
	float scale = sizeKnown ? static_cast< float >( backBufferHeight ) / static_cast< float >( logicalHeight ) : kDefaultGlyphScale;
	if ( scale == mGlyphScale ) {
		return; // 画面の大きさが変わったときだけ作り直す（毎回作ると重い）
	}
	mGlyphScale = scale;
	for ( int i = 0; i < UI_TEXT_SIZE_NUMBER; ++i ) {
		const TextStyle& style = kTextStyles[ i ];
		int charHeight = static_cast< int >( static_cast< float >( style.charHeight ) * scale + kHalf );
		// 古いフォントは、もう積んだ文字（あれば）が持っているので、この draw が終わるまで消えない
		// 字の絵は初めて使うときに描き足される（文字の画像は後から書き足せる作り。GameLib の Texture::createWritable）
		mFonts[ i ] = Scene::Font::create( style.fontName, charHeight, style.bold, kItalic, kProportional, kAntialias );
		mStrings[ i ].setFont( mFonts[ i ] );
		mStrings[ i ].setGlyphScale( scale );
	}
}

void UiRenderer::addRect( const UiRect& rect, unsigned color ) {
	// 前に積んだ絵が付いたままにならないように外す（PrimitiveRenderer は、絵が変わったときだけ描くまとまりを分ける）
	mPrimitives.setTexture( Graphics::Texture() );
	mPrimitives.addRectangle( Vector2( rect.left, rect.top ), Vector2( rect.right(), rect.bottom() ), color );
}

void UiRenderer::addLine( const Vector2& a, const Vector2& b, float thickness, unsigned color ) {
	const Vector2 d( b.x - a.x, b.y - a.y );
	const float length = sqrtf( d.x * d.x + d.y * d.y );
	if (length < 1e-4f) {
		return; // 長さ 0 の線は積まない（0 割り防止）
	}
	// 線に垂直な向きへ、太さの半分だけ広げた 4 隅を作る（回転した長方形）
	const float half = thickness * kHalf;
	const Vector2 n( -d.y / length * half, d.x / length * half );
	const Vector2 a0( a.x - n.x, a.y - n.y );
	const Vector2 a1( a.x + n.x, a.y + n.y );
	const Vector2 b0( b.x - n.x, b.y - n.y );
	const Vector2 b1( b.x + n.x, b.y + n.y );
	mPrimitives.setTexture( Graphics::Texture() ); // 絵なし（長方形と同じ扱い）
	// スクリーン座標(画素)のまま積む。addRectangle と同じ座標系で描かれる（addTriangle は単位行列で描かれ、画素座標だと画面外に飛ぶ）
	mPrimitives.addScreenTriangle( a0, a1, b1, color, color, color );
	mPrimitives.addScreenTriangle( a0, b1, b0, color, color, color );
}

void UiRenderer::addDisc( const Vector2& center, float radius, unsigned color ) {
	if (radius <= 0.f) {
		return;
	}
	// 中心から放射状に三角形を並べた扇で、塗りつぶした丸を作る（折れ線のつなぎ目を丸くつないで隙間を消す）
	const float kTwoPi = 6.2831853f;
	mPrimitives.setTexture( Graphics::Texture() ); // 絵なし（長方形・線と同じ扱い）
	Vector2 prev( center.x + radius, center.y ); // 角度 0 の点
	for ( int i = 1; i <= kDiscSegments; ++i ) {
		const float angle = kTwoPi * static_cast< float >( i ) / static_cast< float >( kDiscSegments );
		const Vector2 cur( center.x + radius * cosf( angle ), center.y + radius * sinf( angle ) );
		mPrimitives.addScreenTriangle( center, prev, cur, color, color, color );
		prev = cur;
	}
}

void UiRenderer::addGradientRect( const UiRect& rect, unsigned leftColor, unsigned rightColor ) {
	const Vector2 topLeft( rect.left, rect.top );
	const Vector2 topRight( rect.right(), rect.top );
	const Vector2 bottomLeft( rect.left, rect.bottom() );
	const Vector2 bottomRight( rect.right(), rect.bottom() );
	mPrimitives.setTexture( Graphics::Texture() ); // 絵なし（長方形と同じ扱い）
	// 頂点ごとに色を置くと、間は GPU が混ぜてくれる（左の 2 隅が leftColor、右の 2 隅が rightColor）
	mPrimitives.addScreenTriangle( topLeft, topRight, bottomRight, leftColor, rightColor, rightColor );
	mPrimitives.addScreenTriangle( topLeft, bottomRight, bottomLeft, leftColor, rightColor, leftColor );
}

void UiRenderer::addDiamond( const Vector2& center, float radius, unsigned color ) {
	const Vector2 top( center.x, center.y - radius );
	const Vector2 right( center.x + radius, center.y );
	const Vector2 bottom( center.x, center.y + radius );
	const Vector2 left( center.x - radius, center.y );
	mPrimitives.setTexture( Graphics::Texture() ); // 絵なし（長方形と同じ扱い）
	mPrimitives.addScreenTriangle( top, right, bottom, color, color, color );
	mPrimitives.addScreenTriangle( top, bottom, left, color, color, color );
}

void UiRenderer::addImage( const UiRect& rect, const Graphics::Texture& texture, unsigned color ) {
	ASSERT( texture && "UiRenderer::addImage : texture is null." );
	mPrimitives.setTexture( texture );
	mPrimitives.addRectangle(
		Vector2( rect.left, rect.top ), Vector2( rect.right(), rect.bottom() ),
		Vector2( kImageUvTopLeft, kImageUvTopLeft ), Vector2( kImageUvBottomRight, kImageUvBottomRight ), color );
}

void UiRenderer::addText( const char* text, const Vector2& position, UiTextSize size, UiAlign align, unsigned color, float letterSpacing ) {
	ASSERT( text && size >= 0 && size < UI_TEXT_SIZE_NUMBER && "UiRenderer : bad text." );
	// 文字を積む前に合わせる（draw で合わせると、積んだ後に倍率だけが変わって文字の大きさがずれる）
	// ウィンドウの大きさは描画の始めに変わるので、その回の最初の addText で合う
	fitFontsToScreen();
	Vector2 topLeft = position;
	if ( align == UI_ALIGN_CENTER ) {
		topLeft.x -= textWidth( text, size, letterSpacing ) * kHalf;
	}
	if ( align == UI_ALIGN_CENTER || align == UI_ALIGN_MIDDLE_LEFT ) {
		topLeft.y -= lineHeight( size ) * kHalf;
	}
	if ( letterSpacing <= 0.f ) {
		mStrings[ size ].add( topLeft, text, color );
		return;
	}
	// 1 字ずつ切り出して積み、字の幅＋字の間だけ右へ進める（UTF-8 の 1 字は 1〜4 バイト）
	const int kMaxCharBytes = 4;
	char oneChar[ kMaxCharBytes + 1 ];
	int byteCount = 0;
	for ( const char* c = text; *c != '\0'; c += byteCount ) {
		int code = Utf8::decode( c, &byteCount );
		ASSERT( byteCount > 0 && byteCount <= kMaxCharBytes && "UiRenderer::addText : bad UTF-8." );
		for ( int i = 0; i < byteCount; ++i ) {
			oneChar[ i ] = c[ i ];
		}
		oneChar[ byteCount ] = '\0';
		mStrings[ size ].add( topLeft, oneChar, color );
		topLeft.x += charWidth( code, size ) + letterSpacing;
	}
}

float UiRenderer::lineHeight( UiTextSize size ) {
	fitFontsToScreen();
	return static_cast< float >( mFonts[ size ].charHeight() ) / mGlyphScale; // 絵の画素から座標の画素へ
}

void UiRenderer::prepareRectState() {
	mPrimitives.setBlendMode( Graphics::BLEND_LINEAR ); // 半透明の板も描けるように
	mPrimitives.enableDepthTest( false );               // 3D の上に重ねる
	mPrimitives.enableDepthWrite( false );
	mPrimitives.setCullMode( Graphics::CULL_NONE );
}

void UiRenderer::draw() {
	// 長方形と絵は線形で読む（物の絵は 256 画素の絵を小さく縮めて貼るので、点で読むとギザギザになる。色だけの長方形には効かない）
	// 文字は画素をにじませない（中央揃えは半画素ずれた所に置くことがある）。文字の絵は実際の画面とほぼ同じ細かさなので、点で読んでも粗くならない
	// 描き終えたら 3D 用の読み方（線形）へ戻す
	Graphics::Manager().setTextureFilter( Graphics::TEXTURE_FILTER_LINEAR );
	mPrimitives.draw();
	prepareRectState(); // 次の描画で積む長方形に効かせる（理由は prepareRectState の宣言）
	Graphics::Manager().setTextureFilter( Graphics::TEXTURE_FILTER_POINT );
	for ( int i = 0; i < UI_TEXT_SIZE_NUMBER; ++i ) {
		mStrings[ i ].draw();
	}
	Graphics::Manager().setTextureFilter( Graphics::TEXTURE_FILTER_LINEAR );
}

float UiRenderer::textWidth( const char* text, UiTextSize size, float letterSpacing ) {
	ASSERT( text && size >= 0 && size < UI_TEXT_SIZE_NUMBER && "UiRenderer::textWidth : bad text." );
	fitFontsToScreen();
	// 文字列は UTF-8 なので、StringRenderer と同じ読み方（Utf8::decode）で 1 字ずつ取り出す（1 バイトずつ数えると日本語の幅が合わない）
	float width = 0.f;
	int charCount = 0;
	int byteCount = 0;
	for ( const char* c = text; *c != '\0'; c += byteCount ) {
		int code = Utf8::decode( c, &byteCount );
		width += charWidth( code, size );
		++charCount;
	}
	if ( charCount > 1 ) {
		width += letterSpacing * static_cast< float >( charCount - 1 ); // 字の間は字の数より 1 つ少ない
	}
	return width;
}

float UiRenderer::charWidth( int code, UiTextSize size ) {
	// 文字の絵がフォントのテクスチャのどこにあるか（UV）から幅を求める
	Graphics::Texture texture;
	Vector2 uvTopLeft;
	Vector2 uvBottomRight;
	if ( !mFonts[ size ].getChar( &texture, &uvTopLeft, &uvBottomRight, code ) ) {
		return 0.f;
	}
	return ( uvBottomRight.x - uvTopLeft.x ) * static_cast< float >( texture.width() ) / mGlyphScale; // 絵の画素から座標の画素へ
}
