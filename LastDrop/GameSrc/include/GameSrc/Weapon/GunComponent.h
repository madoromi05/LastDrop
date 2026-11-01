#ifndef INCLUDED_GUN_COMPONENT_H
#define INCLUDED_GUN_COMPONENT_H

#include "GameSrc/World/Component.h"
#include "GameSrc/Weapon/FireControl.h"
#include "GameSrc/Weapon/Magazine.h"
#include "GameSrc/Weapon/WeaponRecoil.h"
#include "GameSrc/Weapon/WeaponReload.h"

class Inventory;

// 銃（"AK47"）の撃つ仕組み（画像の GunComponent）。連射の間隔は FireControl、弾倉の弾の数は Magazine、
// 撃った反動の量は WeaponRecoil、リロードの時間割は WeaponReload に任せる（委譲）。
// 引き金を引くか・リロードを始めるかは持つ側（PlayerComponent が HoldComponent 越しに）が決める。ここは「撃てたか」だけを答える。
// 弾を出す・銃声を鳴らすのは、それを読む側（GameWorld・PlayerBodySounds）。
// 予備の弾は持ち物（Inventory の ITEM_AMMO）にあり、リロードを込め終えた瞬間に、そこから弾倉の足りない分だけ移す（弾倉に残っていた弾は捨てない）。
// 弾倉が空で撃つ番が来たら、弾は出ずに空撃ちになる（dryFiredThisTick）。引き金を離すまで次の空撃ちはしない。
class GunComponent : public Component{
public:
	GunComponent();

	// 撃つ番が来たら、弾倉に弾があれば 1 発撃ち、空なら空撃ちにする（1 ティックに 1 回）。
	// pulling: 撃てる状態で引き金を引いているか。triggerHeld: 撃てるかに関わらず引き金を引いているか（離したら空撃ちの止めを外す）。
	// 撃てない間も呼ぶ（次の1発までの間隔を、撃てない間に止めないため）。
	void pullTrigger( bool pulling, bool triggerHeld );
	// 反動を 1 ティック進める（pullTrigger の後に呼ぶ。撃ったティックに跳ねる）。
	void updateRecoil();
	float recoilKick() const;  // 今の反動の量（FirstPersonBody::setRecoil へ渡す）

	// ── リロード ──
	bool canStartReload( const Inventory& inventory ) const; // リロード中でない・弾倉が満タンでない・予備がある
	void startReload();
	void cancelReload();  // 替えている途中でやめる（弾は込めない）
	// 時間割を 1 ティック進め、込め終えたら予備（inventory）から弾倉の足りない分だけ移す。リロード中でなくても毎ティック呼ぶ（揺れを戻し、合図を消すため）。
	void updateReload( Inventory* inventory );
	const WeaponReload& reload() const;

	bool firedThisTick() const;     // 直前の pullTrigger で1発撃ったか
	bool dryFiredThisTick() const;  // 直前の pullTrigger で、弾倉が空のまま引き金を引いたか
	bool isReloading() const;
	WeaponReload::Cue reloadCueThisTick() const; // 直前の updateReload で来たリロードの合図（弾倉を抜く・挿す・槓桿）
	int magazineRounds() const;     // 弾倉に入っている弾の数

private:
	FireControl mFire;         // 連射の間隔
	Magazine mMagazine;        // 弾倉の弾の数
	WeaponRecoil mRecoil;      // 撃った反動の量
	WeaponReload mReload;      // リロードの時間割と、アニメの何フレーム目を見せるか
	bool mFiredThisTick;
	bool mDryFiredThisTick;
	bool mDryFireLatched;      // 空撃ちした後、まだ引き金を離していないか（離すまで次の空撃ちをしない）
};

#endif
