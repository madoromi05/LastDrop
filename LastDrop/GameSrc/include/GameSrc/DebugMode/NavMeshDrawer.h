#ifndef INCLUDED_NAV_MESH_DRAWER_H
#define INCLUDED_NAV_MESH_DRAWER_H

#include "GameLib/Math/Matrix44.h"
#include "GameLib/Scene/PrimitiveRenderer.h"

class NavMesh;

// ゾンビが歩ける床（NavMesh）の多角形の辺を、地面の少し上に薄い緑の線で描く（開発中の確認用）
// 壁・建物の側の縁が、ゾンビの足元が入れる範囲の境目。内側の線は、床を多角形に分けた区切り（壁ではない）
// 手前の物に隠れない線で描く。描くだけで、床を変えない。ON/OFF はこのクラスを呼ぶか呼ばないか（GameDebugOverlay。F6 の NavMesh）
class NavMeshDrawer{
public:
	NavMeshDrawer();

	// navMesh の床の辺を描く。床ができていなければ何もしない
	// 1フレームに1回まで（PrimitiveRenderer の約束）。他のものを描いた後、同じ描画先（PostProcess の beginScene〜endScene の間）で呼ぶ
	// projectionView: カメラの射影×ビュー行列
	void draw( const NavMesh& navMesh, const GameLib::Math::Matrix44& projectionView );

private:
	GameLib::Scene::PrimitiveRenderer mRenderer; // 最初の draw で作る（それまでは空）。床の辺が増えたら作り直す
	int mLineCapacity;                           // mRenderer に描ける線の数（0 なら未作成）
};

#endif
