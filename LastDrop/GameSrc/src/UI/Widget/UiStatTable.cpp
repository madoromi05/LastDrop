#include "GameSrc/UI/Widget/UiStatTable.h"
#include "GameSrc/UI/UiRenderer.h"
#include "GameSrc/UI/MenuPalette.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

namespace{
	// 画素（座標の高さ 480 で。モックの 720 の 2/3）
	const float kLineThickness = 1.f;		// 行の上下の細い線の太さ
	const float kNameLetterSpacing = 1.7f;	// 名前の字の間（文字の高さの 0.14 倍くらい）
	const float kValueLetterSpacing = 1.1f;	// 値の字の間（文字の高さの 0.06 倍くらい）
	const UiTextSize kNameSize = UI_TEXT_CAPTION;
	const UiTextSize kValueSize = UI_TEXT_MENU;	// 値は名前より大きく（結果で一番見せたい所）
}

UiStatTable::UiStatTable( UiAnchor anchor, const UiRect& firstRow, const std::vector< Row >& rows ) :
	mAnchor( anchor ),
	mFirstRow( firstRow ),
	mRows( rows ) {
	ASSERT( !mRows.empty() && "UiStatTable : rows is empty." );
}

void UiStatTable::draw( UiRenderer& renderer ) const {
	UiRect rect = mFirstRow;
	rect.left += UiLayout::anchorX( mAnchor );
	for ( const Row& row : mRows ) {
		drawRow( renderer, row, rect );
		rect.top += rect.height;
	}
	UiRect bottomLine = rect;	// ループを抜けた rect は最後の行の 1 つ下にあるので、その上端が最後の行の下端
	bottomLine.height = kLineThickness;
	renderer.addRect( bottomLine, MenuPalette::ROW_LINE );
}

void UiStatTable::drawRow( UiRenderer& renderer, const Row& row, const UiRect& rect ) const {
	UiRect line = rect;
	line.height = kLineThickness;
	renderer.addRect( line, MenuPalette::ROW_LINE );
	const float middleY = rect.center().y;
	renderer.addText( row.name.c_str(), Vector2( rect.left, middleY ), kNameSize, UI_ALIGN_MIDDLE_LEFT, MenuPalette::TEXT_SUB, kNameLetterSpacing );
	const float valueWidth = renderer.textWidth( row.value.c_str(), kValueSize, kValueLetterSpacing );	// UiAlign に右揃えは無いので、幅を測って左端を求める
	renderer.addText( row.value.c_str(), Vector2( rect.right() - valueWidth, middleY ), kValueSize, UI_ALIGN_MIDDLE_LEFT, MenuPalette::TEXT, kValueLetterSpacing );
}
