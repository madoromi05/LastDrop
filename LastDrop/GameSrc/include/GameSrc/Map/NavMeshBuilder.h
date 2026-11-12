#ifndef INCLUDED_NAV_MESH_BUILDER_H
#define INCLUDED_NAV_MESH_BUILDER_H

class CollisionWorld;
class dtNavMesh;

// 町の歩ける床の形（ナビメッシュ）を、移動の当たり判定の動かない形から Recast で作る係
// （SummaryPlan/個別仕様/設計と分割/MidPrioritySplit_Plan.txt 4 章）
// 手順：材料の三角形を集める（建物の WallMesh・車などの箱・床 1 枚）→ Recast の設定 → 多角形にする → Detour の形へ詰める
// 作った形の上で道を探すのは NavMesh（ここは作って渡すだけで、持たない）
class NavMeshBuilder{
public:
	// world の動かない形から床の形を作って返す。作れなければ nullptr（理由はログ「NavMesh: failed to …」）
	// 返した物は呼んだ側が持ち、dtFreeNavMesh で消す。polygonCount: 作った多角形の数を書く（ログ用）。nullptr 不可
	static dtNavMesh* build( const CollisionWorld& world, int* polygonCount );
};

#endif
