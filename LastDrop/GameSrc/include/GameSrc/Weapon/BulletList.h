#ifndef INCLUDED_BULLET_LIST_H
#define INCLUDED_BULLET_LIST_H

#include "GameSrc/Weapon/Bullet.h"
#include "GameLib/Math/Vector3.h"
#include <vector>

class StaticMeshData;
class IHittable;

// 飛んでいる弾の持ち主。撃つ（fire）と1発増え、進めて（update）終わった弾を消し、残りを描く（draw）。
// 弾は名簿（CollisionWorld・HitWorld）に載らないので、GameObjectWorld ではなくここで持つ（載せる・外す相手がいない）。
// 弾を外から指すポインタは渡さない（弾は値で持ち、消すと並びが詰まるため）。
// いつ撃つか（連射の間隔）は知らない（FireControl）。当たった後どうするかも知らない（Bullet と同じ）。
class BulletList{
public:
	// 弾が当たった所と、そのとき弾が飛んでいた向き（血しぶきを弾の抜ける向きへ飛ばすのに使う）。
	struct Impact{
		HitWorld::Hit hit;
		GameLib::Math::Vector3 direction; // 弾の飛んでいた向き（長さ1）
	};

	// data: 弾頭の形と見た目（Resource/bullet_tip.txt）。所有権は受け取らない。nullptr 不可。このリストより長く生きていること（MeshCache が持つ）。
	//       まだ構築中でもよい。構築に失敗したら、弾は飛んで当たるが描かれない。
	explicit BulletList( const StaticMeshData* data );

	// origin から direction（長さ1）へ1発撃ち出す。
	void fire( const GameLib::Math::Vector3& origin, const GameLib::Math::Vector3& direction );
	// 全部の弾を1ティック進め、終わった弾（当たった・飛びきった）を消す。
	// world / shooter: Bullet::update へそのまま渡す（当たりを聞く名簿と、当てないキャラ）。
	// impacts: このティックに当たった所を後ろへ足す（前の中身は消さない）。nullptr 不可。
	//          当たった後どうするか（弾痕・ダメージ・血）は、受け取った側が決める。
	void update( const HitWorld& world, const IHittable* shooter, std::vector< Impact >* impacts );
	void draw() const;

private:
	const StaticMeshData* mData; // 借りているだけ
	std::vector< Bullet > mBullets;
};

#endif
