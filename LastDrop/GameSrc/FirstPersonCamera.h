#ifndef INCLUDED_FIRST_PERSON_CAMERA_H
#define INCLUDED_FIRST_PERSON_CAMERA_H

#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix44.h"

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
	// rotate で決めた上下の向きへ足す角度（度。負で見上げる側）。撃った反動のように、一時的に視点をずらして後で戻すものに使う。
	// rotate の向きそのものは変えないので、0 を渡せば元の向きへ戻る。足した結果も上下限で止める。
	void setPitchOffset( float degree );
	// 視点（ワールド座標）。キャラの頭の位置から求めたものを渡す。
	void setEyePosition( const GameLib::Math::Vector3& );
	// 縦の視野角（度）。覗くときに狭めてズームする。
	void setFieldOfView( float degree );
	// 射影・ビュー行列と視点位置を Graphics::Manager へ設定する。
	void apply() const;
	// 射影×ビュー行列（apply で設定するものと同じ）。メッシュ以外のもの（デバッグ表示など）を同じ視点で描くのに使う。
	GameLib::Math::Matrix44 projectionViewMatrix() const;

	float yaw() const;		// 水平の向き（度）。キャラの向きと移動方向の基準
	float pitch() const;	// 上下の向き（度。setPitchOffset の分を足したもの）。キャラの背骨の曲げに使う
	// 視線の向き（長さ1）。音の聞き手の向きにも使う。
	GameLib::Math::Vector3 viewDirection() const;
	const GameLib::Math::Vector3& eyePosition() const;	// 視点（setEyePosition で渡したもの）

private:
	float mYaw;
	float mPitch;       // rotate で決めた上下の向き（mPitchOffset を足す前）
	float mPitchOffset; // 一時的に足す角度（反動など）
	float mFieldOfView;
	GameLib::Math::Vector3 mEyePosition;
};

#endif
