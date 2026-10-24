#ifndef INCLUDED_STATIC_MESH_DATA_H
#define INCLUDED_STATIC_MESH_DATA_H

#include "GameLib/Base/Array.h"
#include "GameLib/Math/Matrix34.h"
#include "GameSrc/Render/StaticMesh/StaticMeshPart.h"
#include <vector>

// 動かないメッシュ（建物など）の「設計図」。形と見た目を1つだけ持ち、複数の配置で共有する。
// 置き場所は持たない（StaticMeshInstance が持つ）。同じ建物を何軒置いても、頂点やテクスチャは1つで済む。
// 見た目の違う部分（テクスチャ付きの壁／単色の窓枠など）はパーツに分けて持ち、まとめて描く。
class StaticMeshData{
public:
	StaticMeshData();
	~StaticMeshData();

	// 全パーツが、GPUへ送った後も CPU 側の頂点を残すようにする（StaticMeshPart::keepSourceVertices）。create より前に呼ぶ。
	// 何枚も置いて1つのメッシュへまとめる元にするデータだけに使う（MeshCache::staticMeshForMerging が呼ぶ）。
	void keepSourceVertices();
	bool keepsSourceVertices() const;
	// パーツ数を確保する。ローダが最初に1回だけ呼ぶ。
	void create( int partNumber );
	int partNumber() const;
	// パーツを取り出す。返す参照の寿命はこのデータと同じ。所有権は移らない。
	StaticMeshPart& part( int index );
	const StaticMeshPart& part( int index ) const;

	// 全パーツが描画できる状態か（upload 済みか）。パーツが無ければ false。
	bool isReady() const;
	// 非同期ロード中のテクスチャの読み込みを進める。描画するフレームごとに1回呼ぶ。全パーツのテクスチャが展開済みなら true。
	bool updateLoading();
	// world の位置・向き・大きさで全パーツを描く。
	void draw( const GameLib::Math::Matrix34& world ) const;
	// 全パーツのうち、線分が一番早く当たる所の時刻と法線。座標はモデルの空間（置く前）。当たらなければ false。
	bool raycast( const GameLib::Math::Vector3& origin, const GameLib::Math::Vector3& delta,
		float* time, GameLib::Math::Vector3* normal ) const;
	// 全パーツの三角形の隅を、3つずつ corners の後ろへ足す（モデルの空間。置く前）。
	// 形に沿った移動の当たり判定を作るため（MeshWallBuilder）。corners: 出力先。nullptr 不可。
	void appendTriangles( std::vector< GameLib::Math::Vector3 >* corners ) const;

	// 全パーツの頂点を囲む箱（モデルのローカル座標）。当たり判定の箱の大きさに使う。
	// ローダが頂点を読むたびに extendBounds で広げる。頂点が1つも無いうちは min > max のまま。
	void extendBounds( const GameLib::Math::Vector3& position );
	const GameLib::Math::Vector3& boundsMin() const;
	const GameLib::Math::Vector3& boundsMax() const;
	// 歩いて越えられない高さにある頂点だけを囲む箱（モデルのローカル座標。モデルの原点が地面にある前提）。
	// 移動の当たり判定の箱の広さに使う。足元の薄い物（歩道の板など）は上を歩けるので、ここには入らない。
	// 壁は上の頂点が高いので入る。全部の頂点が低いメッシュでは、全体の箱（boundsMin / boundsMax）を返す。
	const GameLib::Math::Vector3& blockingBoundsMin() const;
	const GameLib::Math::Vector3& blockingBoundsMax() const;

private:
	// 歩いて越えられない高さの頂点が1つでもあったか。
	bool hasBlockingBounds() const;

	GameLib::Array< StaticMeshPart > mParts;
	GameLib::Math::Vector3 mBoundsMin;
	GameLib::Math::Vector3 mBoundsMax;
	GameLib::Math::Vector3 mBlockingBoundsMin;
	GameLib::Math::Vector3 mBlockingBoundsMax;
	bool mKeepsSourceVertices; // 作るパーツに CPU 側の頂点を残させるか
};

#endif
