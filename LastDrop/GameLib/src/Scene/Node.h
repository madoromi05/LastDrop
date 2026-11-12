#ifndef INCLUDED_GAMELIB_SCENE_NODE_H
#define INCLUDED_GAMELIB_SCENE_NODE_H

#include "GameLib/Math/Matrix34.h"
#include "GameLib/Math/Matrix44.h"
#include "GameLib/Math/Functions.h"
#include "Scene/NodeTemplate.h"
#include "Scene/AnimationNode.h"

namespace GameLib{
namespace Scene{
using namespace GameLib::Math;

class Node{
public:
	Node( const NodeTemplate* tmpl ) : 
	mTranslation( 0.f, 0.f, 0.f ),
	mRotation( 0.f, 0.f, 0.f ),
	mScale( 1.f, 1.f, 1.f ),
	mChildren( 0 ),
	mChildNumber( 0 ),
	mTemplate( tmpl ),
	mAnimation( 0 ){
	}
	~Node(){
		mTemplate = 0; //単なる参照
		mAnimation = 0;
	}
	void setChildren( Node** children, int childNumber ){
		mChildren = children;
		mChildNumber = childNumber;
	}
	//現在のポーズから自分と子孫のワールド行列を計算して保持する（描画はしない）
	//スキニング等でボーン行列を外から読むため、行列計算を draw() から分離した
	void updateWorldMatrix( const Matrix34& parentMatrix ){
		mWorldMatrix.setMul( parentMatrix, mTemplate->mTransform ); //デフォルト変換をかけて、
		mWorldMatrix.translate( mTranslation );
		mWorldMatrix.rotateY( mRotation.y );
		mWorldMatrix.rotateX( mRotation.x );
		mWorldMatrix.rotateZ( mRotation.z );
		mWorldMatrix.scale( mScale );
		for ( int i = 0; i < mChildNumber; ++i ){
			mChildren[ i ]->updateWorldMatrix( mWorldMatrix );
		}
	}
	//updateWorldMatrix() で計算済みのワールド行列を使って描画する
	void draw( const Vector3& color, float transparency ) const {
		if ( mTemplate->mBatch ){
			Graphics::Manager().setWorldMatrix( mWorldMatrix );
			mTemplate->mBatch->draw( color, transparency );
		}
		for ( int i = 0; i < mChildNumber; ++i ){
			mChildren[ i ]->draw( color, transparency );
		}
	}
	//直近の updateWorldMatrix() で計算したワールド行列を返す
	const Matrix34& worldMatrix() const {
		return mWorldMatrix;
	}
	void setAnimation( const AnimationNode* a ){
		mAnimation = a;
	}
	void removeAnimation(){
		mAnimation = 0;
		mTranslation.set( 0.f, 0.f, 0.f );
		mRotation.set( 0.f, 0.f, 0.f );
		mScale.set( 1.f, 1.f, 1.f );
	}
	void updateAnimation( float t ){
		if ( mAnimation ){
			mAnimation->update(
				&mTranslation,
				&mRotation,
				&mScale,
				t );
		}
	}
	const char* name() const {
		return mTemplate->mName;
	}
private:
	Vector3 mTranslation;
	Vector3 mRotation;
	Vector3 mScale;
	Matrix34 mWorldMatrix; //updateWorldMatrix() で計算し、draw()/worldMatrix() が参照する
	Node** mChildren;
	int mChildNumber;
	const NodeTemplate* mTemplate;
	const AnimationNode* mAnimation;
};

} //namespace Scene
} //namespace GameLib

#endif

