#ifndef INCLUDED_WEAPON_MODEL_COMPONENT_H
#define INCLUDED_WEAPON_MODEL_COMPONENT_H

#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix34.h"
#include "GameSrc/World/Component.h"
#include "GameSrc/Weapon/WeaponCatalog.h"

class StaticMeshData;

// 銃の見た目と、手にどう持つか（画像の ModelComponent）。持ち方の値は銃ごとの表（WeaponCatalog の WeaponGrip）にある
// 銃の形（弾倉を除いた物）と弾倉の形、右手の骨から見た持ち方、覗くときに目を置く点、銃口の点を持つ
// 自分では描かない。持つ側（HoldComponent）が、自分の手の骨の今の姿勢からこの持ち方で置いて描く（銃は手に付いて動くため）
class WeaponModelComponent : public Component{
public:
	// weapon: 銃の形（WeaponSpec::modelFile。弾倉を除いた形）。nullptr 不可
	// magazine: 弾倉の形（WeaponSpec::magazineFile。銃と同じ座標）。nullptr なら弾倉を描かない（銃に穴が開いて見えるだけ）
	// どちらも所有権は受け取らない。isReady() 済みであること。この部品より長く生きていること（MeshCache が持つ）
	// grip: 手にどう持つか・覗く点・銃口（表の行の値。表は動かないので借りるだけ）
	WeaponModelComponent( const StaticMeshData* weapon, const StaticMeshData* magazine, const WeaponGrip& grip );

	const StaticMeshData* weapon() const;
	const StaticMeshData* magazine() const;																								// 無ければ nullptr
	GameLib::Math::Matrix34 gripInHand() const;																							// 銃のモデル座標 → 右手の骨の空間 の行列（握りの中心を手に合わせる）
	const GameLib::Math::Vector3& sightEye() const;																						// 覗いたときに目を置く点（銃のモデル座標）
	void bounds( GameLib::Math::Vector3* boundsMin, GameLib::Math::Vector3* boundsMax ) const;											// 銃と弾倉を合わせて囲む箱（銃のモデル座標）。持つ側の移動の当たり判定を、銃口の先まで伸ばすのに使う
	void muzzle( const GameLib::Math::Matrix34& placement, GameLib::Math::Vector3* position, GameLib::Math::Vector3* direction ) const;	// placement（銃のモデル座標 → ワールド。HeldItem::placement）に置いたときの、銃口の位置（ワールド）と向き（長さ1）

private:
	const StaticMeshData* mWeapon;
	const StaticMeshData* mMagazine;
	const WeaponGrip* mGrip; // 借りているだけ
};

#endif
