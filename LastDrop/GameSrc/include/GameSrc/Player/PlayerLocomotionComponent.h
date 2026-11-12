#ifndef INCLUDED_PLAYER_LOCOMOTION_COMPONENT_H
#define INCLUDED_PLAYER_LOCOMOTION_COMPONENT_H

#include "GameLib/Math/Vector3.h"
#include "GameSrc/World/Component.h"
#include "GameSrc/Player/Animator.h"
#include "GameSrc/Player/PlayerCommand.h"
#include "GameSrc/Audio/Gait.h"

class SkinnedMeshData;
class SkinnedModelComponent;

// 自キャラの歩き方：静止／歩き／走り（素手と銃を持った姿の 6 つ）のアニメの切り替え（Animator）と、移動の速さ・向き
// 状態について知りたいこと（アニメ名・速さ・歩き方・武器を持っているか）は .cpp の表にまとめてある
// 状態を足すときは、Animator::State に1つ足し、表に1行足すだけでよい
// どの状態にするか（動いているか・走るか・銃を手に持っているか）は PlayerComponent が決めて渡す
// 速さは素手でも銃でも同じ（銃の有無で変わるのはアニメだけ）
class PlayerLocomotionComponent : public Component{
public:
	PlayerLocomotionComponent();	// アニメ（素手の "idle" / "walk" / "run"、銃の "rifle_idle" / "rifle_walk" / "rifle_run"）は、同じ物の SkinnedModelComponent の形から awake で引く

	static Animator::State chooseState( bool moving, bool running, bool drawn );	// 次の状態を選ぶ。武器を持っているか × 静止／歩き／走り の組み合わせを表から探す
	static float speedOf( Animator::State state );									// 状態の速さ（m/フレーム）
	static bool isCarryState( Animator::State state );								// 銃を持って走る状態か（胸元へ抱える）

	// 操作の前後・横の量から移動方向（水平・長さ1）を求める。移動の操作が無ければ false
	// cameraYaw: カメラの水平の向き（度）。W がこの向きへの前進になる
	bool readMoveDirection( const PlayerCommand& command, float cameraYaw, GameLib::Math::Vector3* direction ) const;
	void setMoveDirection( const GameLib::Math::Vector3& direction );	// 進む向き（水平・長さ1）を変える。入力を離した後の減速中も、最後の向きへ進む
	void approachSpeed( float targetSpeed );							// 移動の速さを targetSpeed へ、1フレームの加速・減速の上限まで近づける
	GameLib::Math::Vector3 step() const;								// このティックに進む量（向き × 速さ）
	void changeState( Animator::State next );							// アニメの状態を切り替える（同じ状態なら何もしない）。前のポーズからクロスフェードでつなぐ

	Gait gait() const;			// 今の歩き方（アニメの状態から。素手か銃かは含まない）
	float gaitPhase() const;	// 歩き・走りのアニメが1周の何割まで進んだか（0〜1。見せたポーズの位相）。足音を足の動きに合わせるのに使う
	float walkRate() const;		// 今の速さ ÷ 歩きの速さ（照準のぶれの歩きの強さ。AimSway が 1 で止める）

	// Component（SkinnedModelComponent が要る。HoldComponent は任意＝付いていれば、銃を持った姿で始める）
	// awake：状態にアニメを登録する。start：最初のアニメを付ける
	void awake() override;
	void start() override;

private:
	Animator mAnimator;
	SkinnedModelComponent* mModel;
	GameLib::Math::Vector3 mMoveDirection; // 最後に進んだ向き（水平・長さ1）。入力を離した後の減速中もこの向きへ進む
	float mSpeed;                          // 今の移動の速さ（m/フレーム）。目標の速さへ加速・減速して近づく
};

#endif
