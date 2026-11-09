#ifndef INCLUDED_RAID_SESSION_H
#define INCLUDED_RAID_SESSION_H

#include "GameSrc/Raid/RaidClock.h"
#include "GameSrc/Raid/ExtractionTracker.h"
#include "GameSrc/Raid/RaidResult.h"
#include "GameSrc/Map/TownMap.h"
#include "GameLib/Math/Vector3.h"
#include <vector>

// 町の中の 1 回の出撃。経った時間（RaidClock）と脱出の車のそば（ExtractionTracker）を持ち、「今終わったか・どう終わったか」を答える
// （R2_Plan.txt 5 章）。脱出の車を置く所は、作るときに候補（北・南）から 1 つ選ぶ。
// 車へ要る数のガソリンを入れ終えたら（openEscape）、車のそばに居続けて脱出できる。倒した数もここで数える。
// 物・カバンには触らない（シーンは毎ティック tick を呼び、終わったら結果を渡して次のシーンへ移るだけ）。
class RaidSession{
public:
	// carSpots: 脱出の車を置く候補（TownMap::escapeCarSpots。名前の文字列は TownMap の表を借りるので、その表より短く生きること）。
	// seed: どこに置くかの乱数の種（出撃のたびに変わるよう、呼ぶ側が起動からの時間などを渡す）。
	RaidSession( const std::vector< TownEscapeCarSpot >& carSpots, unsigned seed );

	// この出撃の脱出の車を置く所（carSpots の番号）。町を組み立てる側が、ここに車を置く。
	int escapeCarSpot() const;
	// 脱出の車へ要る数のガソリンを入れ終えた。ここから車のそばに居続けたら脱出（2 回目からは何もしない）。
	void openEscape();
	// 1 ティック進める（一時停止・カバンの間は呼ばない）。終わった後は何もしない。
	// feet: 自キャラの足元。playerDead: 自キャラが死んだか。kills: このティックに倒した数。
	// 終わり方の順：死んだ → 脱出した（同じティックに重なったら、先の物）。
	void tick( const GameLib::Math::Vector3& feet, bool playerDead, int kills );

	RaidEnding ending() const;            // まだなら RaidEnding::None
	const RaidClock& clock() const;
	const ExtractionTracker& extraction() const;
	int kills() const;                     // この出撃で倒した数
	// 終わった出撃の結果（結果の画面が読む）。
	RaidResult result() const;

	// ── F6（確認用。Debug・Profile だけで呼ぶ）──
	void forceExtract();

private:
	static int pickCarSpot( const std::vector< TownEscapeCarSpot >& carSpots, unsigned seed );
	static std::vector< ExtractZone > zoneAt( const std::vector< TownEscapeCarSpot >& carSpots, int spot );

	int mEscapeCarSpot; // mExtraction より先に決める（並びの順に作られる）
	RaidClock mClock;
	ExtractionTracker mExtraction;
	int mKills;
	RaidEnding mEnding;
};

#endif
