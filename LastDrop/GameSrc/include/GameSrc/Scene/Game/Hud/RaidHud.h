#ifndef INCLUDED_RAID_HUD_H
#define INCLUDED_RAID_HUD_H

#include <string>

class UiRenderer;
class RaidSession;
class ExtractionTracker;
struct DayConditionEffect;

// 本編の HUD（R2_Plan.txt 6-2。2026-11-05 に GameHud〈車のガソリン〉から置き換えた）。
// 2026-11-09 に時間制限と 5 日間をやめたので、右上の残り時間と「DAY 2」は消した（SimpleRun_Plan.txt Q4）。
// ・始めの数秒、上の真ん中：今回の条件「霧の日」（DayCondition_Plan.txt Q4）・脱出の車の場所「脱出の車：北」（FuelEscapeLoading_Plan.txt Q14）
// ・給油し終えた後、車のそばの円の中：真ん中の下に「脱出中 3.2」。円の外：「エンジンがかかった：車のそばで待つ」
// ・漁れる場所を開けている間：真ん中の少し下に「探している」と満ちていく棒（R3_Plan.txt 6-1）。
//   脱出の車に給油している間は、同じ棒に「給油中」（FuelEscapeLoading_Plan.txt 改訂 2）
// 自キャラの体力・持ち物は出さない（バイオハザード式で隠す・カバンの画面で見る。今までの決定のまま）。
// 表示するだけで、ゲームの状態は変えない（RaidSession を読むだけ）。文言と並べ方を変えるときは、このクラスだけを直す。
class RaidHud{
public:
	// 真ん中の少し下の棒が、何を待っているか（棒の上の文字が変わる）。
	enum class ProgressKind{
		Search, // 漁れる場所を開けている「探している」
		Refuel, // 脱出の車に給油している「給油中」
	};

	// renderer: 借りる。所有権は受け取らない。nullptr 不可。この HUD より長く生きていること。
	explicit RaidHud( UiRenderer* renderer );
	RaidHud( const RaidHud& ) = delete;
	RaidHud& operator=( const RaidHud& ) = delete;

	// 積んで描く（中で UiRenderer::draw まで行う）。1回の描画につき1回、3D の後に呼ぶ（上に重ねるため）。
	// 同じ描画で一時停止のメニュー・カバンの画面は描かないこと（UiRenderer は 1 回の描画に 1 回だけ）。
	// condition: 今回の条件（始めの知らせの 1 行目に名前を出す）。
	// progress: 漁る・給油の進み具合（0〜1。真ん中の少し下に棒を出す。待っていなければ負の値で、出さない）。
	// progressKind: 棒の上に出す文字の種類（progress が負なら読まない）。
	void draw( const RaidSession& session, const DayConditionEffect& condition, float progress, ProgressKind progressKind );

	// 脱出の車の場所の名前を separator でつないだ文字（カバンの画面の見出しの下にも使う）。
	static std::string exitNames( const ExtractionTracker& extraction, const char* separator );

private:
	UiRenderer* mRenderer; // 借りているだけ
};

#endif
