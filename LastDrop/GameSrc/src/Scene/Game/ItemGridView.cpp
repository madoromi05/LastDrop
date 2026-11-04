#include "GameSrc/Scene/Game/ItemGridView.h"
#include "GameSrc/Scene/Game/ItemIcons.h"
#include "GameSrc/Interaction/ItemGrid.h"
#include "GameSrc/UI/UiRenderer.h"
#include "GameSrc/Render/VertexColor.h"
#include "GameSrc/Weapon/WeaponCatalog.h"

#include "GameLib/GameLib.h"
#include <algorithm>
#include <cstdio>

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const float kHalf = 0.5f;
	const int kNone = -1;

	// マスどうしの間（画素）。
	const float kCellGap = 4.f;
	const float kIconInset = 3.f;      // マスの縁から絵まで（選んだ枠の太さより広く、枠が絵に掛からないように）
	const float kBothSides = 2.f;      // 左右（上下）の両側の余白の分
	// 数（2 桁）を置く右下の板。板の真ん中に数を置く。
	const float kAmountPlateWidth = 26.f;
	const float kAmountPlateHeight = 20.f; // 文字（UI_TEXT_NORMAL の 20）と同じ高さ
	const float kAmountInset = 3.f;    // マスの右下の角から板まで
	const float kMarkPlateSize = 20.f; // 印（文字 1 つ）の板。文字と同じ高さの正方形

	// 銃の弾倉の数（「12/12」）を置く左下の板。5 文字が入る幅（小さいマスでは、マスの幅に収める）。
	const float kRoundsPlateWidth = 50.f;
	const char* kRoundsFormat = "%d/%d";
	const int kTextSize = 16;

	const float kEmptyCellGray = 0.08f;
	const float kDefaultEmptyAlpha = 0.85f;
	// 物のあるマスの背景（どの物も同じ。物の見分けは絵で付ける）。空のマスより少し明るくして、物のある所が分かるように。
	const float kItemCellGray = 0.2f;
	const float kDefaultItemAlpha = 0.9f;
	const float kOpaque = 1.f;        // 絵はそのままの色・濃さで貼る（白を掛ける）
	const float kLiftedAlpha = 0.35f; // 持ち上げている物の元の場所（薄くして、どれを動かしているか分かるように）
	const float kGhostAlpha = 0.6f;   // 拾って持っている物を、置こうとしている所に重ねる濃さ
	const unsigned kAmountPlateColor = VertexColor::pack( 0.f, 0.f, 0.f, 0.65f );
	const unsigned kTextColor = VertexColor::pack( 0.92f, 0.92f, 0.92f );
	const unsigned kEmptyRoundsColor = VertexColor::pack( 0.95f, 0.25f, 0.2f ); // 弾倉が空
	const unsigned kMarkColor = VertexColor::pack( 0.85f, 0.65f, 0.15f ); // ボタンを選んだ色と同じ黄

	unsigned itemColor( ItemId id, float alpha ) {
		const ItemSpec& spec = itemSpec( id );
		return VertexColor::pack( spec.red, spec.green, spec.blue, alpha );
	}

	// area の内側 kIconInset に、絵の縦横の比を保って収まる一番大きい長方形（真ん中に置く）。
	// 2 マスの物の範囲は絵（2 : 1）とわずかに比が違うので、引き伸ばさずに収める。
	UiRect fitIcon( const UiRect& area, const Graphics::Texture& texture ) {
		float maxWidth = area.width - kIconInset * kBothSides;
		float maxHeight = area.height - kIconInset * kBothSides;
		float aspect = static_cast< float >( texture.originalWidth() ) / static_cast< float >( texture.originalHeight() );
		UiRect rect;
		rect.width = std::min( maxWidth, maxHeight * aspect );
		rect.height = rect.width / aspect;
		rect.left = area.left + ( area.width - rect.width ) * kHalf;
		rect.top = area.top + ( area.height - rect.height ) * kHalf;
		return rect;
	}
}

