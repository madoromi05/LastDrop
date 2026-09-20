#ifndef INCLUDED_SKINNED_MESH_H
#define INCLUDED_SKINNED_MESH_H

#include "GameLib/Base/Array.h"
#include "GameLib/Math/Vector2.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix34.h"
#include "GameLib/Graphics/VertexBuffer.h"
#include "GameLib/Graphics/IndexBuffer.h"
#include "GameLib/Graphics/Texture.h"
#include <string>

namespace GameLib{
	namespace Scene{
		class Tree;
	}
}
using namespace GameLib;
using namespace GameLib::Math;

// CPU線形ブレンドスキニング(LBS)のスキンメッシュ。
// レスト姿勢の頂点＋ボーン重み＋bindInverse を保持し、借用した Scene::Tree の
// 現在ポーズで毎フレームCPU変形して動的頂点バッファへ書き、1回で描画する。
// Tree/Animator は所有しない(借用)。ファイル形式には依存しない(構築はAPI経由)。
class SkinnedMesh{
public:
	static const int MAX_INFLUENCE = 4; //1頂点が影響を受けるボーン数の上限

	SkinnedMesh();
	~SkinnedMesh();

	// 頂点数・インデックス数・ボーン数を確保する。
	void create( int vertexNumber, int indexNumber, int boneNumber );
	// ボーンに対応する Tree のノード名を設定する。
	void setBone( int boneIndex, const char* nodeName );
	// 頂点のレスト姿勢(モデル空間)を設定する。
	void setVertex( int i, const Vector3& position, const Vector3& normal, const Vector2& uv );
	// 頂点の影響ボーンと重みを設定する(slot: 0..MAX_INFLUENCE-1)。
	void setInfluence( int vertexIndex, int slot, int boneIndex, float weight );
	// 三角形インデックスを設定する。
	void setIndex( int i, unsigned short value );
	// テクスチャを設定する(省略時は無地)。
	void setTexture( Graphics::Texture );

	// レスト姿勢(無アニメ)の Tree から bindInverse を計算する。
	// 注意: skeleton のアニメをレストに戻す。呼び出し後に実行用アニメを設定すること。
	void bindPose( Scene::Tree& skeleton );
	// skeleton の現在ポーズでCPUスキニングし、動的VBを更新する(描画前に毎フレーム呼ぶ)。
	void update( Scene::Tree& skeleton );
	// 更新済みの動的VBで描画する(ワールド=単位)。
	void draw() const;

private:
	// 1頂点分のレストデータと影響ボーン。
	struct Vertex{
		Vertex();
		Vector3 mPosition;
		Vector3 mNormal;
		Vector2 mUv;
		int mBoneIndex[ MAX_INFLUENCE ];
		float mWeight[ MAX_INFLUENCE ];
	};
	// 1ボーン分。名前で Tree のノードと対応し、bindInverse を持つ。
	struct Bone{
		std::string mNodeName;
		Matrix34 mBindInverse;
	};

	void uploadIndices(); //インデックスを一度だけIBへ転送する

	Array< Vertex > mVertices;
	Array< Bone > mBones;
	Array< Matrix34 > mSkinMatrices; //毎フレームの skin[b] = now * bindInverse
	Array< unsigned short > mIndices;

	Graphics::VertexBuffer mVertexBuffer; //動的。毎フレーム書き換える
	Graphics::IndexBuffer mIndexBuffer;
	Graphics::Texture mTexture;

	bool mIndicesUploaded;
};

#endif
