#ifndef INCLUDED_FIRE_CONTROL_H
#define INCLUDED_FIRE_CONTROL_H

// 連射（フルオート）の間隔を決める。決めるのは「このティックに1発撃つか」だけ。
// 弾を出す・音を鳴らす・入力を読むことは持たない（引き金の状態は update の引数で受け取るので、
// キー割り当てや「今は撃てない」の条件が変わっても、このクラスには波及しない）。
// 連射の速さを変えるときは FireControl.cpp の定数だけを直す。
class FireControl{
public:
	FireControl();

	// 1ティック進める。このティックに1発撃つなら true。
	// triggerHeld: 引き金を引いているか（押されている間）。撃てない状態（武器をしまっている等）のときは、呼び出し側が false を渡す。
	// 引き金を離しても次の1発までの間隔は進むので、連打しても連射より速くは撃てない。
	bool update( bool triggerHeld );

private:
	int mCooldownTicks; // 次に撃てるようになるまでの残り（ティック）。0 なら撃てる
};

#endif