ItemGridView::ItemGridView( UiRenderer* renderer, ItemIcons* icons ) :
	mRenderer( renderer ),
	mIcons( icons ),
	mOrigin( 0.f, 0.f ),
	mCellSize( 0.f ),
	mEmptyAlpha( kDefaultEmptyAlpha ),
	mItemAlpha( kDefaultItemAlpha ) {
	ASSERT( mRenderer && mIcons && "ItemGridView : an argument is null." );
}

void ItemGridView::setLayout( const Vector2& origin, float cellSize ) {
	mOrigin = origin;
	mCellSize = cellSize;
}

void ItemGridView::setCellAlpha( float alpha ) {
	mEmptyAlpha = alpha;
	mItemAlpha = alpha;
}

float ItemGridView::width( const ItemGrid& grid, float cellSize ) {
	return grid.columns() * cellSize + ( grid.columns() - 1 ) * kCellGap;
}

float ItemGridView::height( const ItemGrid& grid, float cellSize ) {
	return grid.rows() * cellSize + ( grid.rows() - 1 ) * kCellGap;
}

UiRect ItemGridView::cellRect( const ItemGrid& grid, int cell ) const {
	return spanRect( grid, cell, 1 );
}

UiRect ItemGridView::spanRect( const ItemGrid& grid, int owner, int size ) const {
	UiRect rect;
	rect.left = mOrigin.x + ( owner % grid.columns() ) * ( mCellSize + kCellGap );
	rect.top = mOrigin.y + ( owner / grid.columns() ) * ( mCellSize + kCellGap );
	rect.width = size * mCellSize + ( size - 1 ) * kCellGap;
	rect.height = mCellSize;
	return rect;
}

int ItemGridView::cellAt( const ItemGrid& grid, const Vector2& point ) const {
	for ( int i = 0; i < grid.cellCount(); ++i ) {
		if ( cellRect( grid, i ).contains( point ) ) {
			return i;
		}
	}
	return kNone;
}

void ItemGridView::drawCells( const ItemGrid& grid, int lifted ) {
	// ① 背景（長方形だけ）。物のマスは同じ灰。絵が読めなかった物だけ、物の色で塗る（名前は選ぶと下の欄に出る）。
	unsigned emptyColor = VertexColor::pack( kEmptyCellGray, kEmptyCellGray, kEmptyCellGray, mEmptyAlpha );
	for ( int i = 0; i < grid.cellCount(); ++i ) {
		if ( grid.ownerCell( i ) == kNone ) {
			mRenderer->addRect( cellRect( grid, i ), emptyColor );
		}
	}
	for ( int i = 0; i < grid.cellCount(); ++i ) {
		if ( grid.ownerCell( i ) != i ) {
			continue; // 空のマスか、2 マスの物の右のマス
		}
		ItemId item = grid.stackAt( i ).item;
		float alpha = ( i == lifted ) ? kLiftedAlpha : mItemAlpha;
		unsigned color = mIcons->icon( item ) ? VertexColor::pack( kItemCellGray, kItemCellGray, kItemCellGray, alpha ) : itemColor( item, alpha );
		mRenderer->addRect( spanRect( grid, i, itemSpec( item ).cellSize ), color );
	}
	// ② 物の絵（まとめて積む）。持ち上げている物の元の場所は、背景と同じく薄くする。
	for ( int i = 0; i < grid.cellCount(); ++i ) {
		if ( grid.ownerCell( i ) != i ) {
			continue;
		}
		ItemId item = grid.stackAt( i ).item;
		drawIcon( spanRect( grid, i, itemSpec( item ).cellSize ), item, ( i == lifted ) ? kLiftedAlpha : kOpaque );
	}
	// ③ 数（絵の上に重ねる）。
	drawAmounts( grid );
}

void ItemGridView::drawGhost( const ItemGrid& grid, int owner, const ItemStack& stack ) {
	drawIcon( spanRect( grid, owner, itemSpec( stack.item ).cellSize ), stack.item, kGhostAlpha );
}

