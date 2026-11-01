#ifndef INCLUDED_SKINNED_MODEL_COMPONENT_H
#define INCLUDED_SKINNED_MODEL_COMPONENT_H

#include "GameSrc/World/Component.h"
#include "GameSrc/Render/SkinnedMesh/SkinnedMeshInstance.h"

class SkinnedMeshData;

// 骨のある見た目（SkinnedMeshInstance）1 つ。自キャラ・ゾンビが持つ。
// アニメを進める・置き場所を写す・姿勢の書き換えを付けるのは、その物の動きを決める部品（PlayerComponent・ZombieBrainComponent）。
// ここは見た目を持って描くだけ（描くかどうか＝間引きは描く側の GameWorld が決める）。
class SkinnedModelComponent : public Component{
public:
	// data: 形・見た目・アニメ。所有権は受け取らない。nullptr 不可、isReady() 済みであること。この部品より長く生きていること。
	explicit SkinnedModelComponent( const SkinnedMeshData* data ) : mMesh( data ){}

	SkinnedMeshInstance& mesh(){ return mMesh; }
	const SkinnedMeshInstance& mesh() const { return mMesh; }
	void draw() const { mMesh.draw(); }

private:
	SkinnedMeshInstance mMesh;
};

#endif
