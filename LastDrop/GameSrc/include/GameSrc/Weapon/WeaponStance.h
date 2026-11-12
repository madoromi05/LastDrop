#ifndef INCLUDED_WEAPON_STANCE_H
#define INCLUDED_WEAPON_STANCE_H

// 武器の構えの状態（持つ／覗く）を決める
// 決めるのは「覗いているか」と「覗き具合（0〜1）」だけ。アニメ・見た目・入力の読み取りは持たない
// （入力は update の引数で受け取るので、キー割り当てやパッドの違いはこのクラスに波及しない）
// 武器をしまう操作は無い（銃を手に持っている間はいつも出している）
//
// 覗く操作は Escape from Tarkov と同じ: 短く押して離すと覗いたまま（もう一度押すとやめる）、
// 長く押すと押している間だけ覗く
// 走りより覗く方を優先する（覗いている間、呼び出し側は走らせず歩きにする）。ただし覗いたまま（短押し）のときは、
// 走る入力を押し直した瞬間に覗くのをやめる（押し直さない限り覗いたまま歩く）
class WeaponStance{
public:
	WeaponStance();

	// 1フレーム進める
	// aimHeld: 覗く入力（押されている間）
	// runHeld: 走ろうとしているか（走る入力が押されていて、移動の入力もある間）。押した瞬間に覗いたままをやめる
	void update( bool aimHeld, bool runHeld );
	// 覗くのをやめる（覗いたまま・長押しのどちらも）。覗き具合は次の update から一定の速さで戻る
	// リロードを始めたとき・銃を手放したときに呼ぶ（リロード中は呼び出し側が aimHeld を false で渡し、覗き直さないようにする）
	void stopAiming();

	bool isAiming() const;		// 覗いているか（覗き具合が 1 へ向かっている途中も true）
	float aimWeight() const;	// 覗き具合（0 = 持っているだけ、1 = サイトが目の前）。構え始め・終わりがゆっくりになる曲線を掛けた値

private:
	enum AimMode{
		AIM_OFF,	// 覗いていない
		AIM_HOLD,	// 押している最中（離したときの長さで、覗いたままにするか決める）
		AIM_TOGGLE,	// 短押しで覗いたまま
	};
	void updateAimMode( bool aimHeld );	// 覗く入力の押した瞬間・離した瞬間から覗き方を決める

	AimMode mAimMode;
	bool mAimWasHeld;		// 前のフレームに覗く入力が押されていたか（押した瞬間・離した瞬間を知るため）
	bool mRunWasHeld;		// 前のフレームに走ろうとしていたか（走る入力を押した瞬間を知るため）
	int mAimHeldFrames;		// AIM_HOLD になってから押し続けたフレーム数
	float mAimProgress;		// 覗き具合（曲線を掛ける前。0〜1 を一定の速さで動く）
};

#endif
