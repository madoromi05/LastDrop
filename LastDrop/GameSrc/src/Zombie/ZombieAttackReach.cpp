#include "GameSrc/Zombie/ZombieAttackReach.h"
#include "GameSrc/Core/GameTime.h"
#include "GameSrc/Math/AngleMath.h"
#include "GameSrc/Math/FlatMath.h"

#include "GameLib/Math/Functions.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// ── 立っている体（SummaryPlan/個別仕様/ZombieAttack_Plan.txt の 3 章）──
	// 攻撃を始める間合い（m。足元どうし、水平だけ）。追いついて止まる距離（ZombieChase の kStopDistance = 1.2m）より遠くする。
	// 自キャラの移動の当たり判定は、銃を持っている間は銃の先まで前へ伸びるので、正面から来たゾンビは 1.2m より手前で
	// 銃の箱にぶつかって止まり、1.2m の内へ入れない（2026-10-21 ユーザー報告「攻撃してこない」）。
	// 止まった後に歩き出す距離（kResumeDistance = 1.5m）より少し遠くして、ぶつかって止まったゾンビも攻撃を始めるようにする。
	const float kAttackStartDistance = 1.6f;
	// 攻撃を始める向きの差（度。左右それぞれ）。相手の方を向き終わってから腕を振る。
	const float kAttackStartHalfAngle = 30.f;
	// 腕を振り下ろしたときに、当たる距離（m）と向きの差（度）。振っている間に下がられたら空振りにする。
	// 距離は始める間合いと同じ（銃の箱にぶつかって寄れなかったときも、手は銃・腕に届いている）。
	const float kHitReachDistance = kAttackStartDistance;
	const float kHitHalfAngle = 45.f;
	// 腕を振っている間に寄る距離（m）。手は足元から 0.86m 前へ出るので、止まる距離（1.2m）のままだと相手に届かない。
	const float kHitCloseDistance = 0.9f;
	// 寄る速さ（m/ティック）。0.3m を 0.5 秒で（振り下ろす約 0.47 秒の間にほぼ寄り終える）。
	const float kReachApproachSpeed = 0.6f / GameTime::kTicksPerSecond;
	// ── 脚を失って体を引きずる体（SummaryPlan/個別仕様/ZombieCrawl_Plan.txt）──
	// 攻撃を始める間合い（m。頭から相手の足元まで、水平だけ）と、腕を伸ばし切ったときに当たる距離。
	// 追いついて止まる距離（胸から 1.2m）で、頭は相手の足元の手前 0.7m 辺りに来るので、それより少し遠くする。
	const float kCrawlAttackStartDistance = 0.9f;
	const float kCrawlHitReachDistance = 1.0f;

	// 体の中心から target までの向きの差（度。-180〜180）。真上は向きが無いので 0（向いているとみなす）。
	float headingDifference( const ZombieAttackReach::Origin& origin, const Vector3& target ) {
		Vector3 toTarget;
		toTarget.setSub( target, origin.center );
		if ( toTarget.x == 0.f && toTarget.z == 0.f ) {
			return 0.f;
		}
		return AngleMath::difference( AngleMath::headingOf( toTarget ), origin.angleY );
	}

	// distance 以内・halfAngle 以内に相手が居るか。寝ている体は足元から前へ 1.5m ほど伸びているので、距離は reachFrom（頭）から測る。
	bool isWithin( const ZombieAttackReach::Origin& origin, const Vector3& target, float distance, float halfAngle ) {
		return FlatMath::distance( origin.reachFrom, target ) <= distance &&
			Math::abs( headingDifference( origin, target ) ) <= halfAngle;
	}
}

bool ZombieAttackReach::canStart( const Origin& origin, const Vector3& target ) {
	float distance = origin.crawling ? kCrawlAttackStartDistance : kAttackStartDistance;
	return isWithin( origin, target, distance, kAttackStartHalfAngle );
}

bool ZombieAttackReach::canHit( const Origin& origin, const Vector3& target ) {
	float distance = origin.crawling ? kCrawlHitReachDistance : kHitReachDistance;
	return isWithin( origin, target, distance, kHitHalfAngle );
}

float ZombieAttackReach::approachStep( const Vector3& center, const Vector3& target ) {
	float remaining = FlatMath::distance( center, target ) - kHitCloseDistance;
	if ( remaining <= 0.f ) {
		return 0.f;
	}
	return Math::min( kReachApproachSpeed, remaining );
}
