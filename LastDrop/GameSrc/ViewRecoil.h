#ifndef INCLUDED_VIEW_RECOIL_H
#define INCLUDED_VIEW_RECOIL_H

// 撃った反動による視点の跳ね上がりの量を決める。撃ったティックに少し上へ跳ね、その後は時間で元へ戻る。
// 連射でも次の1発までに戻りきるので、1発ごとに「跳ねる → 戻る」を繰り返す（積み上がらない）。
// 決めるのは「視点の上下へ足す角度」だけ。カメラへ足すのは持ち主（FirstPersonControl）。
// マウスで決めた向きそのものは変えず、上に足すだけなので、撃ちやめれば狙いは元の所へ戻る。
// 銃の見た目の反動（WeaponRecoil）とは別の量（こちらは照準と弾の向きが動く。向こうは銃が跳ねて見えるだけ）。
// 跳ねる大きさ・戻る速さを変えるときは ViewRecoil.cpp の定数だけを直す。
class ViewRecoil{
public:
	ViewRecoil();

	// 1ティック進める。fired: このティックに1発撃ったか。
	void update( bool fired );
	// 視点の上下へ足す角度（度。FirstPersonCamera のピッチと同じ約束で、負が見上げる向き。反動が無ければ 0）。
	float pitchOffset() const;

private:
	float mRise; // 跳ね上がっている角度（度。0 以上）
};

#endif
