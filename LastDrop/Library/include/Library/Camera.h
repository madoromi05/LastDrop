#ifndef INCLUDED_CAMERA_H
#define INCLUDED_CAMERA_H

#include "GameLib/Math/Vector3.h"
namespace GameLib{
	namespace Math{
		class Matrix44;
	}
}
using namespace GameLib::Math;

// 視点位置・注視点・画角などを保持し、透視投影のビュー行列を作成するカメラクラス
class Camera{
public:
	Camera();
	~Camera();
	void setPosition( const Vector3& );			// カメラの位置(視点)を設定する
	void setTarget( const Vector3& );			// カメラの注視点を設定する
	void setFieldOfViewY( float );				// 垂直方向の画角(ラジアン)を設定する
	void setNearClip( float );				// ニアクリップ距離を設定する
	void setFarClip( float );				// ファークリップ距離を設定する

	const Vector3* getPosition() const;			// カメラの位置を取得する
	const Vector3* getTarget() const;			// カメラの注視点を取得する
	float getFieldOfViewY() const;				// 垂直方向の画角を取得する
	float getNearClip() const;				// ニアクリップ距離を取得する
	float getFarClip() const;				// ファークリップ距離を取得する

	// 現在の設定値から透視投影のビュー行列を作成する
	void createPerspectiveViewMatrix( Matrix44* ) const;
private:
	Vector3 mPosition;
	Vector3 mTarget;
	float mFieldOfViewY;
	float mNearClip;
	float mFarClip;
};


#endif