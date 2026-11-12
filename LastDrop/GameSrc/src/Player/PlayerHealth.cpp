#include "GameSrc/Player/PlayerHealth.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Functions.h"

using namespace GameLib;

namespace{
	const float kMaxHealth = 100.f;	// 最大の体力。ゾンビの攻撃 1 回が 20（ZombieAttack）なので、満タンから 5 回で死ぬ

	// 状態の境目（残りの割合）
	// これ以上なら Fine、Caution の境目以上なら Caution、それ未満は Danger
	const float kFineRatio = 0.67f;
	const float kCautionRatio = 0.34f;

	// 状態ごとの歩く・走る速さの倍率
	const float kFineSpeedScale = 1.f;
	const float kCautionSpeedScale = 0.85f;
	const float kDangerSpeedScale = 0.70f;
}

const char* playerConditionName( PlayerCondition condition ) {
	switch ( condition ){
		case PlayerCondition::Fine:    return "Fine";
		case PlayerCondition::Caution: return "Caution";
		case PlayerCondition::Danger:  return "Danger";
	}
	return "-";
}

PlayerHealth::PlayerHealth() :
	mCurrent( kMaxHealth ) {
}

void PlayerHealth::takeDamage( float amount ) {
	if ( isDead() ) {
		return;
	}
	mCurrent = Math::max( 0.f, mCurrent - amount );
}

void PlayerHealth::heal( float amount ) {
	if ( isDead() ) {
		return;
	}
	mCurrent = Math::min( kMaxHealth, mCurrent + amount );
}

bool PlayerHealth::isInjured() const {
	return !isDead() && mCurrent < kMaxHealth;
}

bool PlayerHealth::isDead() const {
	return mCurrent <= 0.f;
}

float PlayerHealth::current() const {
	return mCurrent;
}

float PlayerHealth::maxHealth() const {
	return kMaxHealth;
}

PlayerCondition PlayerHealth::condition() const {
	float ratio = mCurrent / kMaxHealth;
	if ( ratio >= kFineRatio ) {
		return PlayerCondition::Fine;
	}
	return ( ratio >= kCautionRatio ) ? PlayerCondition::Caution : PlayerCondition::Danger;
}

float PlayerHealth::walkSpeedScale() const {
	switch ( condition() ){
		case PlayerCondition::Fine:    return kFineSpeedScale;
		case PlayerCondition::Caution: return kCautionSpeedScale;
		case PlayerCondition::Danger:  return kDangerSpeedScale;
	}
	return kFineSpeedScale;
}
