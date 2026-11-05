#include "GameSrc/Zombie/ZombieDamageReaction.h"

ZombieDamageReaction::Fall ZombieDamageReaction::chooseFall( const Facts& facts ) {
	if ( !facts.canFall ) {
		return Fall::None;
	}
	if ( facts.isDead ) {
		return Fall::Die; // 倒れる 1 発で部位も 0 になったら、切れてから倒れる（切るのは持ち主が先に済ませている）
	}
	if ( facts.isStanding && facts.hasLostFooting ) {
		return Fall::Down;
	}
	return Fall::None;
}

ZombieDamageReaction::Hit ZombieDamageReaction::chooseHit( const Facts& facts, bool shakenOff ) {
	if ( shakenOff ) {
		return facts.isStanding ? Hit::ShakeOff : Hit::None;
	}
	if ( facts.isStanding && !facts.isAttacking && facts.canStagger ) {
		return Hit::Stagger;
	}
	return Hit::None;
}
