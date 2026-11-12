#ifndef INCLUDED_EXTRACTION_TRACKER_H
#define INCLUDED_EXTRACTION_TRACKER_H

#include "GameLib/Math/Vector3.h"
#include <vector>

// 脱出できる所 1 つ（FuelEscapeLoading_Plan.txt）。name は画面に出す名前（TownMap の表の文字列を借りる）
struct ExtractZone{
	const char* name;
	GameLib::Math::Vector3 position; // 円の中心（車の足元の中心）
};

// 脱出できる所の円の中に、何秒居続けたかを数える（R2_Plan.txt Q11）
// 始めは閉じていて、open（脱出の車へ要る数のガソリンを入れ終えた）の後だけ数える
// 円を出たら 0 に戻す。攻撃されても止めない（タルコフと同じ）。決まった秒に届いたら脱出した、と答えるだけで、物には触らない
// 円の大きさと何秒かは作るときに受け取る（RaidParams）。どこにするかは知らない（RaidSession が選んで渡す）
class ExtractionTracker{
public:
	ExtractionTracker( const std::vector< ExtractZone >& zones, float radius, float seconds );	// zones: 脱出できる所（写して持つ）。radius: 円の半径（m。水平だけで測る）。seconds: 居続ける長さ（秒）。どちらも 0 より大きいこと

	// 数え始める（給油し終えた）。2 回目からは何もしない
	void open();
	bool isOpen() const;
	void tick( const GameLib::Math::Vector3& feet );	// 1 ティック進める。feet: 自キャラの足元（ワールド）。開いていて円の中なら数え、外なら 0 に戻す。脱出した後は何もしない
	bool isInside() const;								// 円の中にいるか（最後の tick。開く前は false）
	// 脱出し終えるまでの残り（秒。円の外では居続ける長さそのもの）
	float remainingSeconds() const;
	bool hasExtracted() const;
	// すぐに脱出したことにする（F6 で、脱出の後の流れをすぐ試すため）
	void forceExtract();
	const std::vector< ExtractZone >& zones() const;

private:
	std::vector< ExtractZone > mZones;
	float mRadius;
	int mRequiredTicks;
	int mInsideTicks; // 円の中に居続けたティック数（出たら 0）
	bool mOpen;
	bool mInside;
	bool mExtracted;
};

#endif
