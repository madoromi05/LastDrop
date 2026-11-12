#ifndef INCLUDED_UI_RENDERER_H
#define INCLUDED_UI_RENDERER_H

#include "GameLib/Math/Vector2.h"
#include "GameLib/Graphics/Texture.h"
#include "GameLib/Scene/Font.h"
#include "GameLib/Scene/PrimitiveRenderer.h"
#include "GameLib/Scene/StringRenderer.h"
#include "GameSrc/UI/UiRect.h"

// 文字の大きさ（と書体）。増やすときはここに1つ足し、UiRenderer.cpp の文字の表にも1つ足す。
enum UiTextSize{
	UI_TEXT_NORMAL, // ボタン・HUD
	UI_TEXT_LARGE,  // 大きい見出し（結果画面・ロード画面・カバン・HUD の知らせ）
	// RE:2 風のメニュー（明朝体）
	UI_TEXT_TITLE,   // ゲームの題名
	UI_TEXT_HEADING, // 設定・操作の一覧の見出し（左上）
	UI_TEXT_MENU,   // 左寄せのメニューの 1 行
	UI_TEXT_CAPTION, // 題名の下のひと言・画面の下の操作の案内

	UI_TEXT_SIZE_NUMBER,
};

// 文字を置く位置の基準。
enum UiAlign{
	UI_ALIGN_LEFT,   // position が文字の左上
	UI_ALIGN_CENTER, // position が文字の中心
	UI_ALIGN_MIDDLE_LEFT, // position が文字の左端の、縦の真ん中（行の中で左に寄せて縦は揃えるとき。設定画面の名前など）
};

// 画面の部品（IUiElement）が描く内容を積んでおき、draw でまとめて画面に描く（画素の座標。左上が原点）。
// 長方形と絵は PrimitiveRenderer、文字は StringRenderer へ積み、長方形・絵（積んだ順）→ 文字の順に描く（文字が板に隠れないように）。
// 3D の後（PostProcess::endScene の後）に描けば、3D の上に重なる。
class UiRenderer{
public:
	UiRenderer();

	// 塗りつぶした長方形を積む。color: VertexColor::pack の並び。
	void addRect( const UiRect& rect, unsigned color );
	// 太さ thickness の線分を a→b に積む（回転した長方形＝三角形2枚）。斜めにも引ける（心電図などの折れ線）。
	void addLine( const GameLib::Math::Vector2& a, const GameLib::Math::Vector2& b, float thickness, unsigned color );
	// 塗りつぶした丸（半径 radius）を center に積む。折れ線のつなぎ目に太さと同じ直径で置くと、角が丸くつながり隙間が消える。
	void addDisc( const GameLib::Math::Vector2& center, float radius, unsigned color );
	// 左端の色から右端の色へ横に移り変わる長方形を積む（メニューの選んでいる行の帯・背景の左の影）。
	// 色は VertexColor::pack の並び。a を 0 にした端は透明になる。
	void addGradientRect( const UiRect& rect, unsigned leftColor, unsigned rightColor );
	// 塗りつぶしたひし形（中心から上下左右の角まで radius）を center に積む（メニューの選んでいる印）。
	void addDiamond( const GameLib::Math::Vector2& center, float radius, unsigned color );
	// 絵を rect いっぱいに貼った長方形を積む（カバンの物の絵）。
	// texture: 読み終えた絵（isReady が true・isError が false）。画面は sRGB ではないので、絵は TEXTURE_COLOR_SPACE_LINEAR で読んだ物
	// （色の変換をせずに、画像の色のまま出す）。color: 絵に掛ける色（白でそのまま。a で薄くする）。
	// 絵が変わるたびに描く回数が 1 つ増えるので、長方形と絵を交互に積まず、絵はまとめて積む。
	void addImage( const UiRect& rect, const GameLib::Graphics::Texture& texture, unsigned color );
	// 文字を積む（UTF-8。日本語も書ける）。color: VertexColor::pack の並び。
	// letterSpacing: 字と字の間に足す幅（画素）。0 なら字の間を詰めずにまとめて積む（今までと同じ）。
	// 0 より大きいときは 1 字ずつ置く（StringRenderer は字の間を広げられないため）。最後の字の後には足さない（中央揃えがずれないように）。
	void addText( const char* text, const GameLib::Math::Vector2& position, UiTextSize size, UiAlign align, unsigned color, float letterSpacing = 0.f );
	// text を size の文字・字の間 letterSpacing で書いたときの幅（画素）。右揃え・枠の大きさを決めるのに使う。
	float textWidth( const char* text, UiTextSize size, float letterSpacing = 0.f );
	// size の文字の 1 行の高さ（画素）。枠の大きさを決めるのに使う。
	float lineHeight( UiTextSize size );
	// 積んだものを描いて空にする。1回の描画につき1回。
	void draw();

private:
	// 長方形の描き方（半透明に混ぜる・奥行きを見ない・裏も描く）を、これから積む長方形に設定する。
	// PrimitiveRenderer の描き方の設定は「設定した後に積んだもの」にだけ効く（積み終えた後で設定すると、空のまとまりに掛かるだけ）。
	void prepareRectState();
	// 実際の画面（バックバッファ）がゲームの座標の何倍の細かさかを求め、変わっていたらフォントをその細かさで作り直す。
	void fitFontsToScreen();
	// 1 字（code）の幅（画素）。フォントに無い字は 0。
	float charWidth( int code, UiTextSize size );

	GameLib::Scene::PrimitiveRenderer mPrimitives;
	GameLib::Scene::Font mFonts[ UI_TEXT_SIZE_NUMBER ];
	GameLib::Scene::StringRenderer mStrings[ UI_TEXT_SIZE_NUMBER ]; // フォントごとに1つ（1つの StringRenderer はフォント1つ）
	float mGlyphScale; // 今のフォントを作った細かさ（実際の画面の高さ / 座標の高さ）。0 はまだ作っていない
};

#endif
