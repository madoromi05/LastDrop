#ifndef INCLUDED_GAMELIB_INPUT_GAMEPAD_H
#define INCLUDED_GAMELIB_INPUT_GAMEPAD_H

namespace GameLib {
	namespace Input {

		// パッド 1 台。ボタンは Xbox の並び（A/B/X/Y…）で表す
		// 読む元は 2 つ。i 番のパッドは、XInput の差し口 i がつながっていれば XInput（Xbox 系）から読み、
		// つながっていなければ、対応表（GamePadMapping）にある i 番目の DirectInput のパッド（DualSense・DualShock 4）から読む
		// DirectInput のパッドの並びは対応表で Xbox の並びに置き換えるので、割り当て（RT で撃つ など）はどちらでも同じ
		// XInput は抜き差しをそのまま追えるが、DirectInput のパッドは起動したときに 1 回だけ数える（起動前につないでおく）
		// ウィンドウが非アクティブの間は、キーボード・マウスと同じく何も押していないことにする
		// 対応表に無い DirectInput の機器は Joystick として読む（こちらは並びを置き換えない）
		class GamePad {
		public:
			// ボタンの番号。保存ファイル（InputConfig）に番号で残るので、途中に挿さず末尾へ足すこと
			enum Button {
				BUTTON_A,
				BUTTON_B,
				BUTTON_X,
				BUTTON_Y,
				BUTTON_LB,
				BUTTON_RB,
				BUTTON_BACK,
				BUTTON_START,
				BUTTON_LEFT_THUMB,  // 左スティックの押し込み
				BUTTON_RIGHT_THUMB, // 右スティックの押し込み
				BUTTON_DPAD_UP,
				BUTTON_DPAD_DOWN,
				BUTTON_DPAD_LEFT,
				BUTTON_DPAD_RIGHT,
				BUTTON_LT, // トリガーを一定より深く引いたらボタンとして押したことにする
				BUTTON_RT,

				BUTTON_MAX,
			};
			enum Stick {
				STICK_LEFT,
				STICK_RIGHT,

				STICK_MAX,
			};
			static const int PAD_MAX = 4;	// 同時に読めるパッドの数（XInput の差し口の数 XUSER_MAX_COUNT と同じ。DirectInput のパッドもこの数まで）

			bool isConnected() const;
			bool isOn( Button ) const;
			bool isTriggered( Button ) const;
			// スティックの傾き（-1〜1。右・下が正）。真ん中付近の遊び（デッドゾーン）は 0 にしてある
			float stickX( Stick ) const;
			float stickY( Stick ) const;
			const char* sourceName() const;	// 今読んでいる元の名前（"XInput"・DirectInput の対応表の名前・つながっていなければ "None"）。確かめる表示用

			GamePad();
			~GamePad();
			operator void* () const;
			class Impl;
		private:
			friend class Manager;
			GamePad( Impl* );
			Impl* mImpl;
		};

	}
}

#endif
