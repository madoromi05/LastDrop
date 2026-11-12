#ifndef INCLUDED_SCALAR_MATH_H
#define INCLUDED_SCALAR_MATH_H

// 1つの数の、よく使う計算の、ただ1か所。状態は持たない
// GameLib::Math に無い物だけを置く（min・max・abs・sqrt などは GameLib のまま使う）
class ScalarMath{
public:
	static float lerp( float a, float b, float t );				// a から b へ t の割合だけ進んだ値（a + ( b - a ) * t）。t は 0〜1 に限らない（外なら外へ伸ばす）
	static float clamp( float value, float low, float high );	// value を low〜high に収める。low <= high であること
	static float clampSigned( float value, float limit );		// value を -limit〜limit に収める。limit は 0 以上
	// 始めと終わりがゆっくりになる曲線（シェーダーの smoothstep と同じ t * t * ( 3 - 2t )）
	// t は 0〜1 で渡す（外の値は丸めない）。曲線にする理由は、使う所ごとに違うので呼ぶ側に書く
	static float smoothStep( float t );
	static float easeOut( float t );							// 始めが速く、だんだんゆっくりになる曲線（1 - ( 1 - t )²）。t は 0〜1 で渡す（外の値は丸めない）
	static float wrapUnit( float phase );						// 1 周を 1 とする輪の上の位置（歩きの位相など）を [0, 1) へ折り返す。何周ずれていても1回で計算する
	// value に remain（1 ティックごとに残る割合）を掛け、絶対値が minimum より小さくなったら 0 にして返す
	// 0 にするのは、いつまでも小さな値で動かし続けないため。minimum は量ごとに単位が違うので、呼ぶ側が決める
	static float decay( float value, float remain, float minimum );
};

#endif
