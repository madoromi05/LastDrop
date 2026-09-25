#ifndef INCLUDED_THIRD_PERSON_CAMERA_H
#define INCLUDED_THIRD_PERSON_CAMERA_H

#include "GameLib/Math/Vector3.h"

// 追従対象の背後から見る三人称(TPS)カメラ。
// 向き（ヨー・ピッチ）を持ち、追従対象の位置から視点・注視点を決めて描画ステートへ反映するだけを担う。
// 入力を直接読まないので、マウスでもスティックでも rotate() に量を渡せば回せる。
//
// 角度の約束: ヨーはキャラの setAngleY と同じ（0 で +Z を見る。正で左回り＝上から見て反時計回り）。
// ピッチは正でカメラが上がり、見下ろす。
//
// 使い方: 毎フレーム rotate → follow → apply の順に呼ぶ。移動の基準には yaw() を使う。
class ThirdPersonCamera{
public:
	ThirdPersonCamera();

	// 向きを変える（度）。ピッチは地面の下や真上へ回り込まないよう上下限で止める。
	void rotate( float yawDelta, float pitchDelta );
	// 追従対象の足元の位置から、視点と注視点を決める。
	void follow( const GameLib::Math::Vector3& targetFootPosition );
	// 射影・ビュー行列と視点位置を Graphics::Manager へ設定する。
	void apply() const;

	float yaw() const;										// 水平の向き（度）。キャラの移動方向の基準
	const GameLib::Math::Vector3& eyePosition() const;		// 視点（ライティングの計算にも使う）

private:
	float mYaw;
	float mPitch;
	GameLib::Math::Vector3 mEyePosition;
	GameLib::Math::Vector3 mLookAt;
};

#endif
