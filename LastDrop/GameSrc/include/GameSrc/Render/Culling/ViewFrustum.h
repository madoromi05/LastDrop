#ifndef INCLUDED_VIEW_FRUSTUM_H
#define INCLUDED_VIEW_FRUSTUM_H

#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix44.h"

// カメラに映る範囲（視錐台カリング）。「この箱は画面に映りうるか」を答えるだけ。状態はカメラの行列から作った6枚の面だけ
// 映らない物を描かずに済ませるために使う（町は見えていない建物・地面の方が多く、全部描くと描画の回数が無駄に増える）
// 判定は甘い側に倒してある: 映らない箱を「映りうる」と答えることはあるが、映る箱を「映らない」と答えることは無い
// 描くか決めるだけに使い、当たり判定やゲームの進行には使わないこと（見ているかどうかで結果が変わってはいけないため）
class ViewFrustum{
public:
	// projectionView: カメラの射影×ビュー行列（FirstPersonCamera::projectionViewMatrix）
	explicit ViewFrustum( const GameLib::Math::Matrix44& projectionView );

	// boundsMin〜boundsMax の軸に沿った箱（ワールド座標）が、少しでも映りうるか
	bool mayContain( const GameLib::Math::Vector3& boundsMin, const GameLib::Math::Vector3& boundsMax ) const;

private:
	// 面1枚。normal・p + distance が 0 以上の側が、映る側
	struct Plane{
		GameLib::Math::Vector3 normal; // 長さは 1 とは限らない（どちら側かしか見ないので、揃えなくてよい）
		float distance;
	};
	static const int kPlaneNumber = 6; // 左・右・下・上・手前・奥
	Plane mPlanes[ kPlaneNumber ];
};

#endif
