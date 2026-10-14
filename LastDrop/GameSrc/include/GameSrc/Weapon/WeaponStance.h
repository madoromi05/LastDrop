#ifndef INCLUDED_WEAPON_STANCE_H
#define INCLUDED_WEAPON_STANCE_H

// 武器の構えの状態（しまう／持つ／覗く）を決める。
// 決めるのは「今どの段階か」と「覗き具合（0〜1）」だけ。アニメ・見た目・入力の読み取りは持たない
// （入力は update の引数で受け取るので、キー割り当てやパッドの違いはこのクラスに波及しない）。
//
// 覗く操作は Escape from Tarkov と同じ: 短く押して離すと覗いたまま（もう一度押すとやめる）、
// 長く押すと押している間だけ覗く。武器をしまうと覗くのをやめる。
// 走りより覗く方を優先する（覗いている間、呼び出し側は走らせず歩きにする）。ただし覗いたまま（短押し）のときは、
// 走る入力を押し直した瞬間に覗くのをやめる（押し直さない限り覗いたまま歩く）。
class WeaponStance{
public:
	WeaponStance();

	// 1フレーム進める。
	// holsterPressed: 出し入れの入力（押した瞬間）。aimHeld: 覗く入力（押されている間）。
	// runHeld: 走ろうとしているか（走る入力が押されていて、移動の入力もある間）。押した瞬間に覗いたままをやめる。
	void update( bool holsterPressed, bool aimHeld, bool runHeld );

	bool isDrawn() const;	// 武器を持っているか（しまっていれば false）
	bool isAiming() const;	// 覗いているか（覗き具合が 1 へ向かっている途中も true）
	// 覗き具合（0 = 持っているだけ、1 = サイトが目の前）。構え始め・終わりがゆっくりになる曲線を掛けた値。
	float aimWeight() const;

private:
	enum AimMode{
		AIM_OFF,	// 覗いていない
		AIM_HOLD,	// 押している最中（離したときの長さで、覗いたままにするか決める）
		AIM_TOGGLE,	// 短押しで覗いたまま
	};
	// 覗く入力の押した瞬間・離した瞬間から覗き方を決める。
	void updateAimMode( bool aimHeld );

	bool mDrawn;
	AimMode mAimMode;
	bool mAimWasHeld;		// 前のフレームに覗く入力が押されていたか（押した瞬間・離した瞬間を知るため）
	bool mRunWasHeld;		// 前のフレームに走ろうとしていたか（走る入力を押した瞬間を知るため）
	int mAimHeldFrames;		// AIM_HOLD になってから押し続けたフレーム数
	float mAimProgress;		// 覗き具合（曲線を掛ける前。0〜1 を一定の速さで動く）
};

#endif
