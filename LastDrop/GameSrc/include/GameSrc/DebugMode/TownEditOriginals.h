#ifndef INCLUDED_TOWN_EDIT_ORIGINALS_H
#define INCLUDED_TOWN_EDIT_ORIGINALS_H

#include "GameSrc/Map/TownEdits.h"

// 手直しを当てていない町（TownMap の定数のまま）の並びを、F6 の Town タブの種類と番号で読む口
// 手直しの名札は元の座標なので、一覧（TownEditPanel）と、見た目を写す係（TownEditModelMover）が同じ数え方で物を指せるように 1 か所にまとめる
// 元の町は定数から組み立てるだけで起動中は変わらないので、最初に読んだときに 1 回だけ作って持ち続ける。手直しは変えない
class TownEditOriginals{
public:
	static int count( TownEditKind kind );													// kind の元の数。脱出の車と自キャラの始めの位置は、車の置き場所の数
	static const TownPlacement& at( TownEditKind kind, int index );							// kind の index 番の元の置き場所。index: 0〜count−1
	static TownEditKey keyOf( TownEditKind kind, int index );								// kind の index 番を指す手直しの名札（元の座標）
	static TownEditCounts counts();															// 消しすぎを確かめる元の数（ゴミ箱と木箱・木箱・湧く候補）
	static const TownPlacement* placedAt( const TownEdits& edits, TownEditKind kind, int index );	// edits を当てた後の index 番の置き場所（動かした先か元の場所）。消した物は nullptr
};

#endif
