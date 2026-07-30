#ifndef INCLUDED_MODEL_H
#define INCLUDED_MODEL_H

#include "GameLib/GameLib.h"
#include "GameLib/Math/Vector3.h"
namespace GameLib{
	namespace Math{
		class Matrix44;
	}
}
using GameLib::ConstSharedPtr;
using namespace GameLib::Math;
class Batch;

// Batch(頂点・インデックス・テクスチャ・マテリアル)に位置・角度・拡縮を
// 持たせて配置し、ワールド上に描画するためのクラス
class Model{
public:
	Model( ConstSharedPtr< Batch > );		// 描画に使うBatchを指定して構築する
	~Model();
	void draw() const;				// 位置・角度・拡縮を反映してBatchを描画する
	void setPosition( const Vector3& );		// モデルの位置を設定する
	void setAngle( const Vector3& );		// モデルの角度(オイラー角)を設定する
	void setScale( const Vector3& );		// モデルの拡縮率を設定する
	ConstSharedPtr< Batch > getBatch() const;	// このModelが参照しているBatchを取得する
	const Vector3* getPosition() const;		// モデルの位置を取得する
	const Vector3* getAngle() const;		// モデルの角度を取得する
private:
	Vector3 mPosition;
	Vector3 mAngle;
	Vector3 mScale;
	ConstSharedPtr< Batch > mBatch;
};

#endif
