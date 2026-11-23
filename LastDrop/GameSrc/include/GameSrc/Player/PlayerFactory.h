#ifndef INCLUDED_PLAYER_FACTORY_H
#define INCLUDED_PLAYER_FACTORY_H

#include "GameLib/Math/Vector3.h"
#include "GameSrc/Weapon/WeaponCatalog.h"

class MeshCache;
class SkinnedMeshData;
class StaticMeshData;
class GameObject;
class GameObjectWorld;
class ItemGrid;

// 自キャラと、自キャラが持てる銃（拳銃・ライフル。WeaponCatalog の全部）を部品から組み立てる（Unity のプレハブの代わり。ZombieFactory と同じ役）
// 部品の並び・形のファイル・名前を 1 か所で決める
// どこに置くか（TownMap::playerStart）と、置いた後に読むのは GameWorld
// 銃は全部ここで 1 丁ずつ作って HoldComponent へ渡す。どれを手に持つかはカバンの中身で決まる（始めは拳銃）
// 銃の物は捨てても消えず、弾倉の中身を持ち続ける。捨てている間は、同じ物へ PropFactory が地面に置くための部品を付ける
class PlayerFactory{
public:
	// 形の読み込みを頼むだけ（非同期）。meshes: 設計図を借りる倉庫。所有権は受け取らない。nullptr 不可
	// 作った自キャラより長く生きていること（形を借りるため）
	explicit PlayerFactory( MeshCache* meshes );

	bool isReady() const;	// 自キャラの形の構築が済んだか（失敗した・まだなら false。銃の形は無くても素手で出せるので見ない）
	// 銃と自キャラを組み立てて world へ spawn し、自キャラの物を返す（形の構築に失敗した銃は作らない＝持てない）。isReady の後に呼ぶ
	// position / yawDegree: 足元の位置と向き（度。0 は +Z）。startBag: 始めのカバンの中身（自キャラの持ち物が写して持つ）
	GameObject* spawn( GameObjectWorld* world, const GameLib::Math::Vector3& position, float yawDegree, const ItemGrid& startBag ) const;

private:
	GameObject* spawnWeapon( GameObjectWorld* world, WeaponId id ) const;	// id の銃を組み立てて world へ spawn し、返す（spawn から）。形の構築に失敗していたら置かずに nullptr

	const SkinnedMeshData* mPlayerData;   // どれも MeshCache から借りる
	const StaticMeshData* mWeaponData[ WEAPON_NUMBER ];   // 銃ごとの、右手に持つ形（弾倉を除いた形）
	const StaticMeshData* mMagazineData[ WEAPON_NUMBER ]; // 銃ごとの弾倉（リロードで外れる）
};

#endif
