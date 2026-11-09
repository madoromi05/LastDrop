#ifndef INCLUDED_VIEW_EFFECTS_H
#define INCLUDED_VIEW_EFFECTS_H

#include "GameSrc/Player/Camera/AimPunch.h"
#include "GameSrc/Player/Camera/CameraShake.h"
#include "GameSrc/Player/Camera/AimSway.h"
#include "GameSrc/Weapon/Recoil/ViewRecoil.h"
#include "GameSrc/Weapon/Recoil/AimRecoil.h"

class FirstPersonCamera;

// 視点の手触り（撃った反動・揺れ・照準のぶれ）をまとめて進め、カメラへ渡す係
// （SummaryPlan/個別仕様/設計と分割/HighPrioritySplit_Plan.txt 6 章）。
//   狙いのずれ … 1 発ごとに跳ねて戻る反動（ViewRecoil）・連射でたまる弾道のずれ（AimRecoil）・攻撃された怯み（AimPunch）。カメラごと動き、弾の向きに入る
//   見える絵の揺れ … CameraShake（描く行列だけ）
//   照準のぶれ … AimSway（銃と弾の向きだけ。画面の中央は動かさない）
class ViewEffects{
public:
	// 揺れ方・ぶれ方を遊ぶたびに変える（起動からの時間を種にする）。
	ViewEffects();

	// マウス・スティックで回した角度（度）を足す（描画ごと）。照準のぶれ（振り向きの遅れ）が、次の update でまとめて使う。
	void addLook( float yawDegree, float pitchDegree );
	// 1ティック進めて、camera へ渡す（キャラを動かした後。flinch・shakeOnGrab の後）。
	// shotRecoil: このティックに1発撃ったなら、その銃の反動の大きさ（WeaponSpec::recoilScale。ライフルが 1）。撃っていなければ 0。
	// body: 自キャラの今の状態（PlayerAimSwaySource::read。自キャラがいなければ既定のまま＝ぶれない）。camera: nullptr 不可。
	void update( float shotRecoil, const AimSwayBody& body, FirstPersonCamera* camera );
	// 噛まれた（這うゾンビの腕が当たった）瞬間。狙いを跳ねさせ（弾の向きも動く）、絵を大きく揺らす。
	void flinch();
	// つかまれた瞬間。絵だけを揺らす（狙いは LookPull で相手の顔へ向けるので、跳ねさせない）。
	void shakeOnGrab();
	// 撃ったゾンビの部位が飛んだ・頭が弾けた瞬間。絵だけを少し揺らす（弾道は動かさない）。
	// headBurst: 頭が弾けた（部位が飛んだより少し大きく揺らす）。
	void shakeOnHeavyHit( bool headBurst );

private:
	ViewRecoil mRecoil;     // 1 発ごとに跳ねて戻る視点の反動（狙いへ足す）
	AimRecoil mAimRecoil;   // 連射でたまる弾道のずれ（狙いへ足す）
	AimPunch mAimPunch;     // 攻撃された瞬間の怯み（狙いへ足す）
	CameraShake mShake;     // 見える絵の揺れ（描く行列だけへ足す）
	AimSway mSway;          // 照準のぶれ（銃と弾の向きだけへ足す）
	// マウスが回した角度（度）のうち、まだぶれ（振り向きの遅れ）へ渡していない分。
	// マウスは描画ごと、ぶれはティックごとに進むので、ためておいて次のティックで使い切る。
	float mPendingLookYaw;
	float mPendingLookPitch;
};

#endif
