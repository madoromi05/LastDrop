#ifndef INCLUDED_AIM_SWAY_H
#define INCLUDED_AIM_SWAY_H

// 銃を構えているときの照準のぶれ（左右・上下の角度）を決める。仕様は SummaryPlan/個別仕様/射撃と手応え/AimSway_Plan.txt。
// 3 つを足す: a. 息（止まっていても小さく 8 の字）、b. 歩き（歩くと大きく、止まっても少し残る）、
//             c. 振り向きの遅れ（マウスで回すと、銃が少し遅れて付いてくる）。
// その後で、腰だめ（覗いていない）なら大きく、腕が壊れていれば大きく、走って胸元に抱えている間・しまっている間は 0 にする。
//
// ぶれはカメラ（画面の中央）には入れない。銃の見た目（FirstPersonBody）と弾の向き（FirstPersonCamera::weaponDirection）
// だけがぶれる（覗くとサイトが画面の中で少し動き、弾はサイトの先へ飛ぶ。タルコフと同じ）。足し先を決めるのは持ち主
// （FirstPersonControl）。反動（AimRecoil）・怯み（AimPunch）はカメラごと動かすので、別の量にしてある。
// 乱数は使わない（決まった形の波）。大きさ・速さを変えるときは AimSwayParams の値を直す（F6 で動かせる）。
//
// 自キャラの今の状態。PlayerCharacter::aimSwayBody で作る。
struct AimSwayBody{
	bool weaponDrawn = false;  // 銃を手に持っているか（しまっている間はぶれない）
	float aimWeight = 0.f;     // 覗き具合（0〜1）
	float walkRate = 0.f;      // 今の速さ ÷ 銃を持った歩きの速さ（0〜1。走っても 1 で止める）
	float gaitPhase = 0.f;     // 歩きのアニメの位相（0〜1）
	bool gaitCycling = false;  // 歩き・走りのアニメ中か（止まるアニメの位相には寄せない）
	float carryWeight = 0.f;   // 走って胸元に抱えている割合（0〜1）
	bool danger = false;       // 体力が Danger か（PlayerCondition）
};

class AimSway{
public:
	AimSway();

	// 1ティック進める。lookYaw / lookPitch: このティックにマウスで回した角度（度。FirstPersonCamera::rotate に渡したのと同じ約束）。
	void update( const AimSwayBody& body, float lookYaw, float lookPitch );
	// 弾と銃の向きへ足す左右の角度（度。FirstPersonCamera のヨーと同じ約束で、正が左）。
	float yawOffset() const;
	// 弾と銃の向きへ足す上下の角度（度。FirstPersonCamera のピッチと同じ約束で、正が見下ろす向き）。
	float pitchOffset() const;

private:
	// 息の波の進み具合（0〜1 で 1 往復）。左右と上下で別々に進める（F6 で周期を変えても、波が途中から跳ばない）。
	float mBreathYawPhase;
	float mBreathPitchPhase;
	float mWalkStrength; // 歩きの強さ（0〜1）
	float mWalkPhase;    // ぶれの側で進める歩きの位相（止まると進まないので、形を止めたまま強さだけ下げる）
	float mLagYaw;       // 振り向きの遅れで残っているずれ（度）
	float mLagPitch;
	float mYaw;          // 最後に求めたぶれ（度）
	float mPitch;
};

#endif
