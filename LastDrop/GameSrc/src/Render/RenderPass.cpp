#include "GameSrc/Render/RenderPass.h"

#include "GameLib/GameLib.h"
#include "GameLib/Graphics/Shader.h"
#include "GameLib/Graphics/ConstantBuffer.h"

namespace{
	RenderPassKind gCurrent = RenderPassKind::Forward;
	const GameLib::Graphics::Shader* gStaticMeshShader = nullptr;     // GBuffer の間だけ指す（持ち主は GBuffer）
	GameLib::Graphics::ConstantBuffer* gSurfaceConstants = nullptr;  // 同上
}

namespace RenderPass{

RenderPassKind current() {
	return gCurrent;
}

void beginGBuffer( const GameLib::Graphics::Shader& staticMeshShader, GameLib::Graphics::ConstantBuffer* surfaceConstants ) {
	ASSERT( gCurrent == RenderPassKind::Forward && "RenderPass : beginGBuffer is called twice." );
	ASSERT( surfaceConstants && "RenderPass : surfaceConstants is required." );
	gCurrent = RenderPassKind::GBuffer;
	gStaticMeshShader = &staticMeshShader;
	gSurfaceConstants = surfaceConstants;
}

void endGBuffer() {
	ASSERT( gCurrent == RenderPassKind::GBuffer && "RenderPass : endGBuffer without beginGBuffer." );
	gCurrent = RenderPassKind::Forward;
	gStaticMeshShader = nullptr;
	gSurfaceConstants = nullptr;
}

void beginShadow( const GameLib::Graphics::Shader& staticMeshShader ) {
	ASSERT( gCurrent == RenderPassKind::Forward && "RenderPass : beginShadow while another pass is active." );
	gCurrent = RenderPassKind::Shadow;
	gStaticMeshShader = &staticMeshShader;
}

void endShadow() {
	ASSERT( gCurrent == RenderPassKind::Shadow && "RenderPass : endShadow without beginShadow." );
	gCurrent = RenderPassKind::Forward;
	gStaticMeshShader = nullptr;
}

const GameLib::Graphics::Shader& staticMeshShader() {
	ASSERT( gStaticMeshShader && "RenderPass : staticMeshShader is only for the G-Buffer and shadow passes." );
	return *gStaticMeshShader;
}

void setSurface( const SurfaceParams& params ) {
	ASSERT( gSurfaceConstants && "RenderPass : setSurface is only for the G-Buffer pass." );
	// GBuffer.hlsli の cbuffer GBufferMaterial（float4 1 つ）の xyz だけ書く（w は使わない）。
	gSurfaceConstants->set( &params, sizeof( params ) );
}

} //namespace RenderPass
