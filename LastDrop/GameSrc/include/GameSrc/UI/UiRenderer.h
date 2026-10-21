#ifndef INCLUDED_UI_RENDERER_H
#define INCLUDED_UI_RENDERER_H

#include "GameLib/Math/Vector2.h"
#include "GameLib/Scene/Font.h"
#include "GameLib/Scene/PrimitiveRenderer.h"
#include "GameLib/Scene/StringRenderer.h"
#include "GameSrc/UI/UiRect.h"

// 文字の大きさ。増やすときはここに1つ足し、UiRenderer.cpp の大きさの表にも1つ足す。
enum UiTextSize{
	UI_TEXT_NORMAL, // ボタン・HUD
	UI_TEXT_LARGE,  // 画面の見出し（タイトル名・CLEAR）

	UI_TEXT_SIZE_NUMBER,
};

// 文字を置く位置の基準。
enum UiAlign{
	UI_ALIGN_LEFT,   // position が文字の左上
	UI_ALIGN_CENTER, // position が文字の中心
};

// 画面の部品（IUiElement）が描く内容を積んでおき、draw でまとめて画面に描く（画素の座標。左上が原点）。
// 長方形は PrimitiveRenderer、文字は StringRenderer へ積み、長方形 → 文字の順に描く（文字が板に隠れないように）。
// 3D の後（PostProcess::endScene の後）に描けば、3D の上に重なる。
class UiRenderer{
public:
	UiRenderer();

	// 塗りつぶした長方形を積む。color: VertexColor::pack の並び。
	void addRect( const UiRect& rect, unsigned color );
	// 文字を積む（ASCII の英数字・記号のみ。それ以外の文字は、途中で絵を作れないので使えない）。color: VertexColor::pack の並び。
	void addText( const char* text, const GameLib::Math::Vector2& position, UiTextSize size, UiAlign align, unsigned color );
	// 積んだものを描いて空にする。1回の描画につき1回。
	void draw();

private:
	// 長方形の描き方（半透明に混ぜる・奥行きを見ない・裏も描く）を、これから積む長方形に設定する。
	// PrimitiveRenderer の描き方の設定は「設定した後に積んだもの」にだけ効く（積み終えた後で設定すると、空のまとまりに掛かるだけ）。
	// そのため積む前（作った直後と、描き終えた直後）に呼ぶ。2026-10-21 まで draw の中で積んだ後に設定していて、半透明が効いていなかった。
	void prepareRectState();
	// font の英数字・記号の絵を、最初の描画より前にすべて作っておく（理由は UiRenderer.cpp）。
	static void prepareGlyphs( GameLib::Scene::Font& font );
	// text を size の文字で書いたときの幅（画素）。中央に揃えるのに使う。
	float textWidth( const char* text, UiTextSize size );

	GameLib::Scene::PrimitiveRenderer mPrimitives;
	GameLib::Scene::Font mFonts[ UI_TEXT_SIZE_NUMBER ];
	GameLib::Scene::StringRenderer mStrings[ UI_TEXT_SIZE_NUMBER ]; // フォントごとに1つ（1つの StringRenderer はフォント1つ）
};

#endif
