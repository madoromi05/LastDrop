#ifndef INCLUDED_HELD_ITEM_H
#define INCLUDED_HELD_ITEM_H

#include "GameLib/Math/Matrix34.h"

class StaticMeshData;
class SkinnedMeshInstance;

// キャラの骨に付けて描く、動かないメッシュ（手に持った銃など）。
// 持ち主の骨の行列に「持ち方」（骨から見た置き方）を掛けた場所へ、毎フレーム描き直すだけ。
// 持ち主の動き・アニメは知らない（骨の行列を読むだけ）ので、持たせる相手を変えてもこのクラスは変わらない。
//
// 持ち方は骨の空間で表すので、骨格の縮尺（Mixamo は cm）とメッシュの縮尺（m）の違いも持ち方に含める。
class HeldItem{
public:
	// data: 形と見た目。所有権は受け取らない。nullptr 不可。このインスタンスより長く生きていること（MeshCache が持つ）。
	// boneIndex: 付ける骨（SkinnedMeshData::findBone の番号）。
	// grip: 骨の空間での置き方。メッシュの頂点をこれで骨の空間へ移す。
	HeldItem( const StaticMeshData* data, int boneIndex, const GameLib::Math::Matrix34& grip );

	// holder の骨の今の姿勢に付けて描く。holder の update の後に呼ぶ（まだポーズが無ければ描かない）。
	void draw( const SkinnedMeshInstance& holder ) const;

private:
	const StaticMeshData* mData;
	int mBoneIndex;
	GameLib::Math::Matrix34 mGrip;
};

#endif
