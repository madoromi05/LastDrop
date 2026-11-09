#ifndef INCLUDED_MENU_PALETTE_H
#define INCLUDED_MENU_PALETTE_H

#include "GameSrc/Render/VertexColor.h"

// RE:2 風のメニュー（タイトル画面から使う。TitleScreen_Plan.txt）の色。
// 部品（UiMenuItem・UiKeyHintRow）と並べ方（MenuLayout）が同じ色を使うので 1 か所に置く。
// 値は claude.ai のモック（https://claude.ai/artifact/RHr5y2N5hLMGD8SW4cx5jD）の色をそのまま 0..1 にした物。
// 画面は sRGB ではなく、UI は ToneMap の後にそのまま描くので、モックの 16 進の色と同じ数で同じ色に見える。
// 赤（ACCENT）は印と線だけに使う（文字には使わない。黒地では細い文字が読みにくい）。
namespace MenuPalette{
	const unsigned TEXT = VertexColor::pack( 0.910f, 0.894f, 0.863f );          // #e8e4dc 題名・強い文字
	const unsigned TEXT_SELECTED = VertexColor::pack( 0.953f, 0.937f, 0.906f ); // #f3efe7 選んでいる行の文字
	const unsigned TEXT_SUB = VertexColor::pack( 0.659f, 0.639f, 0.604f );      // #a8a39a ひと言・説明
	const unsigned TEXT_DIM = VertexColor::pack( 0.545f, 0.525f, 0.494f );      // #8b867e 選んでいない行・案内
	const unsigned KEY_TEXT = VertexColor::pack( 0.788f, 0.769f, 0.729f );      // #c9c4ba 案内のキーの名前
	const unsigned KEY_FRAME = VertexColor::pack( 0.290f, 0.278f, 0.255f );     // #4a4741 案内のキーの枠
	const unsigned ACCENT = VertexColor::pack( 0.639f, 0.133f, 0.122f );        // #a3221f 赤い線・選んでいる印
	// 選んでいる行の上下の細い線と、左から右へ薄れる帯（右端は透明）。
	const unsigned SELECTED_LINE = VertexColor::pack( 0.910f, 0.894f, 0.863f, 0.28f );
	const unsigned SELECTED_BAND = VertexColor::pack( 0.910f, 0.894f, 0.863f, 0.12f );
	const unsigned SELECTED_BAND_END = VertexColor::pack( 0.910f, 0.894f, 0.863f, 0.f );
	// 背景の絵の左を暗くする影（UiShade）。#050505。
	const unsigned SHADE = VertexColor::pack( 0.020f, 0.020f, 0.020f, 0.92f );
}

#endif
