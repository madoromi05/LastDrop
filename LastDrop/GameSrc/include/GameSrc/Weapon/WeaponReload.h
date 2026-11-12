#ifndef INCLUDED_WEAPON_RELOAD_H
#define INCLUDED_WEAPON_RELOAD_H

#include "GameSrc/Weapon/WeaponCatalog.h"

// リロードの時間割（始めてから込め終わるまで）を決める
// 決めるのは「リロード中か」「このティックに何の合図（弾倉を抜く・挿す・槓桿を引く）が来たか」と、
// リロードのアニメの何フレーム目を見せるか・腕をどれだけアニメに任せるか、だけ
// 何発込めるか（Magazine と持ち物）・腕をどう動かすか（FirstPersonBody）・音（PlayerFoley）は知らない
// どのアニメを使い、合図がいつ来て、何秒で込め終わるかは銃ごとの表（WeaponCatalog の WeaponReloadSpec）
// 弾倉が空のときと残っているときで別の時間割を使える（拳銃は空のときだけスライドを引く。PistolAnimation_Plan.txt Q4）
//
// 弾は込め終わった瞬間に込める（途中でやめれば 1 発も入らない）。弾倉に残っていた弾は捨てない（バイオハザードと同じ）
class WeaponReload{
public:
	// 音を鳴らす合図
	enum Cue{
		CUE_NONE,
		CUE_MAG_OUT, // 弾倉を抜いた
		CUE_MAG_IN,  // 新しい弾倉を挿した
		CUE_BOLT,    // 槓桿を引いた
	};

	// normal: 弾倉に弾が残っているときの時間割。empty: 空のとき。どちらも表の行（表は動かないので借りるだけ）
	WeaponReload( const WeaponReloadSpec& normal, const WeaponReloadSpec& empty );

	// 始める。もうリロード中なら何もしない（始めからやり直さない）。始めてよいか（弾倉が満タンでない・予備がある）は呼び出し側が決める
	// magazineEmpty: 弾倉が空か（空のときの時間割を使う）
	void start( bool magazineEmpty );
	// 見せるリロードのアニメの名前（今の時間割の。リロード中でなくても、最後に選んだ物を返す）
	const char* animationName() const;
	// やめる（弾は込めない）。リロード中でなければ何もしない
	void cancel();
	// 1ティック進める。込め終わったティックに true を返す（このとき呼び出し側が弾を込める）
	bool update();

	bool isActive() const;	// リロード中か
	Cue cueThisTick() const;	// 直前の update で来た合図（無ければ CUE_NONE）
	// 見せるリロードのアニメ時刻（フレーム）。リロード中でなければ 0
	float animationFrame() const;
	// 腕をリロードのアニメに任せる量（0 = 今のアニメのまま、1 = アニメどおり）。始め・終わりはなめらかに切り替える
	float armWeight() const;
	// 弾倉が銃から外れて手にあるか（弾倉を抜く合図から、挿す合図の前まで）
	bool isMagazineOut() const;
	// 手の弾倉を銃の差し込み口へ寄せる量（0 = 手のまま、1 = はまった）。挿す合図の少し前から 1 へ向かう
	float magazineSeatWeight() const;

private:
	float elapsedSeconds() const; // 始めてからの秒
	// 時間割を選び、合図のティックを求め直す（start から）
	void select( bool magazineEmpty );
	// アニメを流す速さ（1 でアニメのまま）
	float animationRate() const;
	// アニメの時刻（秒）を、リロードを始めてからのティックに直す（速めて流すときは、アニメの時刻より早く来る）
	int animationSecondsToTicks( float animationSeconds ) const;

	const WeaponReloadSpec* mNormal; // 借りているだけ
	const WeaponReloadSpec* mEmpty;
	const WeaponReloadSpec* mSpec;   // 今の時間割（mNormal か mEmpty）
	int mDurationTicks;
	int mMagOutTicks;       // 弾倉を抜く合図のティック（始めてから）
	int mMagInTicks;        // 挿す合図
	int mBoltTicks;         // 槓桿を引く合図（引かない時間割なら -1）

	int mTicks;	// 始めてから進んだティック数。リロード中でなければ -1
	Cue mCue;
};

#endif
