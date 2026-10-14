#include "GameSrc/UI/UiInput.h"
#include "GameSrc/Core/Pad.h"

#include "GameLib/Input/Manager.h"
#include "GameLib/Input/Mouse.h"

using namespace GameLib;
using namespace GameLib::Math;

UiInput UiInput::read( const Vector2& previousCursor ) {
	GameLib::Input::Mouse mouse = GameLib::Input::Manager::instance().mouse();
	UiInput input;
	input.cursor.set( static_cast< float >( mouse.x() ), static_cast< float >( mouse.y() ) );
	input.cursorMoved = input.cursor.x != previousCursor.x || input.cursor.y != previousCursor.y;
	input.clicked = mouse.isTriggered( GameLib::Input::Mouse::BUTTON_LEFT );
	input.upPressed = InputUiUp();
	input.downPressed = InputUiDown();
	input.decidePressed = InputUiDecide();
	return input;
}
