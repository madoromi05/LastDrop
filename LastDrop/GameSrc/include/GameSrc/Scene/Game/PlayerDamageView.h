#ifndef INCLUDED_PLAYER_DAMAGE_VIEW_H
#define INCLUDED_PLAYER_DAMAGE_VIEW_H

// 自キャラが傷ついたときの見せ方の量を、時間で決める（SummaryPlan/個別仕様/ZombieAttack_Plan.txt の 5 章）。
// ・噛まれた瞬間：画面の周りを赤くし、少しずつ消す（PostProcess::setDamageFlash へ渡す量）
//                 痛みで周りをぼかし・暗くし、少しずつ消す（PostProcess::setPain へ渡す量。CameraShake_Recoil_Plan.txt の 4 章）
// ・死んだ後　　：視点を下げ（膝から崩れる。FirstPersonControl::setEyeDrop へ渡す長さ）、画面を暗くしていく（PostProcess::setFade へ渡す量）
// 量を決めるだけで、画面にも視点にも触らない（渡すのはシーン）。暗くなり終わったかを答え、シーンがゲームオーバー画面へ切り替える。
// 体力も知らない（噛まれたか・死んだかを毎ティック受け取るだけ）。
class PlayerDamageView{
public:
	PlayerDamageView();

	// 1ティック進める。bitten: このティックに噛まれたか。dead: 自キャラが死んでいるか（一度 true になったら戻らない前提）。
	void update( bool bitten, bool dead );

	float damageFlash() const; // 画面の周りの赤み（0〜1）
	float pain() const;        // 痛みの強さ（0〜1）。画面の周りのぼかしと暗さ
	float eyeDrop() const;     // 目の位置から下げる長さ（m）
	float fade() const;        // 画面全体を暗くする量（0〜1）
	bool isFadedOut() const;   // 死んで、真っ暗になり終わったか

private:
	int mFlashTicks; // 赤みが消えるまでの残り（ティック）
	int mPainTicks;  // 痛み（ぼかし・暗さ）が消えるまでの残り（ティック）
	int mDeadTicks;  // 死んでからのティック数（生きている間は 0）
	bool mDead;
};

#endif
