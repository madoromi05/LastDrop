#ifndef INCLUDED_ZOMBIE_KNOCKBACK_H
#define INCLUDED_ZOMBIE_KNOCKBACK_H

#include "GameLib/Math/Vector3.h"

// よろけたときに体を押し下げる量を、ティックごとに配る（SummaryPlan/個別仕様/ゾンビ/ZombieStagger_Plan.txt の 9 章）
// 距離と長さは ZombieStaggerParams（始めたときの値を使う）。初めが速く、だんだん遅くなる（弾を受けた瞬間に一番押される）
// 量を決めるだけで、体は動かさない（壁・車に当たりながら足元を動かすのは ZombieHealthComponent）
class ZombieKnockback{
public:
	ZombieKnockback();

	// direction の向き（ワールド。水平にして使う。長さは問わない）へ押し始める。押している途中なら、新しい向きで始め直す
	// 水平の長さが 0（真上・真下から撃たれた）なら押さない
	void start( const GameLib::Math::Vector3& direction );
	void cancel();	// 押すのをやめる（倒れ始めた）
	// このティックに押す量（ワールド・水平）を返して 1 ティック進める。押していなければ 0
	GameLib::Math::Vector3 step();
	bool isActive() const;

private:
	GameLib::Math::Vector3 mDirection; // 押す向き（水平の単位ベクトル）
	float mDistance;                   // 押す距離の合計（m）
	int mTotalTicks;                   // 押し終えるまでの長さ（ティック）
	int mElapsedTicks;                 // 押し始めてからのティック。mTotalTicks に届いたら押していない
};

#endif
