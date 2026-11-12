#ifndef INCLUDED_ANIMATOR_H
#define INCLUDED_ANIMATOR_H

#include "GameLib/Scene/Animation.h"

// キャラクターのアニメーション状態遷移を司るクラス（状態遷移マシン単体）
// 「今どの状態か」「状態→アニメの対応」「遷移してよいか」だけを持つ
// 時刻更新(updateAnimation)や描画は Scene::Tree の責務とし、ここには持たない（単一責任）
// Scene::Animation は Container から借用するだけで所有しない
class Animator{
public:
	enum State{
		STATE_IDLE,			// 静止（素手）
		STATE_WALK,			// 歩き（素手）
		STATE_RUN,			// 走り（素手）
		// 銃を持っている間（拳銃・ライフル共通。アニメの名前は素材のまま "rifle_idle" など。腕は銃ごとに HoldComponent が差し替える）
		STATE_ARMED_IDLE,	// 銃を持って静止（覗く間もこのアニメのまま、腕と銃を FirstPersonBody で寄せる）
		STATE_ARMED_WALK,	// 銃を持って歩き
		STATE_ARMED_RUN,	// 銃を持って走り
		STATE_NUM,		// 状態数。配列サイズや範囲判定に使う（要素数をマジックナンバーにしないため）
	};

	Animator();
	// State に対応するアニメを登録する（借用。所有しない）
	void setAnimation( State, GameLib::Scene::Animation );
	// 指定 State への遷移を要求する。実際に遷移したら true を返す
	// true は「呼び出し側が Tree::setAnimation( animation() ) を呼ぶべき合図」
	bool request( State next );
	// 現在の State を取得する
	State state() const;
	// 現在の State に対応するアニメを取得する（Tree に渡す用）
	GameLib::Scene::Animation animation() const;

private:
	// from から to へ遷移してよいかを判定する
	bool canTransit( State from, State to ) const;

	State mState;
	GameLib::Scene::Animation mAnimations[ STATE_NUM ];
};

#endif
