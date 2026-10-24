#ifndef INCLUDED_GAMELIB_INPUT_MANAGER_H
#define INCLUDED_GAMELIB_INPUT_MANAGER_H

namespace GameLib{
namespace Math{
	class Vector2;
}
namespace Input{
class Keyboard;
class Mouse;
class Joystick;

class Manager{
public:
	Manager();
	static Manager instance();

	Mouse mouse() const;
	Keyboard keyboard() const;
	Joystick joystick( int i = 0 ) const;
	int joystickNumber() const;

	// カーソルロック（TPS/FPS の視点操作用）。ロック中は毎フレームカーソルを画面中央へ戻し、
	// 中央からのずれを Mouse::velocityX/Y として返す（画面端で移動量が0にならない）。
	// ロック中はカーソルを隠し、ウィンドウの外へ出ないよう閉じ込める。ウィンドウが非アクティブの間は効かない。
	// Mouse::x/y はロック中は画面中央を指したままになる。
	void setCursorLocked( bool );
	bool isCursorLocked() const;

	// 開発用のエディタ（DebugUi）が入力を使っている間、ゲームに渡さない（FPS のカーソルロックとの取り合いを避けるため）。
	// mouse：ボタン・ホイール・移動量を 0 にし、カーソルロックも外す（ゲームが頼んだロックは覚えておき、外れたら戻す）。
	// keyboard：キーを全部離したことにする。DebugUi が毎フレーム呼ぶ。効くのは次に入力を読むとき。
	void setUiCapture( bool mouse, bool keyboard );

	//以下知らなくていい
	static void create( void* windowHandle );
	static void destroy();
	void update( float pointerScale, const Math::Vector2& pointerOffset );
};
}
}

#endif
