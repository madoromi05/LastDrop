#ifndef INCLUDED_TARGET_H
#define INCLUDED_TARGET_H

#include "GameLib/Math/Vector3.h"
#include "GameLib/Base/AutoPtr.h"
using namespace GameLib;
using namespace GameLib::Math;

class GraphicsDatabase;
class Model;

// ────────────────────────────────────────────────
// Target：アクション検証用の静止ダミー。
//
// 企画書 v4 の「対象（異常）」の最小スタンドイン。今回は移動・攻撃・AIを持たず、
// プレイヤーの機動（ワイヤー引き寄せ・回避・射撃で弱化）を確認するためだけに置く。
// ボスAI・テレグラフ・フェーズ遷移・捕縛はここには実装しない（別スコープ）。
//
// 提供するのは「見た目」「被弾で減る体力」「ワイヤー/弾が当たる球」だけ。
// ────────────────────────────────────────────────
class Target{
public:
	static const float mRadius;       // 当たり・アンカー判定に使う球の半径。
	static const int   mMaxHitPoint;  // 最大体力。0で弱化しきり。

	Target( const Vector3& position );
	~Target();

	void draw() const;

	const Vector3* getPosition() const;
	float getRadius() const;

	void setDamage( int damage );
	int  getHitPoint() const;
	bool isDown() const;               // 体力0＝弱化しきった状態。

private:
	Vector3 mPosition;
	int     mHitPoint;
	AutoPtr< GraphicsDatabase > mDatabase;
	AutoPtr< Model > mModel;
};

#endif
