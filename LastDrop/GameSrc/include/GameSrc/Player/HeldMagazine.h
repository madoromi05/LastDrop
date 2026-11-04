#ifndef INCLUDED_HELD_MAGAZINE_H
#define INCLUDED_HELD_MAGAZINE_H

#include "GameLib/Math/Matrix34.h"

class StaticMeshData;
class SkinnedMeshInstance;

// 銃から外せる弾倉の見た目。銃に付いている・左手に持っている・左手から銃の差し込み口へ寄せている、のどこにあるかで描く場所を変える。
// 弾倉のモデルは銃のモデルと同じ座標で作ってある（tools/configs/rifle_mag.json・pistol_mag.json など）ので、銃に付いている間は銃と同じ行列で描けば元の場所に収まる。
// いつ外す・持つ・はめるか（リロードの時間割）は知らない（呼び出し側が WeaponReload から読んで渡す）。
class HeldMagazine{
public:
	// data: 弾倉の形と見え方。所有権は受け取らない。nullptr 不可。このインスタンスより長く生きていること（MeshCache が持つ）。
	// handBone: 弾倉を持つ手の骨（SkinnedMeshData::findBone の番号）。
	HeldMagazine( const StaticMeshData* data, int handBone );

	// 銃から外して手に持つ。今の手から見た弾倉の場所を覚え、それからは手に付いて動く（手の中で弾倉がずれない）。
	// holder の update の後に呼ぶ。gun: 今の銃の置き場所（HeldItem::placement）。まだポーズが無ければ何もしない。
	void takeOut( const SkinnedMeshInstance& holder, const GameLib::Math::Matrix34& gun );
	// 描く。gun: 今の銃の置き場所。inHand: 手に持っているか（false なら銃に付けて描く）。
	// seatWeight: 手から銃の差し込み口へ寄せる量（0 = 手のまま、1 = 銃に付いた場所）。手の位置と差し込み口がずれていても跳ばずにはまる。
	void draw( const SkinnedMeshInstance& holder, const GameLib::Math::Matrix34& gun, bool inHand, float seatWeight ) const;

private:
	const StaticMeshData* mData;
	int mHandBone;
	GameLib::Math::Matrix34 mInHand; // 手の骨から見た弾倉の置き場所（takeOut で覚える）
};

#endif
