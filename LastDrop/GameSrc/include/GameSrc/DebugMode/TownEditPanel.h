#ifndef INCLUDED_TOWN_EDIT_PANEL_H
#define INCLUDED_TOWN_EDIT_PANEL_H

#include "GameSrc/Map/TownEdits.h"
#include "GameSrc/Raid/DayCondition.h"

class TownEnvironment;
class TownEditFileState;
class RaidRecord;
class FirstPersonCamera;
class TownCache;
class CollisionWorld;
class ZombieDirector;

// F6 のエディタの「Town」タブの中身。町の物を world 座標で手直しする（Debug・Profile ビルドだけ）
//   上の段：Save（TownEdit.xml へ）・Revert（ファイルから読み直す）・Apply（今の手直しで、同じ日の条件・同じ脱出の車のまま出撃を読み直す）
//           最後の操作の結果（灰色）と警告（黄色。今の町で使わなかった手直しの行・起動時か Revert でファイルの読めなかった行）
//   Bake の段：事前に作る床と表のファイルの状態と、書き出し（TownBakePanel）
//   選んだ物の欄：元の座標・x・y・z・yaw・Reset・Delete。Pick で飛んでいる自キャラの正面の物を選ぶ
//   種類ごとの一覧：元の物を「番号・元の x, z」で並べる（手直しした物は黄色）。足した物は「+ 番号」。見出しの Add で飛んでいる自キャラの正面の地面に足す
// 手直しは起動中ずっとある 1 つ（GameResources）を直接書き換える。保存していなくても Apply・Retry・次の出撃で使う
// 数字を動かす・Delete・Add は、枠（TownEditPreview）と町の物の見た目（TownEditModelMover）にすぐ出る
// 移動の当たり判定・歩ける床・隠れの表と、足した箱の中身は、Apply で出撃を読み直したときに合う
// 元の町の並び（一覧の番号と元の座標）は TownEditOriginals から読む
// 出撃の読み直し（Apply・Bake）を待つ係（TownEditReload）はここが 1 つ持ち、Bake の段にも貸す
namespace TownEditPanel{

// 欄が読む・触る物。どれも借りるだけ（所有権は受け取らない）。無い物は nullptr
struct Context{
	TownEdits* edits;						// 起動中ずっとある町の手直し（GameResources）。nullptr なら欄に何も出さない
	TownEditFileState* file;				// edits と TownEdit.xml の間の様子（GameResources）。読めなかった行を出し、Save・Revert・Bake が書き換える
	const TownEnvironment* town;			// 今の出撃の町（手直しを当てた後）。出撃を読み終えている間だけ
	int escapeCarSpot;						// 今の出撃の脱出の車の置き場所（town があるときだけ読む）
	DayCondition today;						// 今の出撃の日の条件（town があるときだけ読む）
	RaidRecord* record;						// Apply・Bake が、読み直す出撃の日の条件と車を決める所
	const FirstPersonCamera* flightCamera;	// 飛んでいる自キャラの視点（飛んでいる間だけ）。Pick と Add がこの正面の線を使う
	TownCache* cache;						// Bake が床と表を作って書き出す所（起動中ずっとある 1 つ）
	const CollisionWorld* collisionWorld;	// 今の出撃の移動の当たり判定の名簿。Bake が床を作る材料（town があるときだけ）
	const ZombieDirector* zombies;			// 今の出撃のゾンビの係。Release が床を作る長さを求める（town があるときだけ）
};

void update( const Context& context );	// F6 が開いている間、Town タブを出していなくても 1 回の描画につき 1 回呼ぶ（読み直しの待ちと Bake を進める）。Release では何もしない
void draw( const Context& context );	// Town タブを出しているときに、1 回の描画につき 1 回呼ぶ（update の後）。Release では何もしない

// 選んでいる物の、元の位置と今の位置（TownEditPreview が枠を描く）
struct Highlight{
	TownEditKind kind;
	bool hasOriginal;		// 元の位置があるか（足した物には無い）
	TownPlacement original;
	bool hasCurrent;		// 今の位置があるか（消した物には無い）
	TownPlacement current;
};
const Highlight* highlight();	// F6 が開いていて何か選んでいるなら、その物（最後に欄を描いたときの値）。それ以外は nullptr。Release では常に nullptr

} //namespace TownEditPanel

#endif
