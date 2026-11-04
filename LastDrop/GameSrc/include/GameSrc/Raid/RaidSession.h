#ifndef INCLUDED_RAID_SESSION_H
#define INCLUDED_RAID_SESSION_H

#include "GameSrc/Raid/RaidClock.h"
#include "GameSrc/Raid/ExtractionTracker.h"
#include "GameSrc/Raid/DayResult.h"
#include "GameSrc/Map/TownMap.h"
#include "GameLib/Math/Vector3.h"
#include <vector>

// 町の中の 1 日（1 回の出撃）。残り時間（RaidClock）と脱出点（ExtractionTracker）を持ち、「今終わったか・どう終わったか」を答える
// （R2_Plan.txt 5 章）。開く脱出点は作るときに 4 つから 2 つを選ぶ（Q10）。倒した数もここで数える。
// 物・カバン・保存には触らない（終わった後に何を残すかは RunRules。シーンは毎ティック tick を呼び、終わったら次のシーンへ移るだけ）。
class RaidSession{
public:
	// extractPoints: 脱出点の候補（TownMap::extractPoints。名前の文字列は TownMap の表を借りるので、その表より短く生きること）。
	// seed: どの脱出点を開くかの乱数の種（毎日変わるよう、呼ぶ側が起動からの時間などを渡す）。
	RaidSession( const std::vector< TownExtractPoint >& extractPoints, unsigned seed );

	// 1 ティック進める（一時停止・カバンの間は呼ばない）。終わった後は何もしない。
	// feet: 自キャラの足元。playerDead: 自キャラが死んだか。kills: このティックに倒した数。
	// 終わり方の順：死んだ → 脱出した → 時間切れ（同じティックに重なったら、先の物）。
	void tick( const GameLib::Math::Vector3& feet, bool playerDead, int kills );

	RaidEnding ending() const;            // まだなら RaidEnding::None
	const RaidClock& clock() const;
	const ExtractionTracker& extraction() const;
	int kills() const;                     // この日に倒した数
	// 終わった日の結果（持ち帰った物は入れない。カバンを知っているのはシーンなので、シーンが足す）。
	DayResult result() const;

	// ── F6（確認用。Debug・Profile だけで呼ぶ）──
	void setRemainingSeconds( float seconds );
	void forceExtract();

private:
	static std::vector< ExtractZone > pickOpenZones( const std::vector< TownExtractPoint >& points, unsigned seed );

	RaidClock mClock;
	ExtractionTracker mExtraction;
	int mKills;
	RaidEnding mEnding;
};

#endif
