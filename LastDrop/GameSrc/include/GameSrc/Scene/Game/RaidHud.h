#ifndef INCLUDED_RAID_HUD_H
#define INCLUDED_RAID_HUD_H

#include <string>

class UiRenderer;
class RaidSession;
class ExtractionTracker;

// 本編の HUD（R2_Plan.txt 6-2。2026-11-05 に GameHud〈車のガソリン〉から置き換えた）。
// ・右上：残り時間「09:42」（残りが RaidParams::warningSeconds 以下で赤）
// ・始めの数秒、上の真ん中：「DAY 2」と、開いている脱出点「EXITS: NORTH / EAST」
// ・脱出点の円の中：真ん中の下に「EXTRACTING 3.2」
// 自キャラの体力・持ち物は出さない（バイオハザード式で隠す・カバンの画面で見る。今までの決定のまま）。
// 表示するだけで、ゲームの状態は変えない（RaidSession を読むだけ）。文言と並べ方を変えるときは、このクラスだけを直す。
class RaidHud{
public:
	// renderer: 借りる。所有権は受け取らない。nullptr 不可。この HUD より長く生きていること。
	explicit RaidHud( UiRenderer* renderer );
	RaidHud( const RaidHud& ) = delete;
	RaidHud& operator=( const RaidHud& ) = delete;

	// 積んで描く（中で UiRenderer::draw まで行う）。1回の描画につき1回、3D の後に呼ぶ（上に重ねるため）。
	// 同じ描画で一時停止のメニュー・カバンの画面は描かないこと（UiRenderer は 1 回の描画に 1 回だけ）。day: 何日目か。
	void draw( const RaidSession& session, int day );

	// 開いている脱出点の名前を separator でつないだ文字（カバンの画面の見出しの下にも使う）。
	static std::string exitNames( const ExtractionTracker& extraction, const char* separator );

private:
	UiRenderer* mRenderer; // 借りているだけ
};

#endif
