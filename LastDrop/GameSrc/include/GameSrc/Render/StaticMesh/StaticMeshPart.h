#ifndef INCLUDED_STATIC_MESH_PART_H
#define INCLUDED_STATIC_MESH_PART_H

#include "GameLib/Base/Array.h"
#include "GameLib/Math/Vector2.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix34.h"
#include "GameLib/Graphics/Vertex.h"
#include "GameLib/Graphics/VertexBuffer.h"
#include "GameLib/Graphics/IndexBuffer.h"
#include "GameSrc/Render/MeshMaterial.h"
#include "GameSrc/Hit/TriangleMesh.h"

// 動かないメッシュのパーツ1つ。「1つの見た目（MeshMaterial）で1回に描ける頂点とインデックス」のまとまり
// 例: 建物の「テクスチャを貼ったレンガ壁」と「単色の窓枠・ガラス」は別パーツ。StaticMeshData が束ねる
//
// 使い方: create → setVertex/setColor/setIndex → upload（ここまでローダが行う）→ draw(ワールド行列)
// 頂点は upload で一度だけGPUへ送り、CPU側の作業用配列は捨てる（SkinnedMesh と違い毎フレームの更新は無い）
// ただし弾の当たり判定のため、位置とつながりだけは TriangleMesh に写して残す
// 何枚も置いた物を1つのメッシュへまとめる元にするとき（StaticMeshMerger）だけは、keepSourceVertices で作業用配列も残す
class StaticMeshPart{
public:
	StaticMeshPart();
	~StaticMeshPart();

	// 頂点数・インデックス数を確保する
	void create( int vertexNumber, int indexNumber );
	void setVertex( int i, const GameLib::Math::Vector3& position, const GameLib::Math::Vector3& normal, const GameLib::Math::Vector2& uv );
	void setColor( int i, float r, float g, float b );		// 頂点カラー(0..1)。既定は白
	void setColor( int i, unsigned packedColor );			// 頂点カラー（VertexColor::pack の並びのまま。別のパーツから写すとき用）
	void setIndex( int i, unsigned short value );			// 三角形インデックス
	MeshMaterial& material();								// 見た目。寿命はこのデータと同じ
	const MeshMaterial& material() const;
	// upload の後も、CPU側の頂点とインデックスを捨てずに残す（sourceVertex / sourceIndex で読める）。upload より前に呼ぶ
	// まとめる元になる小さい部品だけに使う（全部のメッシュで残すと、頂点の数だけメモリを使い続ける）
	void keepSourceVertices();

	void upload();			// 設定し終えた頂点とインデックスをGPUへ送る。以降 set 系は呼べない
	bool isReady() const;	// 描画できる状態か（upload 済みか）

	bool updateLoading();										// 非同期ロード中のテクスチャの読み込みを進める。描画するフレームごとに1回呼ぶ。テクスチャが展開済みなら true
	void draw( const GameLib::Math::Matrix34& world ) const;	// world の位置・向き・大きさで描画する。upload 前なら何もしない
	// instanceNumber 個まとめて描画する（インスタンシング）。upload 前なら何もしない
	// シェーダ・置き場所・ワールド行列は StaticMeshInstancing が付けておく（ここでは選ばない）
	void drawInstanced( int instanceNumber ) const;
	// 弾の当たり判定。座標はモデルの空間（置く前）。意味は TriangleMesh::raycast と同じ。upload 前なら当たらない
	bool raycast( const GameLib::Math::Vector3& origin, const GameLib::Math::Vector3& delta,
		float* time, GameLib::Math::Vector3* normal ) const;
	void appendTriangles( std::vector< GameLib::Math::Vector3 >* corners ) const;	// 全部の三角形の隅を、3つずつ corners の後ろへ足す（モデルの空間）。upload 前なら何も足さない。corners: 出力先。nullptr 不可

	// ── まとめる元として読む（keepSourceVertices していなければ、upload の後は数が 0）──
	int sourceVertexNumber() const;
	const GameLib::Graphics::Vertex& sourceVertex( int i ) const;	// モデルの空間（置く前）の頂点
	int sourceIndexNumber() const;
	unsigned short sourceIndex( int i ) const;

private:
	void bindForDraw() const;	// 材質（テクスチャ・材質値）・頂点・インデックスを付ける。draw と drawInstanced の共通の準備

	// 構築中だけ使う作業用配列。upload 後は空になる（mKeepsSource なら残す）
	GameLib::Array< GameLib::Graphics::Vertex > mStagingVertices;
	GameLib::Array< unsigned short > mStagingIndices;

	GameLib::Graphics::VertexBuffer mVertexBuffer;
	GameLib::Graphics::IndexBuffer mIndexBuffer;
	MeshMaterial mMaterial;
	TriangleMesh mShape; // 弾の当たり判定用に、upload で位置とつながりだけ残したもの
	int mTriangleNumber; // upload 時に確定する。0 のうちは未 upload
	bool mKeepsSource;   // upload の後も作業用配列を残すか（keepSourceVertices）
};

#endif
