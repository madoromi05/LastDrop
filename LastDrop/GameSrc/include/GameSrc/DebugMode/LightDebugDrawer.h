#ifndef INCLUDED_LIGHT_DEBUG_DRAWER_H
#define INCLUDED_LIGHT_DEBUG_DRAWER_H

#include "GameLib/Math/Matrix44.h"
#include "GameLib/Scene/PrimitiveRenderer.h"

class LightList;

// 点光源・スポット（街灯・マズルフラッシュ）の置き場所と届く範囲を描く（開発中の確認用。PbrNightLighting_Plan.txt 11 章）。
//   光の位置：小さな十字と、真下の地面までの縦の線。スポット（懐中電灯）は照らす向きへ 3m の線も
//   届く範囲：光の届く球が地面（車道の面。高さ 0）を切る円
// 照らしている物（LightList が 16 個に選んだ物）は黄色、選ばれず消えている物は灰色。平行光（月）は描かない。
// 建物の向こうの街灯も見えるよう、手前の物に隠れない線で描く。描くだけで、ライトを変えない。
// ON/OFF はこのクラスを呼ぶか呼ばないか（GameDebugOverlay の F3）。
class LightDebugDrawer{
public:
	// lights：最後に bind した一覧（選ばれたかを読む）。1フレームに1回まで（PrimitiveRenderer の約束）。
	// 他のものを描いた後、同じ描画先（PostProcess の beginScene〜endScene の間）で呼ぶ。projectionView: カメラの射影×ビュー行列。
	void draw( const LightList& lights, const GameLib::Math::Matrix44& projectionView );

private:
	GameLib::Scene::PrimitiveRenderer mRenderer; // 最初の draw で作る（それまでは空）
};

#endif
