#include "GameSrc/Scene/Game/Screen/ModalController.h"
#include "GameSrc/Audio/AudioEngine.h"
#include "GameSrc/Player/Camera/FirstPersonControl.h"

ModalController::ModalController( AudioEngine* audio, FirstPersonControl* control ) :
	mAudio( audio ),
	mControl( control ),
	mMode( Mode::None ) {
}

void ModalController::enter( Mode mode ) {
	mMode = mode;
	mAudio->setPaused( true );   // 世界の音を止める（戻ったとき早送りにならない）
	mControl->clearInput();      // 押していた移動・撃つを放す（戻った後も、離すまで撃たない）
	mControl->unlockCursor();    // ボタン・マスをマウスで操作できるように
}

void ModalController::exit() {
	mMode = Mode::None;
	mAudio->setPaused( false );
	mControl->lockCursor();
}
