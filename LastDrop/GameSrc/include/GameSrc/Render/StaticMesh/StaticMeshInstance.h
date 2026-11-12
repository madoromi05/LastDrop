#ifndef INCLUDED_STATIC_MESH_INSTANCE_H
#define INCLUDED_STATIC_MESH_INSTANCE_H

#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix34.h"

class StaticMeshData;
class ViewFrustum;

// 動かないメッシュを「どこに・どの向きで・どの大きさで」置くか。1軒1軒がこのクラス1つ
// 形と見た目は StaticMeshData を借りて描く（所有しない）。同じ建物を並べるときは、
// 1つの StaticMeshData を複数のインスタンスで指せばよい
//
// 置き場所の行列・その逆行列・ワールドで囲む箱は、置き場所を変えたときに1回だけ求めて覚えておく
// 弾・視線の判定（raycast）は置いた物の数だけ毎フレーム呼ばれるので、そのたびに逆行列を作り直さないため
// 当たり判定を作る側（MeshWallBuilder / MeshBoundsBox）も、ここの行列をそのまま使う（見た目とずれない）
class StaticMeshInstance{
public:
	// data: 描く形と見た目。所有権は受け取らない。nullptr 不可、isReady() 済みであること（囲む箱を頂点の範囲から求めるため）
	//       このインスタンスより長く生きていること（MeshCache が持つ）
	// 原点・向き 0・等倍に置く
	explicit StaticMeshInstance( const StaticMeshData* data );
	StaticMeshInstance( const StaticMeshData* data, const GameLib::Math::Vector3& position, float angleY );	// position / angleY: 置き場所（ワールド座標）と向き（度）。等倍で置く

	void setPosition( const GameLib::Math::Vector3& );	// 置き場所（ワールド座標）
	void setAngleY( float degree );						// Y軸まわりの向き（度）
	void setScale( float );								// 全体の拡大率（1で等倍）

	const StaticMeshData* data() const;					// 借りている形と見た目
	const GameLib::Math::Vector3& position() const;		// 置き場所（ワールド座標）
	float angleY() const;								// 向き（度）
	float scale() const;								// 拡大率
	const GameLib::Math::Matrix34& worldMatrix() const;	// 置き場所・向き・大きさの行列（モデルの空間 → ワールド）。描画・raycast・当たり判定づくりが同じものを使う
	// 全頂点をワールドで囲む、軸に沿った箱（少し広げてある）。ますに分けて絞り込む側（TownGround）が使う
	const GameLib::Math::Vector3& boundsMin() const;
	const GameLib::Math::Vector3& boundsMax() const;

	void draw() const;								// 置き場所・向き・大きさを反映して描く
	void draw( const StaticMeshData* shape ) const;	// 形だけ shape に替えて、同じ置き場所で描く（細かさの段 LOD。shape は data と同じモデルの空間で作った物）。nullptr 不可
	// 上と同じだが、囲む箱が frustum（カメラに映る範囲）に少しも入らなければ描かない
	// 町のように、置いた物の多くが画面の外にある所で使う
	void draw( const ViewFrustum& frustum ) const;
	// ワールドの線分 origin〜origin+delta が一番早く当たる所の時刻（0〜1）と法線（ワールド・長さ1）。当たらなければ false
	// 囲む箱に触れない線分は、三角形を調べずに外れにする。time / normal: 出力先。nullptr 不可
	bool raycast( const GameLib::Math::Vector3& origin, const GameLib::Math::Vector3& delta,
		float* time, GameLib::Math::Vector3* normal ) const;

private:
	void updatePlacement();	// 置き場所・向き・大きさから、行列・逆行列・囲む箱を求め直す（置き場所を変えるたびに呼ぶ）

	const StaticMeshData* mData;
	GameLib::Math::Vector3 mPosition;
	float mAngleY;
	float mScale;
	// 以下は mPosition / mAngleY / mScale から求めた写し（updatePlacement だけが書く）
	GameLib::Math::Matrix34 mWorld;		// モデルの空間 → ワールド
	GameLib::Math::Matrix34 mToLocal;	// ワールド → モデルの空間（mWorld の逆）
	GameLib::Math::Vector3 mBoundsMin;	// 全頂点をワールドで囲む、軸に沿った箱（少し広げてある）
	GameLib::Math::Vector3 mBoundsMax;
};

#endif
