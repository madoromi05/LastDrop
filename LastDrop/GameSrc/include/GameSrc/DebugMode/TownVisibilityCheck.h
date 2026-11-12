#ifndef INCLUDED_TOWN_VISIBILITY_CHECK_H
#define INCLUDED_TOWN_VISIBILITY_CHECK_H

#include <vector>

class TownVisibility;
class Building;
class HitWorld;

// 簡易 PVS（TownVisibility）の表を、3D の線で確かめる道具（OptimizationC_Plan.txt Q6。Debug だけで使う）
// 表を作る・読む物（TownVisibility）と、確かめる物（ここ）を分けてある
// 表は公開の口（cellAt・showsBuilding・cellBounds・isOpenCell）だけで読み、建物の足元は建物の形から求め直す
class TownVisibilityCheck{
public:
	// 乱数の点から PVS が「見えない」とした建物へ 3D の線（hitWorld）を撃ち、3D では見えた数を出力へ書く
	// buildings：表を作るのに渡した物と同じ並び。見落としが 0 でなければ、点の間隔か縮める幅を見直す。表がまだ無ければ何もしない
	static void run( const TownVisibility& visibility, const std::vector< Building >& buildings, const HitWorld& hitWorld );
};

#endif
