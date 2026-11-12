#include "GameSrc/UI/Widget/UiKeyBindingGroup.h"
#include "GameSrc/UI/Widget/KeyCap.h"
#include "GameSrc/UI/UiRenderer.h"
#include "GameSrc/UI/MenuPalette.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

namespace {
	const float kNameToRows = 21.f;          // まとまりの名前の上端から最初の行の上端まで（文字の高さ 12 ＋ 間 9）
	const float kNameLetterSpacing = 3.4f;   // まとまりの名前の字の間（文字の高さの 0.28 倍くらい）
	const float kRowHeight = 33.f;
	const float kLineThickness = 1.f;        // 行の上の細い線の太さ
	const float kActionLetterSpacing = 1.2f; // 操作の名前の字の間（文字の高さの 0.1 倍くらい）
	const float kKeyGap = 4.f;               // キーの枠と枠の間
	const UiTextSize kTextSize = UI_TEXT_CAPTION;
}

UiKeyBindingGroup::UiKeyBindingGroup( const UiColumn& column, float top, const char* name, const std::vector< Row >& rows ) :
	mColumn( column ),
	mTop( top ),
	mName( name ),
	mRows( rows ) {
	ASSERT( name && !mRows.empty() && "UiKeyBindingGroup : name is null or rows is empty." );
}

void UiKeyBindingGroup::draw( UiRenderer& renderer ) const {
	UiRect rect;
	rect.left = UiLayout::columnLeft( mColumn );
	rect.width = UiLayout::columnWidth( mColumn );
	rect.top = mTop + kNameToRows;
	rect.height = kRowHeight;
	renderer.addText( mName.c_str(), Vector2( rect.left, mTop ), kTextSize, UI_ALIGN_LEFT, MenuPalette::TEXT_DIM, kNameLetterSpacing );
	for (const Row& row : mRows) {
		drawRow( renderer, row, rect );
		rect.top += kRowHeight;
	}
}

void UiKeyBindingGroup::drawRow( UiRenderer& renderer, const Row& row, const UiRect& rect ) const {
	UiRect line = rect;
	line.height = kLineThickness;
	renderer.addRect( line, MenuPalette::ROW_LINE );
	const float middleY = rect.center().y;
	renderer.addText( row.action.c_str(), Vector2( rect.left, middleY ), kTextSize, UI_ALIGN_MIDDLE_LEFT, MenuPalette::TEXT, kActionLetterSpacing );

	// キーの枠は行の右端に寄せる。先に全部の幅を測ってから、左の枠から順に描く
	float keysWidth = 0.f;
	for (const std::string& key : row.keys) {
		if (keysWidth > 0.f) {
			keysWidth += kKeyGap;
		}
		keysWidth += KeyCap::width( renderer, key.c_str() );
	}
	float x = rect.right() - keysWidth;
	for (const std::string& key : row.keys) {
		x = KeyCap::draw( renderer, key.c_str(), x, middleY ) + kKeyGap;
	}
}