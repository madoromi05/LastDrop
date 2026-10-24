#include "GameSrc/UI/UiCanvas.h"
#include "GameSrc/UI/UiInput.h"
#include "GameSrc/UI/UiRenderer.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

namespace{
	const int kNone = -1; // 何も選んでいない
}

UiCanvas::UiCanvas() :
	mSelected( kNone ),
	mDragging( kNone ) {
}

void UiCanvas::addSelectable( IUiSelectable* selectable, IUiAdjustable* adjustable ) {
	ASSERT( selectable && "UiCanvas : selectable is null." );
	mSelectables.push_back( selectable );
	mAdjustables.push_back( adjustable );
	if ( mSelected == kNone ) {
		select( 0 );
	}
}

void UiCanvas::handleInput( const UiInput& input ) {
	if ( mSelectables.empty() ) {
		return;
	}
	int count = static_cast< int >( mSelectables.size() );
	// マウスで値を動かしている間は、離すまでその部品だけに付いていく（ほかの部品の上を通っても選び直さない）。
	if ( !input.held ) {
		mDragging = kNone;
	} else if ( mDragging != kNone && input.cursorMoved ) {
		mAdjustables[ mDragging ]->dragTo( input.cursor );
	}
	// マウス：動いたときだけ、カーソルの下の部品を選ぶ（止まっているマウスがキーで選んだものを奪わないように）。
	int pointed = findSelectableAt( input.cursor );
	if ( input.cursorMoved && pointed != kNone && mDragging == kNone ) {
		select( pointed );
	}
	if ( input.upPressed ) {
		select( ( mSelected + count - 1 ) % count );
	}
	if ( input.downPressed ) {
		select( ( mSelected + 1 ) % count );
	}
	IUiAdjustable* selectedAdjustable = ( mSelected != kNone ) ? mAdjustables[ mSelected ] : nullptr;
	if ( selectedAdjustable && input.leftPressed ) {
		selectedAdjustable->adjust( -1 );
	}
	if ( selectedAdjustable && input.rightPressed ) {
		selectedAdjustable->adjust( 1 );
	}
	// 決定は最後に1回だけ（activate でシーンが切り替わる予約をしたら、この画面ではそれ以上何もしない）。
	// 値を動かせる部品のクリックは、決定ではなく「その場所の値にして、ドラッグを始める」。
	if ( input.clicked && pointed != kNone && mAdjustables[ pointed ] ) {
		select( pointed );
		mDragging = pointed;
		mAdjustables[ pointed ]->dragTo( input.cursor );
	} else if ( input.clicked && pointed != kNone ) {
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
