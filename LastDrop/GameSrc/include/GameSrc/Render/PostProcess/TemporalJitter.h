#ifndef INCLUDED_TEMPORAL_JITTER_H
#define INCLUDED_TEMPORAL_JITTER_H

#include "GameLib/Math/Matrix44.h"

// TAA のための画面のずらし（SummaryPlan/個別仕様/グラフィック/Taa_Plan.txt 5 章・Q2）。毎フレーム、カメラを 1 画素より細かく少しずつずらす
// ずらす量は Halton 列（底 2 と 3）の 8 個を繰り返す（−0.5〜0.5 画素）。TAA が何フレームかの色を混ぜると、1 画素の中を細かく調べたのと同じになる
// 射影×ビューの行列に足すだけなので、カメラ（FirstPersonCamera）は知らない。GameScene が毎フレーム advance し、描く物すべてに apply した行列を渡す
class TemporalJitter{
public:
	TemporalJitter();

	void advance( bool enabled );																					// 次のフレームへ進む。enabled が false（TAA を使わない）なら、ずらしを 0 にして番号も 0 に止める（今と同じ絵）。1 フレームに 1 回
	GameLib::Math::Matrix44 apply( const GameLib::Math::Matrix44& projectionView, int width, int height ) const;	// projectionView に今のずらしを足した行列。width・height：描く先の画素数（ずらしは画素で決めるため）
	int frameIndex() const;																							// 今のフレームの番号（0〜kSampleNumber−1）。ノイズを毎フレーム動かすのにも使う

	static const int kSampleNumber = 8;

private:
	int mFrameIndex;
	float mOffsetX;
	float mOffsetY;
};

#endif
