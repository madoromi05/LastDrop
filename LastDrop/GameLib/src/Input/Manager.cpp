#include "GameLib/GameLib.h"
#include "GameLib/Input/Manager.h"
#include "GameLib/Input/Keyboard.h"
#include "GameLib/Input/Mouse.h"
#include "GameLib/Input/Joystick.h"

#include "Input/ManagerImpl.h"

namespace GameLib{
	namespace Input{

		// 入力システムの実体(唯一のインスタンス)。
		// Manager は値渡しできる軽量ハンドルに過ぎず、状態はすべてここに集約する。
		// どの Manager ハンドルからも同じ実体を参照させるため、また
		// create/destroy でプロセス全体の生存期間を一元管理するためにグローバルにしている。
		ManagerImpl* gManagerImpl;

		Manager::Manager(){
			// 入力デバイスはメインスレッドのウィンドウに紐付くため、
			// 別スレッドからハンドルを生成させない(生成時点で不正利用を弾く)。
			ASSERT( WindowCreator::isMainThread() && "you must call from MAIN thread" );
		}

		void Manager::create( void* wh ){
			// 二重生成は実体のリークにつながるため強く禁止する。
			STRONG_ASSERT( !gManagerImpl );
			// void* で受けるのは公開ヘッダを Windows.h から切り離すため。ここで HWND へ復元する。
			gManagerImpl = NEW ManagerImpl( static_cast< HWND >( wh ) );
		}

		void Manager::destroy(){
			SAFE_DELETE( gManagerImpl );
		}

		void Manager::update( float pointerScale, const Vector2& pointerOffset ){
			gManagerImpl->update( pointerScale, pointerOffset );
		}

		// ハンドル自体は状態を持たないため、コンストラクタで生成して値返しするだけでよい。
		Manager Manager::instance(){
			return Manager();
		}

		int Manager::joystickNumber() const {
			return gManagerImpl->mJoystickNumber;
		}

		void Manager::setCursorLocked( bool locked ){
			gManagerImpl->setCursorLocked( locked );
		}

		bool Manager::isCursorLocked() const {
			return gManagerImpl->mCursorLocked;
		}

		void Manager::setUiCapture( bool mouse, bool keyboard ){
			gManagerImpl->mUiCapturesMouse = mouse;
			gManagerImpl->mUiCapturesKeyboard = keyboard;
		}

		Keyboard Manager::keyboard() const {
			return Keyboard();
		}

		Mouse Manager::mouse() const {
			return Mouse();
		}

		Joystick Manager::joystick( int i ) const {
			// 実体の配列を直接指すハンドルを返すので、範囲外アクセスを事前に弾く。
			ASSERT( i >= 0 && i < gManagerImpl->mJoystickNumber );
			return Joystick( &gManagerImpl->mJoysticks[ i ] );
		}
	}
}
