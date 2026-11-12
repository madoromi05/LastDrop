#include "GameSrc/Render/SkinnedMesh/SkinningPipeline.h"
#include "GameSrc/Render/RenderPass.h"
#include "GameSrc/Render/ShaderConstants.h"

#include "GameLib/GameLib.h"
#include "GameLib/Graphics/Manager.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const char* kShaderFile = "Resource/Shader/SkinnedMesh.hlsl";
	const char* kGBufferShaderFile = "Resource/Shader/SkinnedMeshGBuffer.hlsl";
	const char* kShadowShaderFile = "Resource/Shader/SkinnedMeshShadow.hlsl";
	const int kBonePaletteSlot = Graphics::Manager::FIRST_USER_CONSTANT_SLOT; // Skinning.hlsli の b1
	const int kFloatsPerBone = ShaderConstants::kMatrix34Floats;	// 骨 1 本は 3x4 行列（float4 が 3 行）

	// 頂点の並び。SkinnedMeshData の GPU 頂点（SkinnedGpuVertex）と同じ順にする
	const Graphics::VertexElement kElements[] = {
		Graphics::VERTEX_POSITION4,
		Graphics::VERTEX_NORMAL3,
		Graphics::VERTEX_COLOR,
		Graphics::VERTEX_TEXCOORD2,
		Graphics::VERTEX_BONE_INDICES4,
		Graphics::VERTEX_BONE_WEIGHTS4,
	};
	const int kElementNumber = sizeof( kElements ) / sizeof( kElements[ 0 ] );
}

SkinningPipeline::SkinningPipeline() :
	mBoneRows( kMaxBones * kFloatsPerBone, 0.f ) {
	mShader = Graphics::Shader::create( kShaderFile, vertexFormat() );
	mGBufferShader = Graphics::Shader::create( kGBufferShaderFile, vertexFormat() );
	mShadowShader = Graphics::Shader::create( kShadowShaderFile, vertexFormat() );
	mBonePalette = Graphics::ConstantBuffer::create( static_cast< int >( mBoneRows.size() * sizeof( float ) ) );
}

const Graphics::VertexFormat& SkinningPipeline::vertexFormat() {
	static const Graphics::VertexFormat format = Graphics::VertexFormat::create( kElements, kElementNumber );
	return format;
}

bool SkinningPipeline::isReady() const {
	return !currentShader().isError();
}

const char* SkinningPipeline::errorMessage() const {
	const char* message = mShader.errorMessage();
	if ( message[ 0 ] != '\0' ) {
		return message;
	}
	message = mGBufferShader.errorMessage();
	if ( message[ 0 ] != '\0' ) {
		return message;
	}
	return mShadowShader.errorMessage();
}

const Graphics::Shader& SkinningPipeline::currentShader() const {
	switch ( RenderPass::current() ) {
	case RenderPassKind::GBuffer: return mGBufferShader;
	case RenderPassKind::Shadow:  return mShadowShader;
	default:                      return mShader;
	}
}

void SkinningPipeline::begin( const std::vector< Matrix34 >& skinMatrices ) {
	int boneNumber = static_cast< int >( skinMatrices.size() );
	ASSERT( boneNumber <= kMaxBones && "SkinningPipeline : too many bones." );
	for ( int b = 0; b < boneNumber; ++b ) {
		ShaderConstants::copyMatrix34( skinMatrices[ b ], &mBoneRows[ b * kFloatsPerBone ] );
	}
	mBonePalette.set( mBoneRows.data(), boneNumber * kFloatsPerBone * static_cast< int >( sizeof( float ) ) );	// 使う骨のぶんだけ送る。残りの中身は不定になるが、頂点は骨の数より大きい番号を指さないので読まれない

	Graphics::Manager gm = Graphics::Manager::instance();
	gm.setShader( currentShader() );
	gm.setConstantBuffer( kBonePaletteSlot, mBonePalette );
}

void SkinningPipeline::end() {
	Graphics::Manager gm = Graphics::Manager::instance();
	gm.setShader( Graphics::Shader() );
	gm.setConstantBuffer( kBonePaletteSlot, Graphics::ConstantBuffer() );
}
