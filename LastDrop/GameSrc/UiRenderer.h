#ifndef INCLUDED_UI_RENDERER_H
#define INCLUDED_UI_RENDERER_H

#include "GameLib/Math/Vector2.h"
#include "GameLib/Scene/Font.h"
#include "GameLib/Scene/PrimitiveRenderer.h"
#include "GameLib/Scene/StringRenderer.h"
#include "UiRect.h"

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
	// 文字を積む（英数字のみ。日本語のフォントはまだ無い）。color: VertexColor::pack の並び。
	void addText( const char* text, const GameLib::Math::Vector2& position, UiTextSize size, UiAlign align, unsigned color );
	// 積んだものを描いて空にする。1回の描画につき1回。
	void draw();

private:
	// text を size の文字で書いたときの幅（画素）。中央に揃えるのに使う。
	float textWidth( const char* text, UiTextSize size );

	GameLib::Scene::PrimitiveRenderer mPrimitives;
	GameLib::Scene::Font mFonts[ UI_TEXT_SIZE_NUMBER ];
	GameLib::Scene::StringRenderer mStrings[ UI_TEXT_SIZE_NUMBER ]; // フォントごとに1つ（1つの StringRenderer はフォント1つ）
};

#endif
