#ifndef INCLUDED_SKINNED_MESH_SHAPE_H
#define INCLUDED_SKINNED_MESH_SHAPE_H

#include "GameLib/Math/Vector3.h"
#include <vector>

class SkinnedMeshData;

// スキンメッシュの設計図（SkinnedMeshData）のレスト姿勢の頂点から、当たり判定の形を決めるための問い合わせ
// 部位ごとの当たり判定の箱（HitboxLayout）と、移動の当たり判定の大きさ（BodyFootprint）を、モデルの形に沿わせて作るのに使う
// 設計図の公開の口（頂点のレスト位置・一番強く付いている骨・bindInverse）だけを読む。読み込みの後に 1 回ずつ呼ぶ物（毎フレームは呼ばない）
class SkinnedMeshShape{
public:
	// 一番強く付いている骨が boneMask で true の頂点を集め、anchorBone の空間（レスト姿勢で bindInverse を掛けた所）へ移して out に入れる
	// boneMask: boneNumber() 個ぶん。対象の骨なら true。anchorBone: 箱を置く骨の番号。out: 出力先（前の中身は捨てる）。nullptr 不可
	static void collectBoneSpaceVertices( const SkinnedMeshData& data, const std::vector< bool >& boneMask, int anchorBone,
		std::vector< GameLib::Math::Vector3 >* out );
	static GameLib::Math::Vector3 boneSpaceJointPosition( const SkinnedMeshData& data, int anchorBone, int bone );	// bone の関節の位置（レスト姿勢）を、anchorBone の空間で返す。部位の箱の長い辺を、関節を結ぶ向きに合わせるのに使う
	// 一番強く付いている骨が boneMask で true の頂点を、レスト姿勢のモデルの空間（置き場所・向きを掛ける前。単位は表示と同じ m）で囲む
	// boundsMin / boundsMax: 出力先。nullptr 不可。対象の頂点が1つも無ければ false（出力は変えない）
	static bool computeRestBounds( const SkinnedMeshData& data, const std::vector< bool >& boneMask,
		GameLib::Math::Vector3* boundsMin, GameLib::Math::Vector3* boundsMax );
};

#endif
