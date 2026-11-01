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
	// 上下の向きは、自キャラの背骨の曲げで体が付いてこられる範囲で止める。
	FirstPersonCamera();
	// pitchLimit: 上下の向きを止める角度（度。正の値で、上下とも同じだけ）。
	// 体の付いていないカメラ（確認用の FreeCamera）が、真下近くまで見下ろせるようにするために使う。
	explicit FirstPersonCamera( float pitchLimit );

	// 向きを変える（度）。ピッチは真上・真下へ回り込まないよう上下限で止める。
	void rotate( float yawDelta, float pitchDelta );
	// rotate で決めた向きへ足す角度（度。ヨーは正で左、ピッチは負で見上げる側）。撃った反動・攻撃された怯みのように、
	// 一時的に狙いをずらして後で戻すものに使う。yaw・pitch・viewDirection（弾の向き）に入る。
	// rotate の向きそのものは変えないので、0 を渡せば元の向きへ戻る。足した結果のピッチも上下限で止める。
	void setAimOffset( float yawDegree, float pitchDegree );
	// 描く行列だけへ足す揺れ（度。カメラシェイク）。yaw・pitch・viewDirection には入らない（弾・体・音の向きは揺らさない）。
	// ロールは視線の周りの傾き（向きは正負どちらでもよい。揺れは両方へ振れる）。
	void setShake( float yawDegree, float pitchDegree, float rollDegree );
	// 銃と弾の向きだけへ足すぶれ（度。照準のぶれ AimSway。ヨーは正で左、ピッチは正で見下ろす）。
	// 描く行列・yaw・pitch・viewDirection には入らない（画面の中央は動かさない）。weaponDirection にだけ入る。
	void setWeaponSway( float yawDegree, float pitchDegree );
	// 視点（ワールド座標）。キャラの頭の位置から求めたものを渡す。
	void setEyePosition( const GameLib::Math::Vector3& );
	// 縦の視野角（度）。覗くときに狭めてズームする。
	void setFieldOfView( float degree );
	// 射影・ビュー行列と視点位置を Graphics::Manager へ設定する。
	void apply() const;
	// 射影×ビュー行列（apply で設定するものと同じ）。メッシュ以外のもの（デバッグ表示など）を同じ視点で描くのに使う。
	GameLib::Math::Matrix44 projectionViewMatrix() const;

	float yaw() const;		// 水平の向き（度。setAimOffset の分を足したもの）。キャラの向きと移動方向の基準
	float pitch() const;	// 上下の向き（度。setAimOffset の分を足したもの）。キャラの背骨の曲げに使う
	// 狙いの向き（長さ1。setAimOffset の分は入り、setShake の分は入らない）。弾の向き・音の聞き手の向きにも使う。
	GameLib::Math::Vector3 viewDirection() const;
	// 弾を出す向き（長さ1）。viewDirection に setWeaponSway の分を足したもの（銃の見た目も同じだけぶれている）。
	GameLib::Math::Vector3 weaponDirection() const;
	float weaponSwayYaw() const;   // setWeaponSway で渡した左右（度）
	float weaponSwayPitch() const; // setWeaponSway で渡した上下（度）
	float fieldOfView() const;     // 縦の視野角（度。setFieldOfView で渡したもの）
	const GameLib::Math::Vector3& eyePosition() const;	// 視点（setEyePosition で渡したもの）

private:
	float mPitchLimit;  // 上下の向きを止める角度（度。-mPitchLimit〜+mPitchLimit）
	float mYaw;
	float mPitch;       // rotate で決めた上下の向き（mPitchOffset を足す前）
	float mYawOffset;   // 一時的に狙いへ足す角度（反動・怯み）
	float mPitchOffset;
	float mShakeYaw;    // 描く行列だけへ足す揺れ（カメラシェイク）
	float mShakePitch;
	float mShakeRoll;
	float mSwayYaw;     // 銃と弾の向きだけへ足すぶれ（照準のぶれ）
	float mSwayPitch;
	float mFieldOfView;
	GameLib::Math::Vector3 mEyePosition;
};

#endif
