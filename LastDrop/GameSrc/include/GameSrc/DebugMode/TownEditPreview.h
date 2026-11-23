#ifndef INCLUDED_TOWN_EDIT_PREVIEW_H
#define INCLUDED_TOWN_EDIT_PREVIEW_H

#include "GameLib/Scene/PrimitiveRenderer.h"

class FirstPersonCamera;

// F6 の Town タブで選んでいる物を、町の中に枠で示す（開発中の確認用）
// 元の位置は灰色、今の位置（欄の数字）は黄色の箱。箱の大きさは種類ごとに決まった大きさ（TownEditFrame）
// 数字を動かすと枠はすぐ動く（町の物の見た目も TownEditModelMover がすぐ動かす・消す・置く）。移動の当たり判定と床が合うのは、Apply で出撃を読み直したとき
// 建物の向こうでも見つけられるよう、手前の物に隠れない線で描く。何を選んでいるかは TownEditPanel::highlight を読むだけで、手直しは変えない
// Debug・Profile ビルドだけ（Release では中身をビルドしない）
class TownEditPreview{
public:
	// 選んでいる物があれば描く（F6 が閉じている・選んでいないなら何もしない）。1 フレームに 1 回まで（PrimitiveRenderer の約束）
	// 他のものを描いた後、同じ描画先（PostProcess の beginScene〜endScene の間）で呼ぶ。camera: 今の描画のカメラ
	void draw( const FirstPersonCamera& camera );

private:
	GameLib::Scene::PrimitiveRenderer mRenderer; // 最初に描くときに作る（それまでは空）
};

#endif
