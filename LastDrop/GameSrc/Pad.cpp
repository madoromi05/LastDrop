#include "Pad.h"
#include "GameLib/GameLib.h"
#include "GameLib/Input/Manager.h"
#include "GameLib/Input/Keyboard.h"
#include "GameLib/Input/Mouse.h"
#include "GameLib/Input/Joystick.h"
using namespace GameLib::Input;

Pad* Pad::mInstance = 0;

Pad::Pad(){
}

Pad::~Pad(){
}

void Pad::create(){
	ASSERT( !mInstance );
	mInstance = NEW Pad();
}

void Pad::destroy(){
	ASSERT( mInstance );
	SAFE_DELETE( mInstance );
}

Pad* Pad::instance(){
	return mInstance;
}

bool Pad::getButtonState( Button b, int id, bool triggered ) const {
	bool r = false;
	Manager m = Manager::instance();
	if ( m.joystickNumber() > id ){
		Joystick j = m.joystick( id );
		switch ( b ){
			case JUMP:  r = triggered ? j.isTriggered( 0 ) : j.isOn( 0 ); break;
			case FIRE:  r = triggered ? j.isTriggered( 1 ) : j.isOn( 1 ); break;
			case UP:    r = triggered ? j.isTriggered( Joystick::BUTTON_UP )    : j.isOn( Joystick::BUTTON_UP );    break;
			case DOWN:  r = triggered ? j.isTriggered( Joystick::BUTTON_DOWN )  : j.isOn( Joystick::BUTTON_DOWN );  break;
			case LEFT:  r = triggered ? j.isTriggered( Joystick::BUTTON_LEFT )  : j.isOn( Joystick::BUTTON_LEFT );  break;
			case RIGHT: r = triggered ? j.isTriggered( Joystick::BUTTON_RIGHT ) : j.isOn( Joystick::BUTTON_RIGHT ); break;
			default: ASSERT( false ); break;
		}
	}
	Keyboard k = m.keyboard();
	char c = 0;
	if ( id == 0 ){
		switch ( b ){
			case UP:    c = 'w'; break;
			case DOWN:  c = 's'; break;
			case LEFT:  c = 'a'; break;
			case RIGHT: c = 'd'; break;
			case JUMP:  c = ' '; break;
			case FIRE:  break;
			default: ASSERT( false ); break;
		}
		if ( b == FIRE ){
			Mouse mouse = m.mouse();
			r = r || ( triggered ? mouse.isTriggered( Mouse::BUTTON_LEFT ) : mouse.isOn( Mouse::BUTTON_LEFT ) );
		}
	}else if ( id == 1 ){
		switch ( b ){
			case UP:    c = 'i'; break;
			case DOWN:  c = 'm'; break;
			case LEFT:  c = 'j'; break;
			case RIGHT: c = 'k'; break;
			case JUMP:  c = 'l'; break;
			case FIRE:  c = ','; break;
			default: ASSERT( false ); break;
		}
	}
	if ( c != 0 ){
		r = r || ( triggered ? k.isTriggered( c ) : k.isOn( c ) );
	}
	return r;
}

bool Pad::isOn( Button b, int id ) const {
	return getButtonState( b, id, false );
}

bool Pad::isTriggered( Button b, int id ) const {
	return getButtonState( b, id, true );
}
