#ifndef INCLUDED_DAY_CONDITION_H
#define INCLUDED_DAY_CONDITION_H

// 日ごとの条件（SummaryPlan/個別仕様/DayCondition_Plan.txt）。何が変わるかは、この表（DayCondition.cpp の kEffects）だけを見れば分かる。
// 調整値（LightingParams など）は書き換えず、使う所でこの倍率を掛ける（F6 の値が正のまま）。
// どの日にどれが出るかは RunRules、何日目がどれかを持つのは RunState。
enum class DayCondition{
	Normal,
	Blackout, // 停電：街灯が消える
	Fog,      // 霧：遠くが見えない（ゾンビの目は段階 D-b）
	Rain,     // 雨：足音が届きにくい（雨の粒・音・濡れた地面は段階 D-c）
	Horde,    // 群れ：ゾンビが多い
	Count,
};

struct DayConditionEffect{
	const char* saveName;       // save.xml に書く名前（F6 にも出す）
	const char* title;          // 「霧の日」など（拠点の画面・出撃の始めの知らせ）
	const char* summary;        // 一言の説明（拠点の画面）
	float streetLightScale;     // 街灯の明るさに掛ける（停電 0）
	float fogDistanceScale;     // 霧が半分になる距離に掛ける（霧 0.3 ＝ 40m → 12m。描く距離も一緒に縮む）
	float footstepHearingScale; // ゾンビが足音を聞く距離に掛ける（雨 0.5）
	int zombieCount;            // 立っているゾンビの目標と、同時に居られる数（死体も数える）
};

// condition の効き目（表の 1 行。起動中ずっとあるので、参照で持ってよい）。
const DayConditionEffect& dayConditionEffect( DayCondition condition );
// save.xml の名前から引く。知らない名前・nullptr はふつう（古い save.xml・手で直したファイルでも止まらない）。
DayCondition dayConditionFromSaveName( const char* name );

#endif
