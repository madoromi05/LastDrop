#ifndef INCLUDED_SKINNED_MESH_DATA_H
#define INCLUDED_SKINNED_MESH_DATA_H

#include "GameLib/Base/Array.h"
#include "GameLib/Math/Vector2.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix34.h"
#include "GameLib/Graphics/VertexBuffer.h"
#include "GameLib/Graphics/IndexBuffer.h"
#include "GameLib/Scene/Container.h"
#include "GameLib/Scene/Tree.h"
#include "GameLib/Scene/Animation.h"
#include "MeshMaterial.h"
#include <string>
#include <vector>

using namespace GameLib;
using namespace GameLib::Math;

// CPU線形ブレンドスキニング(LBS)のスキンメッシュの「設計図」。1体ぶんの状態は持たず、同じキャラ全員で共有する。
// 持つもの: レスト姿勢の頂点＋ボーン重み＋bindInverse、インデックス、見た目、骨格とアニメの入れ物(Container)。
// 見た目は「パーツ」（インデックスの範囲＋MeshMaterial）単位で持つ。体と髪でテクスチャが別のモデルは複数パーツになる。
// 頂点・スキニングは全パーツ共通なので、パーツが増えても変形の負荷は増えない（描画呼び出しが増えるだけ）。
// 1体ごとのポーズ・変形後の頂点・位置は SkinnedMeshInstance が持ち、deform() / draw() にこのデータを使う。
// ゾンビを何体出しても、頂点・重み・テクスチャ・アニメは1つで済む。
//
// 使い方: create → set 系 → setContainer → finish（ここまでローダが行う）→ インスタンスから deform / draw
class SkinnedMeshData{
public:
	static const int MAX_INFLUENCE = 4; //1頂点が影響を受けるボーン数の上限

	SkinnedMeshData();
	~SkinnedMeshData();

	// ── 構築（ローダが使う）──
	// 頂点数・インデックス数・ボーン数・パーツ数を確保する。partNumber は1以上。
	void create( int vertexNumber, int indexNumber, int boneNumber, int partNumber );
	// ボーンに対応する Tree のノード名を設定する。
	void setBone( int boneIndex, const char* nodeName );
	// 頂点のレスト姿勢(モデル空間)を設定する。
	void setVertex( int i, const Vector3& position, const Vector3& normal, const Vector2& uv );
	// 頂点の影響ボーンと重みを設定する(slot: 0..MAX_INFLUENCE-1)。
	void setInfluence( int vertexIndex, int slot, int boneIndex, float weight );
	// 頂点カラーを設定する(0..1)。テクスチャ無しでも部位ごとに色を付けられる。
	void setColor( int i, float r, float g, float b );
	// 三角形インデックスを設定する。
	void setIndex( int i, unsigned short value );
	// パーツが使うインデックスの範囲を設定する。firstIndex / indexNumber は3の倍数（三角形単位）。
	void setPart( int partIndex, int firstIndex, int indexNumber );
	// パーツの見た目(テクスチャ・法線マップ・材質値)。返す参照の寿命はこのデータと同じ。所有権は移らない。
	MeshMaterial& material( int partIndex );
	// 骨格(<Tree>)とアニメ(<Animation>)の入れ物。インスタンスはここから自分の骨格を作る。
	void setContainer( Scene::Container );
	// 設定し終えたら呼ぶ。インデックスをGPUへ送り、レスト姿勢から bindInverse を確定する。
	// 骨格が無ければ false（理由はデバッグ出力）。
	bool finish();

	// ── 利用（インスタンスが使う）──
	// finish 済みで使える状態か。
	bool isReady() const;
	int vertexNumber() const;
	int boneNumber() const;
	// 1体ぶんの骨格を新しく作る（ポーズは体ごとに別なので共有しない）。
	Scene::Tree createSkeleton() const;
	// 名前でアニメを探す。無ければ空の Animation（if( a ) で判定できる）。
	Scene::Animation animation( const char* name ) const;
	// 非同期ロード中のテクスチャの読み込みを進める。描画するフレームごとに1回呼ぶ。
	void updateLoading();

	// pose の現在ポーズから、各ボーンの行列（骨格の根から見た姿勢）を bonePose へ書く。
	// ボーンの並びは setBone の番号。Tree に無いボーンは単位行列。
	// ポーズを行列で取り出すのは、2つのポーズを混ぜて（クロスフェード）から変形できるようにするため。
	// bonePose: boneNumber() 個ぶん。呼び出し側が持つ。nullptr 不可。
	void computeBonePose( Scene::Tree& pose, std::vector< Matrix34 >* bonePose ) const;
	// ボーンの姿勢 bonePose を root（置き場所と向き）へ置いて CPU スキニングし、変形後の頂点を out へ書く（ワールド空間）。
	// skinWork: ボーン数ぶんの作業領域。呼び出し側(インスタンス)が持つ。nullptr 不可。
	// out: vertexNumber() 頂点ぶんの頂点バッファ。呼び出し側が持つ。
	// 頂点数ぶんの行列演算をするので、キャラの数だけ CPU 負荷が増える（GPUスキニング化の候補）。
	void deform( const Matrix34& root, const std::vector< Matrix34 >& bonePose,
		std::vector< Matrix34 >* skinWork, Graphics::VertexBuffer out ) const;
	// deform 済みの頂点バッファを、このデータのインデックスと見た目で描く（パーツごとに1回ずつ描画する）。
	void draw( Graphics::VertexBuffer deformed ) const;

private:
	// 1頂点分のレストデータと影響ボーン。
	struct Vertex{
		Vertex();
		Vector3 mPosition;
		Vector3 mNormal;
		Vector2 mUv;
		unsigned mColor; //頂点カラー(既定 白。テクスチャに乗算される)
		int mBoneIndex[ MAX_INFLUENCE ];
		float mWeight[ MAX_INFLUENCE ];
	};
	// 1ボーン分。名前で Tree のノードと対応し、bindInverse を持つ。
	struct Bone{
		std::string mNodeName;
		Matrix34 mBindInverse;
	};

	// 見た目1つ分。インデックスバッファのうち、この範囲の三角形をこの見た目で描く。
	struct Part{
		Part();
		int mFirstTriangle;
		int mTriangleNumber;
		MeshMaterial mMaterial;
	};

	Array< Vertex > mVertices;
	Array< Bone > mBones;
	Array< unsigned short > mStagingIndices; // 構築中だけ使う。finish 後は空
	Graphics::IndexBuffer mIndexBuffer; // 全インスタンス・全パーツで共有（形のつながりは変形しても同じ）
	Array< Part > mParts;
	Scene::Container mContainer;
	bool mReady;
};

#endif
