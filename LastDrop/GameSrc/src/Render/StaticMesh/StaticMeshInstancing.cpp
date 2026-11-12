#include "GameSrc/Render/StaticMesh/StaticMeshInstancing.h"
#include "GameSrc/Render/RenderPass.h"
#include "GameSrc/Render/ShaderConstants.h"

#include "GameLib/GameLib.h"
#include "GameLib/Graphics/Manager.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const char* kGBufferShaderFile = "Resource/Shader/StaticMeshInstancedGBuffer.hlsl";
	const char* kShadowShaderFile = "Resource/Shader/StaticMeshInstancedShadow.hlsl";
	const int kPlacementSlot = Graphics::Manager::FIRST_USER_CONSTANT_SLOT; // StaticMeshInstancing.hlsli の b1
	const int kFloatsPerInstance = ShaderConstants::kMatrix34Floats;	// 置き場所 1 つは 3x4 行列（float4 が 3 行）
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
	for ( int i = 0; i < number; ++i ) {
		ShaderConstants::copyMatrix34( placements[ i ], &mPlacementRows[ i * kFloatsPerInstance ] );
	}
	mPlacementBuffer.set( mPlacementRows.data(), number * kFloatsPerInstance * static_cast< int >( sizeof( float ) ) );	// 使う数のぶんだけ送る。残りの中身は不定になるが、SV_InstanceID は描く数より小さいので読まれない
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
