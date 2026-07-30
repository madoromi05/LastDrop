#ifndef INCLUDED_ARENA_H
#define INCLUDED_ARENA_H

#include "GameLib/Math/Vector3.h"
using namespace GameLib::Math;

// ────────────────────────────────────────────────
// Arena：収容室（アリーナ）の当たり形状を担う。
//
// 見た目は既存の Stage（床モデル）が描画する。ここは「衝突・アンカー判定」だけを持つ。
// 形状は床(y=0)・4枚の壁・天井からなる軸並行の箱として解析的に扱う（メッシュは参照しない）。
// これにより、ワイヤーの地形アンカー判定を重いメッシュレイキャストなしで実現する。
//
// 単一責任：Arena は「幾何問い合わせ」のみ。移動やゲーム進行は Player 側が持つ。
// ────────────────────────────────────────────────
class Arena{
public:
	// 収容室の広さ。床モデル stage.txt の ±50 に合わせる。
	static const float mHalfExtent; // 中心から壁までの水平距離。
	static const float mWallHeight; // 床から天井までの高さ。

	// 位置を床上・壁の内側へ押し戻す（プレイヤーがはみ出さないようにする）。
	// 戻り値：床に接地したら true。
	bool clampInside( Vector3* position, float radius ) const;

	// 半直線 origin + dir*t（dir は正規化済み前提）と、床・壁・天井の最も手前の交点を求める。
	// 交点が範囲内かつ 0 < t <= maxDistance のときだけ *outHit に格納して true を返す。
	// ＝ワイヤーを撃った先の地形アンカー点を求めるための判定。
	bool raycast(
		const Vector3& origin,
		const Vector3& dir,
		float maxDistance,
		Vector3* outHit ) const;
};

#endif
