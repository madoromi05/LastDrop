#ifndef INCLUDED_GRID_PALETTE_H
#define INCLUDED_GRID_PALETTE_H

#include "GameSrc/Render/VertexColor.h"

// マス目の画面（町のカバン・拠点の倉庫・漁る場所）の色
// 3 つの画面とマス目の部品（ItemGridView）が同じ色を使うので 1 か所に置く。メニューの色は MenuPalette
namespace GridPalette{
	const unsigned TEXT = VertexColor::pack( 0.92f, 0.92f, 0.92f );						// 物の名前・数・項目の文字
	const unsigned HINT = VertexColor::pack( 0.65f, 0.65f, 0.65f );						// 操作の案内
	const unsigned HEADING = VertexColor::pack( 0.85f, 0.65f, 0.15f );					// 見出し・手に持っている銃の印（黄）
	// 項目の板（カバンのメニュー・拠点の出撃のボタン）。選んでいる板は見出しと同じ黄にして、文字を暗くする
	const unsigned BUTTON = VertexColor::pack( 0.15f, 0.15f, 0.15f, 0.9f );
	const unsigned BUTTON_SELECTED = VertexColor::pack( 0.85f, 0.65f, 0.15f, 0.95f );
	const unsigned BUTTON_SELECTED_TEXT = VertexColor::pack( 0.05f, 0.05f, 0.05f );
	// マスの枠
	const unsigned FRAME_SELECT = VertexColor::pack( 0.95f, 0.85f, 0.3f );				// 選んでいるマス
	const unsigned FRAME_PLACE = VertexColor::pack( 0.35f, 0.9f, 0.35f );				// 置ける所
	const unsigned FRAME_BLOCKED = VertexColor::pack( 0.9f, 0.2f, 0.15f );				// 入らない所
	// 町で開く画面（カバン・漁る場所）が、後ろの町を透かして見せる濃さ。暗くする板とマスの背景に使い、物の絵と文字は濃いまま
	const float BACKDROP_ALPHA = 0.5f;
	const unsigned BACKDROP = VertexColor::pack( 0.f, 0.f, 0.f, BACKDROP_ALPHA );
}

#endif
