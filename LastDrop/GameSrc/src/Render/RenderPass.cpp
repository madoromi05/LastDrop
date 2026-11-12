#include "GameSrc/Render/RenderPass.h"

#include "GameLib/GameLib.h"
#include "GameLib/Graphics/Shader.h"
#include "GameLib/Graphics/ConstantBuffer.h"
#include "GameLib/Graphics/Manager.h"
#include "GameLib/Profiling/Profiler.h"

namespace{
	const int kOrmSlot = GameLib::Graphics::Manager::FIRST_USER_TEXTURE_SLOT + 9;	// GBuffer.hlsli の gOrmMap の番号（t11）。t2〜t5 は G-Buffer を読む側、t10 は SSAO の結果が使うので重ならない所

	RenderPassKind gCurrent = RenderPassKind::Forward;
	const GameLib::Graphics::Shader* gStaticMeshShader = nullptr;     // GBuffer の間だけ指す（持ち主は GBuffer）
	GameLib::Graphics::ConstantBuffer* gSurfaceConstants = nullptr;  // 同上
	const GameLib::Graphics::Texture* gDefaultOrm = nullptr;         // 同上
	// 直前に送った材質の値と画像（同じ材質が続くとき送り直さないため）
	// G-Buffer のパスの始めに忘れる（定数の中身は GBuffer が作り直すことがあり、画像の場所は終わりに外すため）
	bool gHasSentSurface = false;
	SurfaceParams gSentSurface;
	GameLib::Graphics::Texture gSentOrm;

	bool sameSurface( const SurfaceParams& a, const SurfaceParams& b ) {
		return a.roughness == b.roughness && a.metallic == b.metallic && a.emission == b.emission && a.puddle == b.puddle;
	}
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
	gHasSentSurface = false;
}

void endGBuffer() {
	ASSERT( gCurrent == RenderPassKind::GBuffer && "RenderPass : endGBuffer without beginGBuffer." );
	gCurrent = RenderPassKind::Forward;
	gStaticMeshShader = nullptr;
	gSurfaceConstants = nullptr;
	gDefaultOrm = nullptr;
	gHasSentSurface = false;
	gSentOrm = GameLib::Graphics::Texture(); // 画像を持ち続けない
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
	GameLib::Graphics::Texture shownOrm = orm ? orm : *gDefaultOrm;	// 材質の画像が無い材質は白 1 画素（シェーダは「数値 × 画像」なので、数値がそのまま使われる）
	// GBuffer.hlsli の cbuffer GBufferMaterial（float4 1 つ。粗さ・金属・発光・水たまり）
	if ( !gHasSentSurface || !sameSurface( params, gSentSurface ) ) {
		gSurfaceConstants->set( &params, sizeof( params ) );
		gSentSurface = params;
	}
	if ( !gHasSentSurface || shownOrm != gSentOrm ) {
		GameLib::Graphics::Manager::instance().setShaderResource( kOrmSlot, shownOrm );
		gSentOrm = shownOrm;
	}
	gHasSentSurface = true;
}

void forgetSentSurface() {
	ASSERT( gCurrent == RenderPassKind::GBuffer && "RenderPass : forgetSentSurface is only for the G-Buffer pass." );
	gHasSentSurface = false;
}

void countDraw( int triangleNumber ) {
	if ( gCurrent == RenderPassKind::GBuffer ) {
		PROFILE_COUNT( "Draw.gbuffer", 1 );
		PROFILE_COUNT( "Draw.triangles", static_cast< double >( triangleNumber ) );
	} else if ( gCurrent == RenderPassKind::Shadow ) {
		PROFILE_COUNT( "Draw.shadow", 1 );
	}
}

} //namespace RenderPass