void ItemGridView::drawIcon( const UiRect& area, ItemId item, float alpha ) {
	const Graphics::Texture* icon = mIcons->icon( item );
	if ( icon ) {
		mRenderer->addImage( fitIcon( area, *icon ), *icon, VertexColor::pack( kOpaque, kOpaque, kOpaque, alpha ) );
	}
}

void ItemGridView::drawAmounts( const ItemGrid& grid ) {
	char text[ kTextSize ];
	for ( int i = 0; i < grid.cellCount(); ++i ) {
		if ( grid.ownerCell( i ) != i ) {
			continue;
		}
		const ItemStack& stack = grid.stackAt( i );
		const ItemSpec& spec = itemSpec( stack.item );
		UiRect area = spanRect( grid, i, spec.cellSize );
		// 重ねられる物だけ数を出す（ガソリンのように 1 つずつの物は数を出さない）。
		if ( spec.stackSize > 1 ) {
			std::snprintf( text, sizeof( text ), "%d", stack.amount );
			drawCornerText( area, text, true, kAmountPlateWidth, kTextColor );
		}
		// 銃は左下に「弾倉の数/弾倉の大きさ」。空は赤（リロードし忘れに気づけるように）。
		WeaponId weapon = weaponForItem( stack.item );
		if ( weapon != WEAPON_NONE ) {
			std::snprintf( text, sizeof( text ), kRoundsFormat, stack.loadedRounds, weaponSpec( weapon ).magazineCapacity );
			drawCornerText( area, text, false, kRoundsPlateWidth, ( stack.loadedRounds == 0 ) ? kEmptyRoundsColor : kTextColor );
		}
	}
}

void ItemGridView::drawCornerText( const UiRect& area, const char* text, bool right, float plateWidth, unsigned color ) {
	UiRect plate;
	plate.width = std::min( plateWidth, area.width - kAmountInset * kBothSides ); // 小さいマス（拠点の 40）からはみ出さない
	plate.height = kAmountPlateHeight;
	plate.left = right ? area.right() - kAmountInset - plate.width : area.left + kAmountInset;
	plate.top = area.bottom() - kAmountInset - plate.height;
	mRenderer->addRect( plate, kAmountPlateColor );
	// 板の真ん中に置く（文字の幅は UiRenderer の外から測れないので、右揃えではなく板の中央揃えにする）。
	mRenderer->addText( text, plate.center(), UI_TEXT_NORMAL, UI_ALIGN_CENTER, color );
}

void ItemGridView::drawMark( const ItemGrid& grid, ItemId item, const char* mark ) {
	if ( item == ITEM_NONE ) {
		return;
	}
	for ( int i = 0; i < grid.cellCount(); ++i ) {
		if ( grid.ownerCell( i ) != i || grid.stackAt( i ).item != item ) {
			continue;
		}
		UiRect area = cellRect( grid, i );
		UiRect plate;
		plate.left = area.left + kAmountInset;
		plate.top = area.top + kAmountInset;
		plate.width = kMarkPlateSize;
		plate.height = kMarkPlateSize;
		mRenderer->addRect( plate, kAmountPlateColor );
		mRenderer->addText( mark, plate.center(), UI_TEXT_NORMAL, UI_ALIGN_CENTER, kMarkColor );
		return; // 印を付ける物（手に持っている銃）は 1 種類 1 丁だけ
	}
}

void ItemGridView::addFrame( const UiRect& rect, float thickness, unsigned color ) {
	UiRect edge = rect;
	edge.height = thickness;
	mRenderer->addRect( edge, color ); // 上
	edge.top = rect.bottom() - thickness;
	mRenderer->addRect( edge, color ); // 下
	edge.top = rect.top;
	edge.height = rect.height;
	edge.width = thickness;
	mRenderer->addRect( edge, color ); // 左
	edge.left = rect.right() - thickness;
	mRenderer->addRect( edge, color ); // 右
}
