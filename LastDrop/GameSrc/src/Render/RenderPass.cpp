#include "GameSrc/Render/RenderPass.h"

#include "GameLib/GameLib.h"
#include "GameLib/Graphics/Shader.h"
#include "GameLib/Graphics/ConstantBuffer.h"
#include "GameLib/Graphics/Manager.h"

namespace{
	// GBuffer.hlsli の gOrmMap の番号（t11）。t2〜t5 は G-Buffer を読む側、t10 は SSAO の結果が使うので重ならない所。
	const int kOrmSlot = GameLib::Graphics::Manager::FIRST_USER_TEXTURE_SLOT + 9;

	RenderPassKind gCurrent = RenderPassKind::Forward;
	const GameLib::Graphics::Shader* gStaticMeshShader = nullptr;     // GBuffer の間だけ指す（持ち主は GBuffer）
	GameLib::Graphics::ConstantBuffer* gSurfaceConstants = nullptr;  // 同上
	const GameLib::Graphics::Texture* gDefaultOrm = nullptr;         // 同上
}

namespace RenderPass{

RenderPassKind current() {
	return gCurrent;
}

void beginGBuffer( const GameLib::Graphics::Shader& staticMeshShader, GameLib::Graphics::ConstantBuffer* surfaceConstants,
	const GameLib::Graphics::Texture* defaultOrm ) {
	ASSERT( gCurrent == RenderPassKind::Forward && "RenderPass : beginGBuffer is called twice." );
	ASSERT( surfaceConstants && "RenderPass : surfaceConstants is required." );
	ASSERT( defaultOrm && "RenderPass : defaultOrm is required." );
	gCurrent = RenderPassKind::GBuffer;
	gStaticMeshShader = &staticMeshShader;
	gSurfaceConstants = surfaceConstants;
	gDefaultOrm = defaultOrm;
}

void endGBuffer() {
	ASSERT( gCurrent == RenderPassKind::GBuffer && "RenderPass : endGBuffer without beginGBuffer." );
	gCurrent = RenderPassKind::Forward;
	gStaticMeshShader = nullptr;
	gSurfaceConstants = nullptr;
	gDefaultOrm = nullptr;
	GameLib::Graphics::Manager::instance().setShaderResource( kOrmSlot, GameLib::Graphics::Texture() ); // 後から描く物に残さない
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

void setSurface( const SurfaceParams& params, GameLib::Graphics::Texture orm ) {
	ASSERT( gSurfaceConstants && "RenderPass : setSurface is only for the G-Buffer pass." );
	// GBuffer.hlsli の cbuffer GBufferMaterial（float4 1 つ。粗さ・金属・発光・水たまり）。
	gSurfaceConstants->set( &params, sizeof( params ) );
	// 材質の画像が無い材質は白 1 画素（シェーダは「数値 × 画像」なので、数値がそのまま使われる）。
	GameLib::Graphics::Manager::instance().setShaderResource( kOrmSlot, orm ? orm : *gDefaultOrm );
}

} //namespace RenderPass
