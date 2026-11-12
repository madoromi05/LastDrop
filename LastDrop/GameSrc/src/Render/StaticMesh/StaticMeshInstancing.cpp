#include "GameSrc/Render/StaticMesh/StaticMeshInstancing.h"
#include "GameSrc/Render/RenderPass.h"

#include "GameLib/GameLib.h"
#include "GameLib/Graphics/Manager.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const char* kGBufferShaderFile = "Resource/Shader/StaticMeshInstancedGBuffer.hlsl";
	const char* kShadowShaderFile = "Resource/Shader/StaticMeshInstancedShadow.hlsl";
	const int kPlacementSlot = Graphics::Manager::FIRST_USER_CONSTANT_SLOT; // StaticMeshInstancing.hlsli の b1
	const int kRowsPerInstance = 3;   // 3x4 行列 = float4 が3行
	const int kFloatsPerRow = 4;
	const int kFloatsPerInstance = kRowsPerInstance * kFloatsPerRow;
}

StaticMeshInstancing::StaticMeshInstancing() :
	mPlacementRows( kMaxInstances * kFloatsPerInstance, 0.f ) {
	mGBufferShader = Graphics::Shader::create( kGBufferShaderFile );
	mShadowShader = Graphics::Shader::create( kShadowShaderFile );
	mPlacementBuffer = Graphics::ConstantBuffer::create( static_cast< int >( mPlacementRows.size() * sizeof( float ) ) );
}

bool StaticMeshInstancing::isReady() const {
	RenderPassKind pass = RenderPass::current();
	if ( pass != RenderPassKind::GBuffer && pass != RenderPassKind::Shadow ) {
		return false;
	}
	return !currentShader().isError();
}

const char* StaticMeshInstancing::errorMessage() const {
	const char* message = mGBufferShader.errorMessage();
	if ( message[ 0 ] != '\0' ) {
		return message;
	}
	return mShadowShader.errorMessage();
}

const Graphics::Shader& StaticMeshInstancing::currentShader() const {
	return ( RenderPass::current() == RenderPassKind::Shadow ) ? mShadowShader : mGBufferShader;
}

void StaticMeshInstancing::begin() {
	ASSERT( isReady() && "StaticMeshInstancing : begin is called while not ready." );
	Graphics::Manager gm = Graphics::Manager::instance();
	gm.setShader( currentShader() );
	gm.setConstantBuffer( kPlacementSlot, mPlacementBuffer );
	Matrix34 identity;
	identity.setIdentity();
	gm.setWorldMatrix( identity ); // gTransform を射影×ビューにする（置き場所はシェーダが b1 から掛ける）
}

void StaticMeshInstancing::setPlacements( const Matrix34* placements, int number ) {
	ASSERT( placements && number > 0 && number <= kMaxInstances && "StaticMeshInstancing : bad placements." );
	// Matrix34 は行ごとに m00..m03 / m10..m13 / m20..m23 と並ぶので、そのまま3行ぶん写す
	for ( int i = 0; i < number; ++i ) {
		const Matrix34& m = placements[ i ];
		float* row = &mPlacementRows[ i * kFloatsPerInstance ];
		row[ 0 ] = m.m00; row[ 1 ] = m.m01; row[ 2 ]  = m.m02; row[ 3 ]  = m.m03;
		row[ 4 ] = m.m10; row[ 5 ] = m.m11; row[ 6 ]  = m.m12; row[ 7 ]  = m.m13;
		row[ 8 ] = m.m20; row[ 9 ] = m.m21; row[ 10 ] = m.m22; row[ 11 ] = m.m23;
	}
	// 使う数のぶんだけ送る。残りの中身は不定になるが、SV_InstanceID は描く数より小さいので読まれない
	mPlacementBuffer.set( mPlacementRows.data(), number * kFloatsPerInstance * static_cast< int >( sizeof( float ) ) );
}

void StaticMeshInstancing::end() {
	Graphics::Manager gm = Graphics::Manager::instance();
	gm.setShader( Graphics::Shader() );
	gm.setConstantBuffer( kPlacementSlot, Graphics::ConstantBuffer() );
}

bool& instancingEnabled() {
	static bool sEnabled = true; // 起動中ずっと 1 つ（F6 で書き換える）
	return sEnabled;
}
