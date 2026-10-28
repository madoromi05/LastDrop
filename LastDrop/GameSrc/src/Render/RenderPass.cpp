#include "GameSrc/Render/RenderPass.h"

#include "GameLib/GameLib.h"
#include "GameLib/Graphics/Shader.h"

namespace{
	RenderPassKind gCurrent = RenderPassKind::Forward;
	const GameLib::Graphics::Shader* gStaticMeshShader = nullptr; // GBuffer の間だけ指す（持ち主は GBuffer）
}

namespace RenderPass{

RenderPassKind current() {
	return gCurrent;
}

void beginGBuffer( const GameLib::Graphics::Shader& staticMeshShader ) {
	ASSERT( gCurrent == RenderPassKind::Forward && "RenderPass : beginGBuffer is called twice." );
	gCurrent = RenderPassKind::GBuffer;
	gStaticMeshShader = &staticMeshShader;
}

void endGBuffer() {
	ASSERT( gCurrent == RenderPassKind::GBuffer && "RenderPass : endGBuffer without beginGBuffer." );
	gCurrent = RenderPassKind::Forward;
	gStaticMeshShader = nullptr;
}

const GameLib::Graphics::Shader& staticMeshShader() {
	ASSERT( gStaticMeshShader && "RenderPass : staticMeshShader is only for the G-Buffer pass." );
	return *gStaticMeshShader;
}

} //namespace RenderPass
