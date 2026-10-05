#ifndef INCLUDED_HIT_MATH_H
#define INCLUDED_HIT_MATH_H

#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix34.h"

// 命中判定（弾の通り道の線分と、部位の箱・建物などの三角形）の計算だけを持つ。状態は持たない。
// 箱は「-1〜1 の立方体を行列で置いたもの」として扱う。線分を箱の空間へ移せば、どんな向き・大きさの箱も
// この立方体との判定で済む（行列は移動・回転・拡大だけなので、線分上の時刻は空間を移しても変わらない）。
class HitMath{
public:
	// origin から delta だけ進む線分が、-1〜1 の立方体に最初に入る時刻（0〜1）を time に書いて true。
	// normal には入った面の外向きの法線（立方体の空間。長さ1の軸の向き）を書く。
	// 始点が立方体の中なら time=0 で当たり、入った面が無いので normal は 0 ベクトル（向きは呼び出し側が決める）。
	// time / normal: 出力先。nullptr 不可。
	static bool segmentUnitCube( const GameLib::Math::Vector3& origin, const GameLib::Math::Vector3& delta,
		float* time, GameLib::Math::Vector3* normal );
	// origin から delta だけ進む線分が、三角形 v0 v1 v2 に当たる時刻（0〜1）を time に書いて true（両面とも当たる）。
	// normal には、線分が来た側を向いた面の法線（長さ1）を書く。面積0の三角形には当たらない。
	// time / normal: 出力先。nullptr 不可。
	static bool segmentTriangle( const GameLib::Math::Vector3& origin, const GameLib::Math::Vector3& delta,
		const GameLib::Math::Vector3& v0, const GameLib::Math::Vector3& v1, const GameLib::Math::Vector3& v2,
		float* time, GameLib::Math::Vector3* normal );
	// origin から delta だけ進む線分が、boundsMin〜boundsMax の軸に沿った箱に触れるか（時刻は要らない、絞り込み用）。
	static bool segmentTouchesBox( const GameLib::Math::Vector3& origin, const GameLib::Math::Vector3& delta,
		const GameLib::Math::Vector3& boundsMin, const GameLib::Math::Vector3& boundsMax );
	// 箱・メッシュの空間の法線 localNormal を、ワールドの法線（長さ1）にして out に書く。
	// toLocal: ワールド → その空間の行列（置いた行列の逆）。拡大の入った空間でも面に垂直になるよう、toLocal の転置を掛ける。
	static void normalToWorld( const GameLib::Math::Matrix34& toLocal, const GameLib::Math::Vector3& localNormal, GameLib::Math::Vector3* out );
};

#endif
