#ifndef INCLUDED_SKINNED_MESH_INSTANCE_H
#define INCLUDED_SKINNED_MESH_INSTANCE_H

#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix34.h"
#include "GameLib/Graphics/VertexBuffer.h"
#include "GameLib/Scene/Tree.h"
#include <vector>

class SkinnedMeshData;

// スキンメッシュを置いた「1体」。自分の骨格（ポーズ）・変形後の頂点・位置と向きを持つ。
// 頂点・重み・テクスチャ・アニメは SkinnedMeshData を借りる（所有しない）。
// 同じキャラを何体出すときも、データは1つでインスタンスだけ増やせばよい。
//
// 1体ごとに頂点バッファ（頂点数×約48バイト）と CPU スキニングの負荷がかかる。
// コピーすると骨格と頂点バッファを2体で共有してしまうので、コピーは禁止（std::unique_ptr 等で持つ）。
class SkinnedMeshInstance{
public:
	// data: 形と見た目。所有権は受け取らない。nullptr 不可、isReady() 済みであること。
	//       このインスタンスより長く生きていること（MeshCache が持つ）。
	explicit SkinnedMeshInstance( const SkinnedMeshData* data );
	SkinnedMeshInstance( const SkinnedMeshInstance& ) = delete;
	SkinnedMeshInstance& operator=( const SkinnedMeshInstance& ) = delete;

	void setPosition( const GameLib::Math::Vector3& );	// 置き場所（ワールド座標）
	void setAngleY( float degree );						// Y軸まわりの向き（度）
	// 再生するアニメを名前で設定する。見つからなければデバッグ出力してレスト姿勢のまま。
	// startTime: 再生開始時刻（フレーム）。同じアニメの複数体をずらして、動きが揃わないようにする。
	void setAnimation( const char* name, float startTime );

	// アニメを1フレーム進めて、ポーズどおりに頂点を変形する。描画前に毎フレーム1回呼ぶ。
	void update();
	// update で変形した頂点を描く。
	void draw() const;

private:
	const SkinnedMeshData* mData;
	GameLib::Scene::Tree mSkeleton;						// この体だけのポーズ
	std::vector< GameLib::Math::Matrix34 > mSkinMatrices;	// deform の作業領域（ボーン数ぶん）
	GameLib::Graphics::VertexBuffer mVertexBuffer;		// 変形後の頂点。毎フレーム書き換える
	GameLib::Math::Vector3 mPosition;
	float mAngleY;
};

#endif
