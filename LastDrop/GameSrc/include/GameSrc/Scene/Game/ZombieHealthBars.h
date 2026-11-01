#ifndef INCLUDED_ZOMBIE_HEALTH_BARS_H
#define INCLUDED_ZOMBIE_HEALTH_BARS_H

#include "GameLib/Math/Matrix44.h"

class GameWorld;
class FirstPersonCamera;
class UiRenderer;
class ZombieStateComponent;

// 確認用の、ゾンビの体力の表示（Debug・Profile ビルドだけ。GameDebugOverlay が F3 で出し入れする）。
// ・頭の上に全体の体力のバーと「HP 80/120」。倒れた後は状態（Falling / Corpse 残り秒 / Sinking）も出す
// ・近いときは、部位の箱の中心に部位の体力の数字。0 になった部位は赤い「0 BROKEN」
// 世界の位置を画面の位置へ直し、UiRenderer へ積む（描くのは UiRenderer::draw。HUD の描画でまとめて描かれる）。
// カメラの後ろ・遠い・目との間に壁や建物があるゾンビは出さない（当たり判定の表示と同じく、物に隠れる物は見せない）。
// 世界もゾンビも読むだけで、変えない。ゾンビは表示のことを知らない（体力を読む入口を出すだけ）。
class ZombieHealthBars{
public:
	// world の全ゾンビの体力を renderer に積む。1回の描画につき1回、HUD を描く前に呼ぶ。renderer: nullptr 不可。
	void add( const GameWorld& world, const FirstPersonCamera& camera, UiRenderer* renderer ) const;

private:
	// 1体ぶんを積む。見えない（後ろ・遠い・壁の向こう）なら何もしない。
	void addZombie( const ZombieStateComponent& zombie, const GameWorld& world, const FirstPersonCamera& camera,
		const GameLib::Math::Matrix44& projectionView, UiRenderer* renderer ) const;
	// 部位の箱の中心に、部位の体力の数字を積む。
	void addPartLabels( const ZombieStateComponent& zombie, const GameLib::Math::Matrix44& projectionView, UiRenderer* renderer ) const;
};

#endif
