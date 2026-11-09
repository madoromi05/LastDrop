#ifndef INCLUDED_ZOMBIE_HEARING_H
#define INCLUDED_ZOMBIE_HEARING_H

#include "GameLib/Math/Vector3.h"

class GameObjectWorld;
struct DayConditionEffect;

// 音を、届く距離の内のゾンビ全員へ知らせる係（SummaryPlan/個別仕様/設計と分割/HighPrioritySplit_Plan.txt 3-3）。
// 何体ものゾンビをまとめて扱うので部品にはしない。持ち主は ZombieDirector で、共有の棚（provide / shared）に置く。
// 音を立てる側（自キャラの PlayerNoiseComponent・脱出の車のエンジン）は、ここへ「どこで鳴ったか」を言うだけ。
// 聞くかどうか（倒れている・よろけている等）はゾンビ（ZombieBrainComponent）が決める。
// 届く距離のうち、世界の決まり（銃声の距離・今日の条件の倍率）はここが持ち、歩き方ごとの足音の距離は音を立てる側が持つ。
class ZombieHearing{
public:
	// objects: ゾンビを探す物の並び。day: 今日の条件（足音の届く距離の倍率）。
	// どちらも所有権は受け取らない。nullptr 不可。この係より長く生きていること。
	ZombieHearing( GameObjectWorld* objects, const DayConditionEffect* day );
	ZombieHearing( const ZombieHearing& ) = delete;
	ZombieHearing& operator=( const ZombieHearing& ) = delete;

	// position（撃った瞬間の自キャラの足元）で銃声がした。水平に 30m の内のゾンビが、その場所へ寄ってくる。
	void hearGunshot( const GameLib::Math::Vector3& position );
	// 物音（脱出の車のエンジンなど）がした。届く距離・寄って来方は銃声と同じ。
	void hearNoise( const GameLib::Math::Vector3& position );
	// feet（自キャラの足元）で足音がした。range（m。歩き方で決まる距離）に今日の条件の倍率を掛けた距離の内のゾンビが気付く。
	// 掛けた後が 0 以下なら誰も聞かない。
	void hearFootsteps( const GameLib::Math::Vector3& feet, float range );

private:
	GameObjectWorld* mObjects;       // 借りているだけ
	const DayConditionEffect* mDay;  // 借りているだけ
};

#endif
