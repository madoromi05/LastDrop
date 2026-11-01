#ifndef INCLUDED_PLAYER_FACTORY_H
#define INCLUDED_PLAYER_FACTORY_H

#include "GameLib/Math/Vector3.h"

class MeshCache;
class SkinnedMeshData;
class StaticMeshData;
class GameObject;
class GameObjectWorld;

// 自キャラと、最初から手に持っている銃 "AK47" を部品から組み立てる（Unity のプレハブの代わり。ZombieFactory と同じ役）。
// 部品の並び・形のファイル・名前を 1 か所で決める（2026-11-02 ユーザー指定：GameWorld から分けた）。
// どこに置くか（TownMap::playerStart）と、置いた後に読むのは GameWorld。
// 拾える銃を作るときは、銃の組み立てを WeaponFactory へ分ける。
class PlayerFactory{
public:
	// 形の読み込みを頼むだけ（非同期）。meshes: 設計図を借りる倉庫。所有権は受け取らない。nullptr 不可。
	// 作った自キャラより長く生きていること（形を借りるため）。
	explicit PlayerFactory( MeshCache* meshes );

	// 自キャラの形の構築が済んだか（失敗した・まだなら false。銃の形は無くても素手で出せるので見ない）。
	bool isReady() const;
	// 銃と自キャラを組み立てて world へ spawn し、自キャラの物を返す（銃の形の構築に失敗していたら素手）。isReady の後に呼ぶ。
	// position / yawDegree: 足元の位置と向き（度。0 は +Z）。
	GameObject* spawn( GameObjectWorld* world, const GameLib::Math::Vector3& position, float yawDegree ) const;

private:
	// 銃を組み立てて world へ spawn し、返す（spawn から）。形の構築に失敗していたら置かずに nullptr。
	GameObject* spawnWeapon( GameObjectWorld* world ) const;

	const SkinnedMeshData* mPlayerData;   // どれも MeshCache から借りる
	const StaticMeshData* mWeaponData;    // 自キャラが右手に持つ銃（弾倉を除いた形）
	const StaticMeshData* mMagazineData;  // 銃の弾倉（リロードで外れる）
};

#endif
