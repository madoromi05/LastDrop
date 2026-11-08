#ifndef INCLUDED_ANIMATION_THROTTLE_PARAMS_H
#define INCLUDED_ANIMATION_THROTTLE_PARAMS_H

// ゾンビのアニメを間引く段の値（SummaryPlan/個別仕様/OptimizationB_Plan.txt 9 章・Q8）。起動中に F6 のエディタで動かせる。
// 間隔はティックの数。F6 の欄は小数しか扱わないので小数で持ち、使う所で四捨五入する（1 より小さければ 1）。
struct AnimationThrottleParams{
	// 前のフレームで目に映っていて、目からこれより遠い体は「遠い」段（m）。霧で輪郭がぼやけ始めるくらい。
	float farDistance = 20.f;
	// 「遠い」段・「映らない」段で、アニメの姿勢を求める間隔（ティック）。
	float farIntervalTicks = 2.f;
	float hiddenIntervalTicks = 4.f;

	// 項目ごとに v( 名前, 値, 下限, 上限, 単位 ) を呼ぶ（WeaponPoseParams と同じ約束）。間隔の下限を 1 にするのは 0 で割らないため。
	template< class V > void visit( V& v ) {
		v( "farDistance", farDistance, 0.f, 100.f, "m" );
		v( "farIntervalTicks", farIntervalTicks, 1.f, 8.f, "tick" );
		v( "hiddenIntervalTicks", hiddenIntervalTicks, 1.f, 8.f, "tick" );
	}
};

// 起動中ずっと 1 つだけの調整値（AnimationThrottle.cpp が持つ）。
AnimationThrottleParams& animationThrottleParams();

#endif
