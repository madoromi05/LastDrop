#include "MeshMaterial.h"

#include "GameLib/Graphics/Manager.h"
#include "GameLib/Graphics/Enum.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// 既定の材質値。ゾンビ（暗めの肌テクスチャ）で見え方を調整した値。
	const float kDefaultDiffuse           = 1.0f;
	const float kDefaultSpecular          = 0.01f; // ほぼツヤなし
	const float kDefaultSpecularSharpness = 8.f;
	const float kDefaultNormalMapStrength = 0.3f;  // 標準(1)より弱めに調整した値
	const float kOpaque                   = 1.f;   // 透明度（不透明）
}

MeshMaterial::MeshMaterial() :
	mDiffuseColor( kDefaultDiffuse, kDefaultDiffuse, kDefaultDiffuse ),
	mSpecularColor( kDefaultSpecular, kDefaultSpecular, kDefaultSpecular ),
	mSpecularSharpness( kDefaultSpecularSharpness ),
	mNormalMapStrength( kDefaultNormalMapStrength ) {
}

void MeshMaterial::setTexture( Graphics::Texture texture ) {
	mTexture = texture;
}

void MeshMaterial::setNormalMap( Graphics::Texture texture ) {
	mNormalMap = texture;
}

void MeshMaterial::setDiffuseColor( const Vector3& color ) {
	mDiffuseColor = color;
}

void MeshMaterial::setSpecularColor( const Vector3& color ) {
	mSpecularColor = color;
}

void MeshMaterial::setSpecularSharpness( float sharpness ) {
	mSpecularSharpness = sharpness;
}

void MeshMaterial::setNormalMapStrength( float strength ) {
	mNormalMapStrength = strength;
}

void MeshMaterial::updateLoading() {
	// isReady() を呼ぶことで非同期ロードが進む（完了時にデコード＆GPU転送）。
	if ( mTexture ) {
		mTexture.isReady();
	}
	if ( mNormalMap ) {
		mNormalMap.isReady();
	}
}

void MeshMaterial::apply() const {
	Graphics::Manager m = Graphics::Manager::instance();
	m.setTexture( mTexture );
	if ( mNormalMap ) {
		m.setNormalMap( mNormalMap ); // setTexture が法線マップを一旦解除するので、必ずこの順で呼ぶ
	}
	m.setNormalMapStrength( mNormalMapStrength );
	// ライティング有り。テクスチャ未設定でも陰影で立体が見えるようにする。
	// テクスチャを設定した場合はディフューズ色に乗算される（白なら素の色）。
	m.setLightingMode( Graphics::LIGHTING_PER_PIXEL );
	m.setDiffuseColor( mDiffuseColor );
	m.setSpecularColor( mSpecularColor );
	m.setEmissionColor( Vector3( 0.f, 0.f, 0.f ) );
	m.setSpecularSharpness( mSpecularSharpness );
	m.setTransparency( kOpaque );
}

bool MeshMaterial::hasNormalMap() const {
	return mNormalMap ? true : false;
}
