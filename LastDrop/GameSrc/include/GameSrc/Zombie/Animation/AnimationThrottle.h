#ifndef INCLUDED_ANIMATION_THROTTLE_H
#define INCLUDED_ANIMATION_THROTTLE_H

#include "GameSrc/Render/Culling/ViewFrustum.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix44.h"

class HitboxSet;
class TownVisibility;

// 前のフレームで絵を描いたカメラ（目の位置と映る範囲）。ゾンビがアニメを間引く段を決めるのに読む。
// 描く側（GameScene）が描いた後に GameWorld へ渡し、次のティックで ZombieBrainComponent が TickContext から読む。
// 描く側はゲームの状態を書き換えない（DrawCulling の約束）ので、ゾンビの方が自分で読みに来る形にしている。
struct AnimationView{
	// projectionView：カメラの射影×ビュー（TAA でずらす前）。eye：目の位置。
	// visibility：隠れたゾンビも「映らない」にする表（nullptr なら見ない。OptimizationC_Plan.txt Q5）。viewCell：目のいるます。
	AnimationView( const GameLib::Math::Matrix44& projectionView, const GameLib::Math::Vector3& eye, const TownVisibility* visibility,
		int viewCell ) :
		frustum( projectionView ), eye( eye ), visibility( visibility ), viewCell( viewCell ){}
	ViewFrustum frustum;
	GameLib::Math::Vector3 eye;
	const TownVisibility* visibility; // 借りているだけ
	int viewCell;
};

// ゾンビ 1 体の、アニメの姿勢を求めるティックを間引く係（SummaryPlan/個別仕様/最適化と計測/OptimizationB_Plan.txt 9 章）。
// 段（近い・遠い・映らない）を決め、「このティックは姿勢を求めるか」を答える。姿勢を求めないティックは、
// 持ち主が SkinnedMeshInstance::advance で時刻だけを進める（動きは遅れず、飛ぶだけ）。
// ViewFrustum は「ゲームの進行に使わない」約束だが、ここでは腕や脚の形が数ティック古くなるだけで、体の位置・状態の移り変わりは変わらない
// （部位の箱の形が最大で間隔−1 ティック古くなるのは、仕様書 11 章で承知の上）。
class AnimationThrottle{
public:
	// アニメの段。
	enum class Level{
		Near,   // 映っていて近い：毎ティック
		Far,    // 映っていて遠い：AnimationThrottleParams::farIntervalTicks ごと
		Hidden, // 映っていない：AnimationThrottleParams::hiddenIntervalTicks ごと
	};

	// phase：体ごとに違う数（湧いた順の番号など）。姿勢を求めるティックを体ごとにずらし、全員が同じティックに重ならないようにする。
	explicit AnimationThrottle( unsigned phase );

	// 段を決める。view：前のフレームのカメラ（nullptr ならまだ描いていないので「近い」＝間引かない）。
	// hitboxes：部位の箱（全部を囲む箱で映るかを見る。DrawCulling と同じ箱）。position：足元（目からの距離に使う）。
	static Level judge( const AnimationView* view, const HitboxSet& hitboxes, const GameLib::Math::Vector3& position );
	// 1 ティック進め、このティックに姿勢を求めるか。初めての呼び出しは必ず true（湧いた体が T ポーズで見えないように）。
	// mustPose：間引いてはいけない間（攻撃・根の動き・ひるみ・這う）なら true。
	bool shouldPose( Level level, bool mustPose );
	// 最後の shouldPose の段（F6 の Zombie info の表示に使う）。
	Level level() const;

private:
	unsigned mPhase;
	unsigned mTick;   // shouldPose を呼んだ回数
	bool mHasPosed;   // 一度でも姿勢を求めたか
	Level mLevel;
};

#endif
