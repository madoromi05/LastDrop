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

class SkinningPipeline;
#include <string>
#include <vector>

using namespace GameLib;
using namespace GameLib::Math;

// 線形ブレンドスキニング(LBS)のスキンメッシュの「設計図」。変形は GPU で行う（SkinningPipeline）。1体ぶんの状態は持たず、同じキャラ全員で共有する。
// 持つもの: レスト姿勢の頂点＋ボーン重み（CPU 側と、GPU へ送った頂点バッファ）＋bindInverse、インデックス、見た目、骨格とアニメの入れ物(Container)。
// CPU 側の頂点は当たり判定の箱づくり（collectBoneSpaceVertices など）に使うので、GPU へ送った後も残す。
// 見た目は「パーツ」（インデックスの範囲＋MeshMaterial）単位で持つ。体と髪でテクスチャが別のモデルは複数パーツになる。
// 頂点・スキニングは全パーツ共通なので、パーツが増えても変形の負荷は増えない（描画呼び出しが増えるだけ）。
// 1体ごとのポーズ・位置は SkinnedMeshInstance が持ち、computeSkinMatrices() / draw() にこのデータを使う。
// ゾンビを何体出しても、頂点（GPU の頂点バッファも）・重み・テクスチャ・アニメは1つで済む。
//
// 使い方: create → set 系 → setContainer → finish（ここまでローダが行う）→ setSkinning（MeshCache が行う）→ インスタンスから draw
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
	// 設定し終えたら呼ぶ。頂点とインデックスをGPUへ送り、レスト姿勢から bindInverse を確定する。
	// 骨格が無い・骨が多すぎる（SkinningPipeline::kMaxBones を超える）なら false（理由はデバッグ出力）。
	bool finish();
	// 描くときに使う GPU スキニングの道具。所有権は受け取らない。このデータより長く生きていること（MeshCache が持つ）。
	void setSkinning( SkinningPipeline* skinning );

	// ── 利用（インスタンスが使う）──
	// finish 済みで使える状態か。
	bool isReady() const;
	int vertexNumber() const;
	int boneNumber() const;
	// Tree のノード名からボーン番号（setBone の番号）を探す。無ければ -1。
	// 武器を手に持たせるときなど、特定の骨の姿勢を読みたい側が使う（毎フレームではなく最初に1回引く）。
	int findBone( const char* nodeName ) const;
	// ボーン番号から Tree のノード名。範囲外なら空文字列。腕の骨を名前の頭でまとめて探すときなどに使う。
	const char* boneName( int boneIndex ) const;
	// 一番強く付いている骨が boneMask で true の頂点を集め、anchorBone の空間（レスト姿勢で bindInverse を掛けた所）へ移して out に入れる。
	// 部位ごとの当たり判定の箱を、モデルの形に沿わせて作るのに使う。
	// boneMask: boneNumber() 個ぶん。対象の骨なら true。anchorBone: 箱を置く骨の番号。out: 出力先（前の中身は捨てる）。nullptr 不可。
	void collectBoneSpaceVertices( const std::vector< bool >& boneMask, int anchorBone, std::vector< Vector3 >* out ) const;
	// bone の関節の位置（レスト姿勢）を、anchorBone の空間で返す。部位の箱の長い辺を、関節を結ぶ向きに合わせるのに使う。
	Vector3 boneSpaceJointPosition( int anchorBone, int bone ) const;
	// 一番強く付いている骨が boneMask で true の頂点を、レスト姿勢のモデルの空間（置き場所・向きを掛ける前。単位は表示と同じ m）で囲む。
	// 移動の当たり判定の大きさ（体を真上から囲む長方形）を、モデルの形から決めるのに使う。
	// boundsMin / boundsMax: 出力先。nullptr 不可。対象の頂点が1つも無ければ false（出力は変えない）。
	bool computeRestBounds( const std::vector< bool >& boneMask, Vector3* boundsMin, Vector3* boundsMax ) const;
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
	// ボーンの姿勢 bonePose を root（置き場所と向き）へ置いた、ボーンごとの変形の行列（置き場所×ポーズ×bindInverse）を out へ書く。
	// out: boneNumber() 個ぶん。呼び出し側（インスタンス）が持つ。nullptr 不可。計算はボーンの数だけ（頂点の数には比例しない）。
	void computeSkinMatrices( const Matrix34& root, const std::vector< Matrix34 >& bonePose, std::vector< Matrix34 >* out ) const;
	// skinMatrices（computeSkinMatrices の結果）で GPU 変形して、パーツごとに描く。
	// setSkinning されていない・シェーダが使えない（コンパイル失敗）なら描かない（理由は SkinningPipeline::errorMessage）。
	void draw( const std::vector< Matrix34 >& skinMatrices ) const;

private:
	// CPU 側の頂点（レスト姿勢＋骨の番号と重み）を、GPU スキニング用の頂点バッファへ写す。finish で1回だけ呼ぶ。
	void uploadGpuVertices();
	// collectBoneSpaceVertices / computeRestBounds の中身。対象の頂点を transform で移して out に入れる。
	void collectVertices( const std::vector< bool >& boneMask, const Matrix34& transform, std::vector< Vector3 >* out ) const;

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
	Graphics::VertexBuffer mGpuVertices; // レスト姿勢の頂点＋骨の番号と重み。全インスタンスで共有し、変形は GPU が行う
	Graphics::IndexBuffer mIndexBuffer; // 全インスタンス・全パーツで共有（形のつながりは変形しても同じ）
	Array< Part > mParts;
	Scene::Container mContainer;
	SkinningPipeline* mSkinning; // 借りている（MeshCache が持つ）。nullptr なら描かない
	bool mReady;
};

#endif
