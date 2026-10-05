#ifndef INCLUDED_STATIC_MESH_INSTANCE_H
#define INCLUDED_STATIC_MESH_INSTANCE_H

#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix34.h"

class StaticMeshData;

// 動かないメッシュを「どこに・どの向きで・どの大きさで」置くか。1軒1軒がこのクラス1つ。
// 形と見た目は StaticMeshData を借りて描く（所有しない）。同じ建物を並べるときは、
// 1つの StaticMeshData を複数のインスタンスで指せばよい。
class StaticMeshInstance{
public:
	// data: 描く形と見た目。所有権は受け取らない。nullptr 不可。
	//       このインスタンスより長く生きていること（今はシーンのメンバ、将来はキャッシュが持つ）。
	explicit StaticMeshInstance( const StaticMeshData* data );

	void setPosition( const GameLib::Math::Vector3& );	// 置き場所（ワールド座標）
	void setAngleY( float degree );						// Y軸まわりの向き（度）
	void setScale( float );								// 全体の拡大率（1で等倍）

	// 置き場所・向き・大きさを反映して描く。データの upload 前なら何も描かない。
	void draw() const;
	// ワールドの線分 origin〜origin+delta が一番早く当たる所の時刻（0〜1）と法線（ワールド・長さ1）。当たらなければ false。
	// time / normal: 出力先。nullptr 不可。
	bool raycast( const GameLib::Math::Vector3& origin, const GameLib::Math::Vector3& delta,
		float* time, GameLib::Math::Vector3* normal ) const;

private:
	GameLib::Math::Matrix34 worldMatrix() const; // 置き場所・向き・大きさ（draw と raycast で同じ式を使う）

	const StaticMeshData* mData;
	GameLib::Math::Vector3 mPosition;
	float mAngleY;
	float mScale;
};

#endif
