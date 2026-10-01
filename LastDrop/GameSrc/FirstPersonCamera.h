#ifndef INCLUDED_FIRST_PERSON_CAMERA_H
#define INCLUDED_FIRST_PERSON_CAMERA_H

#include "GameLib/Math/Vector3.h"

// キャラの目の位置から見る一人称(FPS)カメラ。
// 向き（ヨー・ピッチ）を持ち、渡された目の位置から視線を決めて描画ステートへ反映するだけを担う。
// 入力もキャラも直接知らないので、マウスでもスティックでも rotate() に量を渡せば回せ、目の位置は誰が決めてもよい。
//
// 角度の約束: ヨーはキャラの setAngleY と同じ（0 で +Z を見る。正で左回り＝上から見て反時計回り）。
// ピッチは正で見下ろす。
//
// 使い方: 毎フレーム rotate → （キャラを動かす）→ setEyePosition → apply の順に呼ぶ。
// 目の位置はキャラを動かした後のものを渡す（前のフレームの頭の位置だと、視点が体から1フレーム遅れる）。
class FirstPersonCamera{
public:
	FirstPersonCamera();

	// 向きを変える（度）。ピッチは真上・真下へ回り込まないよう上下限で止める。
	void rotate( float yawDelta, float pitchDelta );
	// 視点（ワールド座標）。キャラの頭の位置から求めたものを渡す。
	void setEyePosition( const GameLib::Math::Vector3& );
	// 縦の視野角（度）。覗くときに狭めてズームする。
	void setFieldOfView( float degree );
	// 射影・ビュー行列と視点位置を Graphics::Manager へ設定する。
	void apply() const;

	float yaw() const;		// 水平の向き（度）。キャラの向きと移動方向の基準
	float pitch() const;	// 上下の向き（度）。キャラの背骨の曲げに使う
	// 視線の向き（長さ1）。音の聞き手の向きにも使う。
	GameLib::Math::Vector3 viewDirection() const;
	const GameLib::Math::Vector3& eyePosition() const;	// 視点（setEyePosition で渡したもの）

private:
	float mYaw;
	float mPitch;
	float mFieldOfView;
	GameLib::Math::Vector3 mEyePosition;
};

#endif
