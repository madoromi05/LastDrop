#include "GameSrc/Render/Shadow/ShadowMap.h"
#include "GameSrc/Render/Shadow/ShadowParams.h"
#include "GameSrc/Render/RenderPass.h"

#include "GameLib/Graphics/Manager.h"
#include "GameLib/Graphics/Enum.h"
#include "GameLib/Math/Matrix34.h"
#include "GameLib/Math/Functions.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const char* kStaticMeshShaderFile = "Resource/Shader/StaticMeshShadow.hlsl";
	// 光の外側の角度に足す余白（度。半角）。外側の縁のぎりぎりの所まで影のマップに入れる（PCF で隣の画素を読むため）。
	const float kSpotFieldMargin = 2.f;
	const float kHalf = 0.5f;
	const float kSquareAspect = 1.f; // 影のマップは正方形
	// 照らす向きがほぼ真上・真下のときは、世界の上を「上」にすると行列が作れないので、奥（+Z）を上にする。
	const float kNearlyVertical = 0.99f;
	const Vector3 kWorldUp( 0.f, 1.f, 0.f );
	const Vector3 kVerticalUp( 0.f, 0.f, 1.f );

	float spotFieldOfView( float outerAngleDegree ) {
		return ( outerAngleDegree + kSpotFieldMargin ) * 2.f;
	}
}

ShadowParams& shadowParams() {
	static ShadowParams sParams; // 起動中ずっと 1 つ（F6 で書き換える）
	return sParams;
}

bool& shadowsEnabled() {
	static bool sEnabled = true; // 起動中ずっと 1 つ（F6 で書き換える）
	return sEnabled;
}

ShadowMap::ShadowMap( int size ) :
	mSize( size ) {
	mDepth = Graphics::DepthTarget::create( size, size );
	mStaticMeshShader = Graphics::Shader::create( kStaticMeshShaderFile );
}

void ShadowMap::begin( const Matrix44& lightProjectionView ) {
	Graphics::Manager gm = Graphics::Manager::instance();
	gm.setRenderTargets( nullptr, 0, mDepth );
	gm.clear( mDepth );
	gm.setBlendMode( Graphics::BLEND_OPAQUE );
	gm.enableDepthTest( true );
	gm.enableDepthWrite( true );
	gm.setProjectionViewMatrix( lightProjectionView );
	RenderPass::beginShadow( mStaticMeshShader );
}

void ShadowMap::end() {
	RenderPass::endShadow();
	// 後から描く物（固定パイプライン）に残さない。
	Graphics::Manager::instance().setShader( Graphics::Shader() );
}

Graphics::DepthTarget ShadowMap::depth() const {
	return mDepth;
}

int ShadowMap::size() const {
	return mSize;
}

const char* ShadowMap::errorMessage() const {
	return mStaticMeshShader.errorMessage();
}

Matrix44 spotShadowProjectionView( const Vector3& position, const Vector3& direction, float outerAngleDegree, float nearClip, float farClip ) {
	Matrix44 projectionView;
	projectionView.setPerspectiveTransform( spotFieldOfView( outerAngleDegree ), kSquareAspect, kSquareAspect, nearClip, farClip );
	Vector3 target;
	target.setAdd( position, direction );
	const Vector3& up = ( abs( direction.y ) > kNearlyVertical ) ? kVerticalUp : kWorldUp;
	Matrix34 view;
	view.setViewTransform( position, target, up );
	projectionView *= view;
	return projectionView;
}

float spotShadowTexelSizeAtOneMeter( float outerAngleDegree, int mapSize ) {
	// 光から 1m の所の、影のマップに入る幅は 2 × tan( 画角の半分 )。それを画素の数で割る。
	return 2.f * tan( spotFieldOfView( outerAngleDegree ) * kHalf ) / static_cast< float >( mapSize );
}
