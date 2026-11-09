#ifndef INCLUDED_UPDATE_PHASE_H
#define INCLUDED_UPDATE_PHASE_H

// 部品（Component）が毎ティック動く「段」（SummaryPlan/個別仕様/設計と分割/ComponentSystem2_Plan.txt の ★2）。
// GameObjectWorld::update( 段 ) が、その段の部品の update を作った順に呼ぶ。段をどの順で呼ぶかは GameWorld::update に書く
// （Unity の Update / LateUpdate と Script Execution Order を、番号ではなく名前で決める形。順番がコードに見えるように）。
// 段は、今動かしている部品の分だけ持つ。新しい順番が要る部品を足すときに、ここへ足して GameWorld::update で呼ぶ。
enum class UpdatePhase{
	None,  // 自分では動かない（隣の部品から頼まれて動く・読まれるだけ）
	Input, // 操作を読んで動く（自キャラ）
	Think, // 考えて動く（ゾンビ）。自キャラが動いた後の位置を追う
	Late,  // 動いた後の物に付いて動く（銃口の光は、撃ったティックのうちに光り始める）
};

// 段の数（None も含める。GameObjectWorld が段ごとの並びを持つのに使う）。
const int kUpdatePhaseNumber = static_cast< int >( UpdatePhase::Late ) + 1;

#endif
