#include "GameSrc/Hit/HitWorld.h"
#include "GameSrc/Hit/IHittable.h"
#include "GameSrc/Hit/IRayBlocker.h"

#include "GameLib/GameLib.h"

#include <algorithm>

using namespace GameLib::Math;

void HitWorld::addTarget( const IHittable* target ) {
	ASSERT( target && "HitWorld : target is null." );
	mTargets.push_back( target );
}

void HitWorld::addBlocker( const IRayBlocker* blocker ) {
	ASSERT( blocker && "HitWorld : blocker is null." );
	mBlockers.push_back( blocker );
}

void HitWorld::removeTarget( const IHittable* target ) {
	mTargets.erase( std::remove( mTargets.begin(), mTargets.end(), target ), mTargets.end() );
}

void HitWorld::removeBlocker( const IRayBlocker* blocker ) {
	mBlockers.erase( std::remove( mBlockers.begin(), mBlockers.end(), blocker ), mBlockers.end() );
}

bool HitWorld::raycast( const Vector3& origin, const Vector3& delta, const IHittable* ignore, Hit* hit ) const {
	bool found = false;
	for ( const IHittable* target : mTargets ) {
		if ( target == ignore ) {
			continue;
		}
		HitResult candidate;
		if ( !target->hitboxes().raycast( origin, delta, &candidate ) ) {
			continue;
		}
		if ( found && candidate.time >= hit->time ) {
			continue;
		}
		hit->target = target;
		hit->blocker = nullptr;
		hit->part = candidate.part;
		hit->time = candidate.time;
		hit->point = candidate.point;
		hit->normal = candidate.normal;
		found = true;
	}
	return findNearerBlocker( origin, delta, found, hit );
}

bool HitWorld::raycastBlockers( const Vector3& origin, const Vector3& delta, Hit* hit ) const {
	return findNearerBlocker( origin, delta, false, hit );
}

bool HitWorld::findNearerBlocker( const Vector3& origin, const Vector3& delta, bool found, Hit* hit ) const {
	for ( const IRayBlocker* blocker : mBlockers ) {
		float time;
		Vector3 normal;
		if ( !blocker->raycast( origin, delta, &time, &normal ) ) {
			continue;
		}
		if ( found && time >= hit->time ) {
			continue;
		}
		hit->target = nullptr;
		hit->blocker = blocker;
		hit->part = BodyPart::Count; // 部位ではない
		hit->time = time;
		hit->point.setMadd( origin, delta, time );
		hit->normal = normal;
		found = true;
	}
	return found;
}

bool HitWorld::isBlocked( const Vector3& origin, const Vector3& delta ) const {
	// どれか1つに当たれば足りるので、一番手前を探さずに見つけた所で止める。
	for ( const IRayBlocker* blocker : mBlockers ) {
		float time;
		Vector3 normal;
		if ( blocker->raycast( origin, delta, &time, &normal ) ) {
			return true;
		}
	}
	return false;
}

const std::vector< const IHittable* >& HitWorld::targets() const {
	return mTargets;
}

const std::vector< const IRayBlocker* >& HitWorld::blockers() const {
	return mBlockers;
}
