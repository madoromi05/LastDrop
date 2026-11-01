#ifndef INCLUDED_CASCADE_REFRESH_H
#define INCLUDED_CASCADE_REFRESH_H

#include "GameLib/Math/Vector3.h"

// 月の影（CascadedShadowMap）の 1 枚を、このフレームに描き直すかを決める（OptimizationA_Plan.txt 7 章。Q4）。
// 決まった間隔（interval フレームに 1 回）で描き直し、間のフレームは前に描いた影と行列をそのまま使う。
// ただし前に描いた時から、カメラの向きが大きく変わった・目が大きく動いたときは、間隔を待たずにすぐ描き直す
// （急に振り向くと、前に描いた範囲の外が映り、影が欠けるため）。
// 行列を作る・影を描くのは持ち主（CascadedShadowMap・GameScene）。ここは「描き直すか」を答えるだけ。
class CascadeRefresh{
public:
	CascadeRefresh();

	// このフレームに描き直すかを決めて覚える（isDue で読む）。1 フレームに 1 回呼ぶ。
	// frame：何フレーム目か（持ち主が数える）。interval：何フレームに 1 回描くか（1 以上）。phase：描くフレームをずらす数
	// （(frame + phase) が interval で割り切れるフレームに描く。枚ごとにずらして、同じフレームに重ならないようにする）。
	// eye / viewDirection：今のカメラの目と向き（長さ 1）。
	// redrawCos：前に描いた時の向きとの角度の cos がこれより小さい（＝角度が大きい）と、すぐ描く。
	// redrawDistance：前に描いた時から目がこれ以上（m）動いたら、すぐ描く。force：必ず描く（影の範囲の決め方が変わった・前のフレームに影が無い）。
	void decide( unsigned frame, int interval, int phase, const GameLib::Math::Vector3& eye, const GameLib::Math::Vector3& viewDirection,
		float redrawCos, float redrawDistance, bool force );
	// 最後の decide で、描き直すと決めたか。
	bool isDue() const;

private:
	bool mDue;
	bool mDrawn;                       // 一度でも描いたか（描くまでは毎フレーム描き直す）
	GameLib::Math::Vector3 mEye;       // 最後に描いた時の目
	GameLib::Math::Vector3 mView;      // 最後に描いた時の向き
};

#endif
