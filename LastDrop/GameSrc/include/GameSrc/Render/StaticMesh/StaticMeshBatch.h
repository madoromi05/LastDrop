#ifndef INCLUDED_STATIC_MESH_BATCH_H
#define INCLUDED_STATIC_MESH_BATCH_H

#include "GameLib/Math/Matrix34.h"
#include <vector>

class StaticMeshData;
class StaticMeshInstance;
class StaticMeshInstancing;

// 置いた静的メッシュを形（StaticMeshData）ごとに集め、パーツごとに 1 回の描画で描く（インスタンシング。OptimizationA_Plan.txt 8-3）
// 1 つずつ描く（StaticMeshInstance::draw）のと比べ、描く回数が「置いた数 × パーツ数」から「形の数 × パーツ数」に減り、材質もパーツごとに 1 回だけ付ける
// 何を置いた物かは問わない（今は建物と置物。WorldRenderer）。描くかどうかの判定（DrawCulling）は足す側が済ませておく
// 使い方: 1 回描くごとに clear → add（描きたい順）→ draw
// 描く順：形ごとに、そのフレームで最初に足された順。同じ形の中では足した順（目に近い順に足せば、手前から描く順が保たれる）
class StaticMeshBatch{
public:
	StaticMeshBatch();
	StaticMeshBatch( const StaticMeshBatch& ) = delete;
	StaticMeshBatch& operator=( const StaticMeshBatch& ) = delete;

	void clear();																	// 集めた物を忘れる（形ごとの入れ物は残し、毎フレーム確保し直さない）
	void add( const StaticMeshInstance& instance );									// instance の形と置き場所を足す。instance の寿命は draw までで足りる（行列は写して持つ）
	void add( const StaticMeshInstance& instance, const StaticMeshData* shape );	// 形だけ shape に替えて足す（細かさの段 LOD。段ごとに別の形としてまとまる）。shape: nullptr 不可
	// 集めた物を描く。instancing: 描く道具（begin〜end は中で呼ぶ）。nullptr 不可。isReady() であること
	// 1 つの形を kMaxInstances より多く置いていれば、分けて描く
	void draw( StaticMeshInstancing* instancing ) const;

private:
	// 1 つの形と、その置き場所
	struct Group{
		const StaticMeshData* data;					// 借りているだけ
		std::vector< GameLib::Math::Matrix34 > placements;	// このフレームに足した置き場所（足した順）
	};
	int groupIndexOf( const StaticMeshData* data );	// data の入れ物の番号。まだ無ければ作る

	std::vector< Group > mGroups;	// 一度でも足された形（中身の無い物も残す）
	std::vector< int > mDrawOrder;	// このフレームに最初に足された順の mGroups の番号
};

#endif
