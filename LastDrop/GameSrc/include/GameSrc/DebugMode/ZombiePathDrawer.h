#ifndef INCLUDED_ZOMBIE_PATH_DRAWER_H
#define INCLUDED_ZOMBIE_PATH_DRAWER_H

#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix44.h"
#include "GameLib/Scene/PrimitiveRenderer.h"
#include <vector>

class GameObjectWorld;
class NavMesh;

// ゾンビが建物を回り込む通り道を、足元から通り道の点を結ぶ水色の線で描く（開発中の確認用。ZombiePath が通り道を持つ間だけ）。
// 建物の向こうへ回り込む道を見るため、手前の物に隠れない線で描く。まっすぐ向かっているゾンビは描かない。
// あわせて、歩ける床（NavMesh）の多角形の縁を薄い緑で、仲間を避けて歩く向き（分離）を体から伸びる橙色の線で描く
// （SummaryPlan/個別仕様/ZombieNavMesh_Plan.txt・ZombieCrowd_Plan.txt）。
// 描くだけで、ゾンビを変えない。ON/OFF はこのクラスを呼ぶか呼ばないか（GameDebugOverlay の F3）。
class ZombiePathDrawer{
public:
	ZombiePathDrawer();

	// objects のゾンビ（ZombieStateComponent を持つ物）のうち通り道を持つものと、navMesh の床の縁を描く。
	// 1フレームに1回まで（PrimitiveRenderer の約束）。他のものを描いた後、同じ描画先（PostProcess の beginScene〜endScene の間）で呼ぶ。
	// projectionView: カメラの射影×ビュー行列。容量を超えた分は描かない。
	void draw( const GameObjectWorld& objects, const NavMesh& navMesh, const GameLib::Math::Matrix44& projectionView );

private:
	GameLib::Scene::PrimitiveRenderer mRenderer; // 最初の draw で作る（それまでは空）。床の縁が増えたら作り直す
	int mLineCapacity;                           // mRenderer に描ける線の数（0 なら未作成）
	std::vector< GameLib::Math::Vector3 > mRoute; // 1体ぶんの残りの通り道（毎回確保し直さないために持つ）
};

#endif
