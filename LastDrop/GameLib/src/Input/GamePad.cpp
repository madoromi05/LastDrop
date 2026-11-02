#include "GameLib/GameLib.h"
#include "GameLib/Input/GamePad.h"

#include "Input/GamePadImpl.h"

namespace GameLib{
namespace Input{

GamePad::GamePad() : mImpl( 0 ){
}

GamePad::GamePad( Impl* impl ) : mImpl( impl ){
}

GamePad::~GamePad(){
	mImpl = 0;
}

bool GamePad::isConnected() const {
	return mImpl->mConnected;
}

bool GamePad::isOn( Button i ) const {
	return mImpl->isOn( i );
}

bool GamePad::isTriggered( Button i ) const {
	return mImpl->isTriggered( i );
}

float GamePad::stickX( Stick i ) const {
	return mImpl->stickX( i );
}

float GamePad::stickY( Stick i ) const {
	return mImpl->stickY( i );
}

const char* GamePad::sourceName() const {
	return mImpl->mSourceName;
}

GamePad::operator void*() const {
	return ( mImpl == 0 ) ? 0 : reinterpret_cast< void* >( ~0 );
}

} //namespace Input
} //namespace GameLib
