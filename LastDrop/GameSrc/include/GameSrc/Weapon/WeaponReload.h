#ifndef INCLUDED_WEAPON_RELOAD_H
#define INCLUDED_WEAPON_RELOAD_H

// リロードの時間割（始めてから込め終わるまで）を決める。
// 決めるのは「リロード中か」「このティックに何の合図（弾倉を抜く・挿す・槓桿を引く）が来たか」と、
// リロードのアニメ（Basic Shooter Pack の reloading）の何フレーム目を見せるか・腕をどれだけアニメに任せるか、だけ。
// 何発込めるか（Magazine と持ち物）・腕をどう動かすか（FirstPersonBody）・音（PlayerFoley）は知らない。
// 音の合図はアニメの手の動きに合わせた時刻なので、アニメを替えたら WeaponReload.cpp の時刻も合わせ直す。
//
// 弾は込め終わった瞬間に込める（途中でやめれば 1 発も入らない）。弾倉に残っていた弾は捨てない（バイオハザードと同じ）。
class WeaponReload{
public:
	// 音を鳴らす合図。
	enum Cue{
		CUE_NONE,
		CUE_MAG_OUT, // 弾倉を抜いた
		CUE_MAG_IN,  // 新しい弾倉を挿した
		CUE_BOLT,    // 槓桿を引いた
	};

	WeaponReload();

	// 始める。もうリロード中なら何もしない（始めからやり直さない）。始めてよいか（弾倉が満タンでない・予備がある）は呼び出し側が決める。
	void start();
	// やめる（弾は込めない）。リロード中でなければ何もしない。
	void cancel();
	// 1ティック進める。込め終わったティックに true を返す（このとき呼び出し側が弾を込める）。
	bool update();

	bool isActive() const;	// リロード中か
	Cue cueThisTick() const;	// 直前の update で来た合図（無ければ CUE_NONE）
	// 見せるリロードのアニメ時刻（フレーム）。リロード中でなければ 0。
	float animationFrame() const;
	// 腕をリロードのアニメに任せる量（0 = 今のアニメのまま、1 = アニメどおり）。始め・終わりはなめらかに切り替える。
	float armWeight() const;
	// 弾倉が銃から外れて手にあるか（弾倉を抜く合図から、挿す合図の前まで）。
	bool isMagazineOut() const;
	// 手の弾倉を銃の差し込み口へ寄せる量（0 = 手のまま、1 = はまった）。挿す合図の少し前から 1 へ向かう。
	float magazineSeatWeight() const;

private:
	float elapsedSeconds() const; // 始めてからの秒

	int mTicks;	// 始めてから進んだティック数。リロード中でなければ -1
	Cue mCue;
};

#endif
