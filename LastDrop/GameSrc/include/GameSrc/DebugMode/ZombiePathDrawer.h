#ifndef INCLUDED_ZOMBIE_PATH_DRAWER_H
#define INCLUDED_ZOMBIE_PATH_DRAWER_H

#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix44.h"
#include "GameLib/Scene/PrimitiveRenderer.h"
#include <vector>

class GameObjectWorld;

// ゾンビが建物を回り込む通り道を、足元から通り道の点を結ぶ水色の線で描く（開発中の確認用。ZombiePath が通り道を持つ間だけ）
// 建物の向こうへ回り込む道を見るため、手前の物に隠れない線で描く。まっすぐ向かっているゾンビは描かない
// あわせて、仲間を避けて歩く向き（分離）を体から伸びる橙色の線で、通り道に沿って先読みで向いている点を紫の線で、
// 引っかかった体（ZombieMoveTrace の印が出ている間）を頭の上の赤い印で描く。歩ける床の辺は NavMeshDrawer が描く
// （SummaryPlan/個別仕様/ゾンビ/ZombieCrowd_Plan.txt）
// 描くだけで、ゾンビを変えない。ON/OFF はこのクラスを呼ぶか呼ばないか（GameDebugOverlay。F6 の Zombie info）
class ZombiePathDrawer{
public:
	// objects のゾンビ（ZombieStateComponent を持つ物）の通り道・分離・先読み・引っかかりの印を描く
	// 1フレームに1回まで（PrimitiveRenderer の約束）。他のものを描いた後、同じ描画先（PostProcess の beginScene〜endScene の間）で呼ぶ
	// projectionView: カメラの射影×ビュー行列。容量を超えた分は描かない
	void draw( const GameObjectWorld& objects, const GameLib::Math::Matrix44& projectionView );

private:
	void addStuckMarker( const GameLib::Math::Vector3& feet );	// feet（足元）の上に、引っかかった印（赤い縦線と十字）を積む

	GameLib::Scene::PrimitiveRenderer mRenderer;  // 最初の draw で作る（それまでは空）
	std::vector< GameLib::Math::Vector3 > mRoute; // 1体ぶんの残りの通り道（毎回確保し直さないために持つ）
};

#endif
