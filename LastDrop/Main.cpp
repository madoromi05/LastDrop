#include "Framework.h"
#include "GameLibMath.h"
#include "GameLib/Input/Manager.h"
#include "GameLib/Input/Keyboard.h"
#include "GameLib/Graphics/Manager.h"
#include "GameLib/Math/Matrix44.h"
#include "GameLib/Math/Matrix34.h"
#include "GameLib/Math/Vector3.h"

using namespace GameLib;

// Called every frame. Implement game logic here.
void Framework::update(){
	Framework f = Framework::instance();

	// Exit on ESC
	Input::Keyboard kb = Input::Manager().keyboard();
	if ( kb.isOn( Input::Keyboard::KEY_ESC ) ){
		f.requestEnd();
	}

	f.drawDebugString( 0, 0, "DirectX11 Turn Command Battle" );
	f.drawDebugString( 0, 1, "Press ESC to exit" );
}
