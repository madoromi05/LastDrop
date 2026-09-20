#include "Animator.h"

Animator::Animator() :
	mState( STATE_IDLE ) {
	// mAnimations は Scene::Animation のデフォルト構築で空ハンドルになる。
	// 各 State のアニメは setAnimation() で後から登録する。
}

void Animator::setAnimation( State state, GameLib::Scene::Animation animation ) {
	mAnimations[ state ] = animation;
}

bool Animator::request( State next ) {
	if ( !canTransit( mState, next ) ) {
		return false;
	}
	mState = next;
	return true;
}

Animator::State Animator::state() const {
	return mState;
}

GameLib::Scene::Animation Animator::animation() const {
	return mAnimations[ mState ];
}

bool Animator::canTransit( State from, State to ) const {
	// 同じ状態への遷移は無視する（無駄な setAnimation を防ぐ）。
	if ( from == to ) {
		return false;
	}
	// 死亡は終端。一度入ったら他状態へ抜けない（死亡後に歩き出さない）。
	if ( from == STATE_DEATH ) {
		return false;
	}
	// それ以外の遷移可否（ジャンプ中は歩きに割り込ませない等のゲーム的タイミング）は、
	// 状態を最もよく知る呼び出し側が request() を呼ぶ/呼ばないで判断する。
	return true;
}
