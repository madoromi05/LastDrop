#ifndef INCLUDED_FIRE_CONTROL_H
#define INCLUDED_FIRE_CONTROL_H

// 撃つ間隔を決める。決めるのは「このティックに1発撃つか」だけ
// 弾を出す・音を鳴らす・入力を読むことは持たない（引き金の状態は update の引数で受け取るので、
// キー割り当てや「今は撃てない」の条件が変わっても、このクラスには波及しない）
// 連射の速さと撃ち方（フルオート／セミオート）は銃ごとの表（WeaponCatalog）で決まり、作る時に受け取る
class FireControl{
public:
	// roundsPerMinute: 連射の速さ（発/分。0 より大きい）。セミオートでは、次に撃てるまでの最短の間隔になる
	// fullAuto: true なら引いている間撃ち続ける。false なら 1 回引くと 1 発（離してもう一度引くまで撃たない）
	FireControl( float roundsPerMinute, bool fullAuto );

	// 1ティック進める。このティックに1発撃つなら true
	// triggerHeld: 引き金を引いているか（押されている間）。撃てない状態（武器をしまっている等）のときは、呼び出し側が false を渡す
	// 引き金を離しても次の1発までの間隔は進むので、連打しても連射より速くは撃てない
	bool update( bool triggerHeld );

private:
	int mIntervalTicks; // 1発ごとの間隔（ティック）
	bool mFullAuto;
	int mCooldownTicks; // 次に撃てるようになるまでの残り（ティック）。0 なら撃てる
	bool mWasHeld;      // 前のティックに引き金を引いていたか（セミオートで、引いたままの間は撃たないため）
};

#endif
