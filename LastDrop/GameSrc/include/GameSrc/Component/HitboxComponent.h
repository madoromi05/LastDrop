#ifndef INCLUDED_HITBOX_COMPONENT_H
#define INCLUDED_HITBOX_COMPONENT_H

#include "GameSrc/World/Component.h"
#include "GameSrc/World/IWorldMember.h"
#include "GameSrc/Hit/IHittable.h"
#include "GameSrc/Hit/HitboxLayout.h"
#include "GameSrc/Hit/HitboxSet.h"
#include <memory>

class SkinnedMeshData;

// 部位ごとの命中の当たり判定（HitboxSet）。自キャラ・ゾンビが持つ。命中の名簿（HitWorld）へ自分で載る（IWorldMember）。
// 箱を今のポーズへ置き直す（update）のは、その物の動きを決める部品（アニメを進めた後に呼ぶ）。
class HitboxComponent : public Component, public IHittable, public IWorldMember{
public:
	// 部位の箱の設計図を借りる（ゾンビ。同じ種類の全員で共有する ZombieModel の物）。layout: nullptr 不可。この部品より長く生きていること。
	explicit HitboxComponent( const HitboxLayout* layout );
	// 部位の箱の設計図を data から作って自分で持つ（自キャラ。1 人だけなので共有しない）。
	// data: 骨を引く形。nullptr 不可。boneNamePrefix: 骨の名前の頭の共通部分（"mixamorig:"）。
	HitboxComponent( const SkinnedMeshData* data, const char* boneNamePrefix );

	HitboxSet& boxes(){ return mHitboxes; }

	// IHittable
	virtual const HitboxSet& hitboxes() const;
	// IWorldMember（命中の名簿にだけ載る）
	virtual void addTo( CollisionWorld* collisionWorld, HitWorld* hitWorld );
	virtual void removeFrom( CollisionWorld* collisionWorld, HitWorld* hitWorld );

private:
	std::unique_ptr< HitboxLayout > mOwnedLayout; // 自分で作った設計図（借りるときは空）。mHitboxes が借りるので先に宣言する
	HitboxSet mHitboxes;
};

#endif
