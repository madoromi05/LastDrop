#ifndef INCLUDED_NAV_MESH_H
#define INCLUDED_NAV_MESH_H

#include "GameLib/Math/Vector3.h"
#include <memory>
#include <vector>

class CollisionWorld;
class dtNavMesh;
class dtNavMeshQuery;

// 町の歩ける床の形（ナビメッシュ）と、その上の道探し。Recast で作り、Detour で探す（SummaryPlan/個別仕様/ZombieNavMesh_Plan.txt）。
// 作るのは NavMeshBuilder（2026-11-09 に分けた。MidPrioritySplit_Plan.txt 4 章）。ここは作った形を持ち、道を探す・確認用の縁を出す。
// 材料は移動の当たり判定の動かない形（建物の WallMesh・車などの箱）なので、物を置けば床の形に勝手に穴が開く（手で図を直す所が無い）。
// 途中で消える物（落ちているガソリン缶）・キャラは入れない（歩く側が壁に沿って滑り、詰まったら横へよける）。
// 床は高さ 0 の平らな 1 枚として作る（歩道の段差はキャラの足元だけで扱うため。問い合わせは上下に余裕を持って床を探す）。
// 床の縁は体の半径だけ壁から離してあるので、通り道の角をたどれば体が壁にこすらない。
// 道探しは中の作業領域を書き換えるので、同時に 2 か所から呼ばない（ゾンビは 1 体ずつ順に update する）。
class NavMesh{
public:
	NavMesh();
	~NavMesh();
	NavMesh( const NavMesh& ) = delete;
	NavMesh& operator=( const NavMesh& ) = delete;

	// world の動かない形（WallMesh・箱）から床の形を作る。前に作った物は捨てる。
	// 町を置き終えた後に 1 回だけ呼ぶ（数十〜数百 ms かかる。かかった時間はログへ出す）。作れなければ false（isReady も false のまま）。
	bool build( const CollisionWorld& world );
	bool isReady() const;	// 床の形ができているか

	// from から to まで、床の上をまっすぐ歩いて通れるか（体の半径ぶん壁から離れた線が、床の縁を越えないか）。
	// 床ができていなければ true（今までどおりまっすぐ向かい、壁に沿って滑る）。
	bool isWalkable( const GameLib::Math::Vector3& from, const GameLib::Math::Vector3& to ) const;
	// start から goal までの通り道を out に書く（曲がる角を順に並べ、最後に goal）。前の中身は捨てる。
	// goal へ着けない（建物の中など）ときは、着ける一番近い所までの角のあとに goal を置く（そこから先は壁に沿って滑る）。
	// start・goal の近くに床が無い・床ができていなければ false（out は空）。out: nullptr 不可。
	bool findPath( const GameLib::Math::Vector3& start, const GameLib::Math::Vector3& goal,
		std::vector< GameLib::Math::Vector3 >* out ) const;
	// 床の多角形の縁（2 点ずつで 1 本の線。確認用の表示に使う）。build のときに 1 回だけ作る。
	const std::vector< GameLib::Math::Vector3 >& outline() const;

private:
	// Detour の物は Detour の関数で消す（delete では消せない）。
	struct NavMeshDeleter{
		void operator()( dtNavMesh* navMesh ) const;
	};
	struct QueryDeleter{
		void operator()( dtNavMeshQuery* query ) const;
	};

	// 床の多角形の縁を mOutline に書く（隣の多角形と共有する辺は 1 本だけ）。
	void buildOutline();

	std::unique_ptr< dtNavMesh, NavMeshDeleter > mNavMesh;    // 空なら床ができていない
	std::unique_ptr< dtNavMeshQuery, QueryDeleter > mQuery;   // mNavMesh を借りて探す
	std::vector< GameLib::Math::Vector3 > mOutline;
};

#endif
