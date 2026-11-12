#include "GameSrc/Render/MeshMaterial.h"
#include "GameSrc/Render/RenderPass.h"

#include "GameLib/Graphics/Manager.h"
#include "GameLib/Graphics/Enum.h"
#include "GameLib/Profiling/Profiler.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// 既定の材質値。ゾンビ（暗めの肌テクスチャ）で見え方を調整した値
	const float kDefaultDiffuse           = 1.0f;
	const float kDefaultSpecular          = 0.01f; // ほぼツヤなし
	const float kDefaultSpecularSharpness = 8.f;
	const float kDefaultNormalMapStrength = 0.3f;  // 標準(1)より弱めに調整した値
	const float kOpaque                   = 1.f;   // 透明度（不透明）
	// PBR の既定値。書いていない材質は、つやの無い金属でない物（ゾンビの肌・服など）
	const float kDefaultRoughness         = 0.8f;
	const float kDefaultMetallic          = 0.f;
	const float kDefaultEmission          = 0.f;
	const float kNoPuddle                 = 0.f; // 水たまりは道路だけ（材質の txt に puddle="1"）
	const float kPuddle                   = 1.f;
}

MeshMaterial::MeshMaterial() :
	mDiffuseColor( kDefaultDiffuse, kDefaultDiffuse, kDefaultDiffuse ),
	mSpecularColor( kDefaultSpecular, kDefaultSpecular, kDefaultSpecular ),
	mSpecularSharpness( kDefaultSpecularSharpness ),
	mNormalMapStrength( kDefaultNormalMapStrength ),
	mRoughness( kDefaultRoughness ),
	mMetallic( kDefaultMetallic ),
	mEmission( kDefaultEmission ),
	mPuddle( kNoPuddle ),
	mDoubleSided( false ) {
}

void MeshMaterial::setTexture( Graphics::Texture texture ) {
	mTexture = texture;
}

void MeshMaterial::setNormalMap( Graphics::Texture texture ) {
	mNormalMap = texture;
}

void MeshMaterial::setOrmMap( Graphics::Texture texture ) {
	mOrmMap = texture;
}

void MeshMaterial::setPuddle( bool puddle ) {
	mPuddle = puddle ? kPuddle : kNoPuddle;
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

void MeshMaterial::setRoughness( float roughness ) {
	mRoughness = roughness;
}

void MeshMaterial::setMetallic( float metallic ) {
	mMetallic = metallic;
}

void MeshMaterial::setEmission( float emission ) {
	mEmission = emission;
}

void MeshMaterial::setDoubleSided( bool doubleSided ) {
	mDoubleSided = doubleSided;
}

bool MeshMaterial::updateLoading() {
	// isReady() を呼ぶことで非同期ロードが進む（完了時にデコード＆GPU転送）
	// 片方がまだでも、もう片方の isReady も必ず呼ぶ（展開を 1 枚ずつ待たせないため）
	bool ready = true;
	if ( mTexture && !mTexture.isReady() ) {
		ready = false;
	}
	if ( mNormalMap && !mNormalMap.isReady() ) {
		ready = false;
	}
	if ( mOrmMap && !mOrmMap.isReady() ) {
		ready = false;
	}
	return ready;
}

void MeshMaterial::apply() const {
	PROFILE_COUNT( "Material.apply", 1 );
	Graphics::Manager m = Graphics::Manager::instance();
	// 色の画像と法線マップを一度に渡す。同じ材質が続くとき、GameLib が付け直しを飛ばせる
	m.setTextures( mTexture, mNormalMap );
	m.setNormalMapStrength( mNormalMapStrength );
	// ライティング有り。テクスチャ未設定でも陰影で立体が見えるようにする
	// テクスチャを設定した場合はディフューズ色に乗算される（白なら素の色）
	m.setLightingMode( Graphics::LIGHTING_PER_PIXEL );
	m.setDiffuseColor( mDiffuseColor );
	m.setSpecularColor( mSpecularColor );
	m.setEmissionColor( Vector3( 0.f, 0.f, 0.f ) );
	m.setSpecularSharpness( mSpecularSharpness );
	m.setTransparency( kOpaque );
	// 全部のメッシュの三角形の向きを調べると、99.9% 以上が法線と同じ側を向いていた（表が反時計回り）ので、既定で裏を捨てる
	m.setCullMode( mDoubleSided ? Graphics::CULL_NONE : Graphics::CULL_BACK );
	// G-Buffer へ描く間は、PBR の材質値を G-Buffer のシェーダへ渡す（静的メッシュもキャラもここを通る）
	if ( RenderPass::current() == RenderPassKind::GBuffer ) {
		RenderPass::setSurface( SurfaceParams{ mRoughness, mMetallic, mEmission, mPuddle }, mOrmMap );
	}
}

bool MeshMaterial::hasNormalMap() const {
	return mNormalMap ? true : false;
}
