#include "GameSrc/UI/UiCanvas.h"
#include "GameSrc/UI/UiInput.h"
#include "GameSrc/UI/UiRenderer.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

namespace{
	const int kNone = -1; // 何も選んでいない
}

UiCanvas::UiCanvas() :
	mSelected( kNone ) {
}

void UiCanvas::addSelectable( IUiSelectable* selectable ) {
	ASSERT( selectable && "UiCanvas : selectable is null." );
	mSelectables.push_back( selectable );
	if ( mSelected == kNone ) {
		select( 0 );
	}
}

void UiCanvas::handleInput( const UiInput& input ) {
	if ( mSelectables.empty() ) {
		return;
	}
	int count = static_cast< int >( mSelectables.size() );
	// マウス：動いたときだけ、カーソルの下の部品を選ぶ（止まっているマウスがキーで選んだものを奪わないように）。
	int pointed = findSelectableAt( input.cursor );
	if ( input.cursorMoved && pointed != kNone ) {
		select( pointed );
	}
	if ( input.upPressed ) {
		select( ( mSelected + count - 1 ) % count );
	}
	if ( input.downPressed ) {
		select( ( mSelected + 1 ) % count );
	}
	// 決定は最後に1回だけ（activate でシーンが切り替わる予約をしたら、この画面ではそれ以上何もしない）。
	if ( input.clicked && pointed != kNone ) {
		select( pointed );
		mSelectables[ mSelected ]->activate();
	} else if ( input.decidePressed && mSelected != kNone ) {
		mSelectables[ mSelected ]->activate();
	}
}

void UiCanvas::draw( UiRenderer& renderer ) const {
	for ( const std::unique_ptr< IUiElement >& element : mElements ) {
		element->draw( renderer );
	}
}

void UiCanvas::select( int index ) {
	if ( mSelected != kNone ) {
		mSelectables[ mSelected ]->setSelected( false );
	}
	mSelected = ( index >= 0 && index < static_cast< int >( mSelectables.size() ) ) ? index : kNone;
	if ( mSelected != kNone ) {
		mSelectables[ mSelected ]->setSelected( true );
	}
}

int UiCanvas::findSelectableAt( const Vector2& point ) const {
	for ( int i = 0; i < static_cast< int >( mSelectables.size() ); ++i ) {
		if ( mSelectables[ i ]->contains( point ) ) {
			return i;
		}
	}
	return kNone;
}
