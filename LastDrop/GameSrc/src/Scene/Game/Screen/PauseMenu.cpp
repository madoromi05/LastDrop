#include "GameSrc/Scene/Game/Screen/PauseMenu.h"
#include "GameSrc/Scene/Menu/MenuCatalog.h"
#include "GameSrc/UI/UiRenderer.h"
#include "GameSrc/Render/VertexColor.h"

#include "GameLib/GameLib.h"
#include "Framework.h"

namespace{
	const unsigned kDimColor = VertexColor::pack( 0.f, 0.f, 0.f, 0.74f );	// 止めた画を暗くする板の色。後ろの町がうっすら見え（どこで止めたかが分かる）、上に直接書く細い白い字が読める暗さ
}

PauseMenu::PauseMenu( UiRenderer* renderer, GameSettings* settings, AudioEngine* audio, std::function< void() > onResume ) :
	mPages( renderer, settings, audio, MenuCatalog::pause( onResume ) ),
	mRenderer( renderer ) {
	ASSERT( mRenderer && "PauseMenu : renderer is null." );
}

void PauseMenu::open() {
	mPages.showMain();
}

void PauseMenu::handleInput() {
	mPages.handleInput();
}

bool PauseMenu::back() {
	return mPages.back();
}

void PauseMenu::draw() {
	// 板を先に積む（UiRenderer は積んだ順に長方形を描くので、選んでいる行の帯より下になる）
	UiRect screen;
	screen.left = 0.f;
	screen.top = 0.f;
	screen.width = static_cast< float >( GameLib::Framework::instance().width() );
	screen.height = static_cast< float >( GameLib::Framework::instance().height() );
	mRenderer->addRect( screen, kDimColor );
	mPages.draw();
}
