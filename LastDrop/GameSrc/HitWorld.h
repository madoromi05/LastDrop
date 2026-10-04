#ifndef INCLUDED_HIT_WORLD_H
#define INCLUDED_HIT_WORLD_H

#include "GameLib/Math/Vector3.h"
#include "HitboxSet.h"
#include <vector>

class IHittable;

// 弾が当たりうるものの名簿と、それへの問い合わせ（「この線分は誰のどの部位に最初に当たるか」）。
// 移動の当たり判定（CollisionWorld）とは別の名簿。形も目的も違う（向こうは体を真上から囲む 2D の長方形、こちらは部位ごとの 3D の箱）。
// 当たった後どうするか（ダメージ・部位破壊）は知らない。問い合わせるだけ。
// 建物で弾が止まる判定はまだ無い（建物の当たり判定は高さを持たないため）。射撃を作るときに足す。
class HitWorld{
public:
	// 当たりうるものを登録する。所有権は受け取らない。nullptr 不可。
	// この名簿に問い合わせる間は生きていること（シーンでは、この名簿をキャラより先に宣言する＝キャラより後に破棄される）。
	void addTarget( const IHittable* target );

	// origin から delta だけ進む線分が一番早く当たる相手と部位を、targetOut と hit に書く。当たらなければ false。
	// ignore: 調べない相手（撃った本人）。所有権は受け取らない。nullptr なら全員を調べる。
	// targetOut: 当たった相手の出力先。所有権は渡さない。nullptr なら書かない。hit: 出力先。nullptr 不可。
	bool raycast( const GameLib::Math::Vector3& origin, const GameLib::Math::Vector3& delta, const IHittable* ignore,
		const IHittable** targetOut, HitResult* hit ) const;

	// 登録されたものを読む（当たり判定の表示のため）。
	const std::vector< const IHittable* >& targets() const;

private:
	std::vector< const IHittable* > mTargets; // 借りているだけ
};

#endif
