#ifndef INCLUDED_ANIMATION_PLAYER_H
#define INCLUDED_ANIMATION_PLAYER_H

#include "GameLib/Math/Matrix34.h"
#include "GameLib/Scene/Tree.h"
#include "GameLib/Scene/Animation.h"
#include <vector>

class SkinnedMeshData;

// 1 体ぶんのアニメの再生（SummaryPlan/個別仕様/設計と分割/MidPrioritySplit_Plan.txt 6 章）。
// 自分の骨格を持ち、時刻を進めて、骨格の根から見たボーンの姿勢（ポーズ）を求める。書き換え・置き場所・描画は知らない（SkinnedMeshInstance）。
// アニメの切り替えは2通り。setAnimation は即座に切り替え、crossFadeTo は切り替え直前に見えていたポーズから
// 新しいアニメへ数フレームかけて混ぜる（クロスフェード）。混ぜるのはボーンごとの姿勢行列（MatrixBlend）。
// コピーすると骨格を2体で共有してしまうので、コピーは禁止。
class AnimationPlayer{
public:
	// アニメの再生のしかた。
	enum PlayMode{
		PLAY_LOOP, // 最後まで来たら始めへ戻る（歩き・待機）
		PLAY_ONCE, // 最後のフレームで止まる（倒れる）
	};

	// data: 骨格とアニメを借りる。所有権は受け取らない。nullptr 不可、isReady() 済みであること。この再生より長く生きていること。
	explicit AnimationPlayer( const SkinnedMeshData* data );
	AnimationPlayer( const AnimationPlayer& ) = delete;
	AnimationPlayer& operator=( const AnimationPlayer& ) = delete;

	// 説明は SkinnedMeshInstance の同じ名前の関数。
	void setAnimation( const char* name, float startTime, PlayMode mode );
	void setAnimation( GameLib::Scene::Animation animation, float startTime, PlayMode mode );
	void crossFadeTo( GameLib::Scene::Animation animation, float startTime, float fadeFrames, PlayMode mode );
	void setPlaybackRate( float rate );
	float animationTime() const;
	float animationPhase() const;
	bool isAnimationFinished() const;

	// アニメを 1 ティック進めて、ポーズを求める（フェード中は切り替え前のポーズと混ぜる）。
	void update();
	// 時刻とクロスフェードの進みだけを 1 ティック進め、ポーズは求めない（AnimationThrottle で間引くティック）。
	void advance();
	// ポーズを一度でも求めたか（まだ何も見せていないならフェードしない・描かない）。
	bool hasPose() const;
	// 最後の update で求めたポーズ（骨格の根から見たボーンの姿勢。並びは SkinnedMeshData の骨の番号。フェード中は混ぜた後）。
	const std::vector< GameLib::Math::Matrix34 >& pose() const;

private:
	// アニメの時刻とクロスフェードの進みを 1 ティック進め、このティックに見せる時刻を返す（update と advance の共通の所）。
	float advanceClock();
	// フェード中なら、切り替え前のポーズと今のポーズを混ぜて mBonePose を上書きする（フェードの進みは advanceClock が数える）。
	void blendFadingPose();
	// PLAY_ONCE で止める時刻（フレーム）。GameLib のアニメは最後のキーの時刻ちょうどで始めへ折り返すので、その少し手前。
	float lastFrameTime() const;

	const SkinnedMeshData* mData;
	GameLib::Scene::Tree mSkeleton;     // この体だけのポーズ。根は原点・向き0のまま（置き場所は描くときに掛ける）
	GameLib::Scene::Animation mAnimation; // 再生中のアニメ（Tree は持っているアニメを教えてくれないので自分で覚える）
	float mTime;                        // 次の update で使うアニメ時刻（フレーム）
	// 今見えているアニメ時刻（最後の update で使った時刻）。再生の速さが途中で変わっても、見えている時刻を正しく答えるために分けて持つ。
	float mShownTime;
	float mPlaybackRate;                // 1 回の update で進めるアニメ時刻（フレーム）
	PlayMode mPlayMode;                 // 再生中のアニメの再生のしかた
	// ボーンの姿勢は、骨格の根から見た行列で持つ（置き場所・向きを含まない）。
	// クロスフェード中にキャラが移動・旋回しても、切り替え前のポーズが元の場所に取り残されないようにするため。
	std::vector< GameLib::Math::Matrix34 > mBonePose;     // 今フレームに見せるポーズ（フェード中は混ぜた後）
	std::vector< GameLib::Math::Matrix34 > mFadeFromPose; // 切り替え直前に見えていたポーズ（フェード中だけ使う）
	float mFadeFrames;                  // フェードの長さ（フレーム）。0 ならフェードしていない
	float mFadeElapsed;                 // フェード開始からの経過（フレーム）
	bool mHasPose;                      // mBonePose を一度でも計算したか
};

#endif
