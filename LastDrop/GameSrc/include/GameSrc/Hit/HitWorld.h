#ifndef INCLUDED_HIT_WORLD_H
#define INCLUDED_HIT_WORLD_H

#include "GameLib/Math/Vector3.h"
#include "GameSrc/Hit/HitboxSet.h"
#include <vector>

class IHittable;
class IRayBlocker;

// 弾が当たりうるものの名簿と、それへの問い合わせ（「この線分は誰（何）のどこに最初に当たるか」）。
// 名簿は2つ。部位を持つキャラ（IHittable）と、弾を止める物（IRayBlocker。建物など）。問い合わせは両方を通して一番手前を返す。
// 移動の当たり判定（CollisionWorld）とは別の名簿。形も目的も違う（向こうは体を真上から囲む 2D の長方形、こちらは 3D）。
// 当たった後どうするか（ダメージ・部位破壊・弾痕）は知らない。問い合わせるだけ。
class HitWorld{
public:
	// 線分が当たった結果。target と blocker のどちらか一方だけが入る。
	struct Hit{
		const IHittable* target;    // 当たったキャラ。世界（建物など）に当たったなら nullptr
		const IRayBlocker* blocker; // 当たった物。キャラに当たったなら nullptr
		BodyPart part;              // 当たった部位（target のときだけ意味がある）
		float time;                 // 当たった時刻（0〜1。線分のうち何割進んだところか）
		GameLib::Math::Vector3 point;  // 当たった位置（ワールド座標）
		GameLib::Math::Vector3 normal; // 当たった面の外向きの法線（ワールド・長さ1）
	};

	// 当たりうるキャラを登録する。所有権は受け取らない。nullptr 不可。
	// この名簿に問い合わせる間は生きていること（シーンでは、この名簿をキャラより先に宣言する＝キャラより後に破棄される）。
	void addTarget( const IHittable* target );
	// 弾を止める物を登録する。所有権は受け取らない。nullptr 不可。寿命の決まりは addTarget と同じ。
	void addBlocker( const IRayBlocker* blocker );
	// 登録したものを外す（消す前に呼ぶ）。登録されていなければ何もしない。
	void removeTarget( const IHittable* target );
	void removeBlocker( const IRayBlocker* blocker );

	// origin から delta だけ進む線分が一番早く当たるものを hit に書く。何にも当たらなければ false。
	// ignore: 調べないキャラ（撃った本人）。所有権は受け取らない。nullptr なら全員を調べる。
	// hit: 出力先。nullptr 不可。
	bool raycast( const GameLib::Math::Vector3& origin, const GameLib::Math::Vector3& delta, const IHittable* ignore,
		Hit* hit ) const;

	// 登録されたものを読む（当たり判定の表示のため）。
	const std::vector< const IHittable* >& targets() const;
	const std::vector< const IRayBlocker* >& blockers() const;

private:
	std::vector< const IHittable* > mTargets;    // 借りているだけ
	std::vector< const IRayBlocker* > mBlockers; // 借りているだけ
};

#endif
