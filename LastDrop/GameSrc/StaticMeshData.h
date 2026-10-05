#ifndef INCLUDED_STATIC_MESH_DATA_H
#define INCLUDED_STATIC_MESH_DATA_H

#include "GameLib/Base/Array.h"
#include "GameLib/Math/Matrix34.h"
#include "StaticMeshPart.h"

// 動かないメッシュ（建物など）の「設計図」。形と見た目を1つだけ持ち、複数の配置で共有する。
// 置き場所は持たない（StaticMeshInstance が持つ）。同じ建物を何軒置いても、頂点やテクスチャは1つで済む。
// 見た目の違う部分（テクスチャ付きの壁／単色の窓枠など）はパーツに分けて持ち、まとめて描く。
class StaticMeshData{
public:
	StaticMeshData();
	~StaticMeshData();

	// パーツ数を確保する。ローダが最初に1回だけ呼ぶ。
	void create( int partNumber );
	int partNumber() const;
	// パーツを取り出す。返す参照の寿命はこのデータと同じ。所有権は移らない。
	StaticMeshPart& part( int index );

	// 全パーツが描画できる状態か（upload 済みか）。パーツが無ければ false。
	bool isReady() const;
	// 非同期ロード中のテクスチャの読み込みを進める。描画するフレームごとに1回呼ぶ。
	void updateLoading();
	// world の位置・向き・大きさで全パーツを描く。
	void draw( const GameLib::Math::Matrix34& world ) const;
	// 全パーツのうち、線分が一番早く当たる所の時刻と法線。座標はモデルの空間（置く前）。当たらなければ false。
	bool raycast( const GameLib::Math::Vector3& origin, const GameLib::Math::Vector3& delta,
		float* time, GameLib::Math::Vector3* normal ) const;

	// 全パーツの頂点を囲む箱（モデルのローカル座標）。当たり判定の箱の大きさに使う。
	// ローダが頂点を読むたびに extendBounds で広げる。頂点が1つも無いうちは min > max のまま。
	void extendBounds( const GameLib::Math::Vector3& position );
	const GameLib::Math::Vector3& boundsMin() const;
	const GameLib::Math::Vector3& boundsMax() const;

private:
	GameLib::Array< StaticMeshPart > mParts;
	GameLib::Math::Vector3 mBoundsMin;
	GameLib::Math::Vector3 mBoundsMax;
};

#endif
