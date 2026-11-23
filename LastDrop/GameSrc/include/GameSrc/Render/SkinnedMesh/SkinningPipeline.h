#ifndef INCLUDED_SKINNING_PIPELINE_H
#define INCLUDED_SKINNING_PIPELINE_H

#include "GameLib/Graphics/Shader.h"
#include "GameLib/Graphics/ConstantBuffer.h"
#include "GameLib/Graphics/VertexFormat.h"
#include "GameLib/Math/Matrix34.h"
#include <vector>

// スキンメッシュを GPU で変形して描く道具一式（シェーダと、骨の行列を渡す定数バッファ）
// シェーダは 2 つ：G-Buffer へ描く Resource/Shader/SkinnedMeshGBuffer.hlsl と、
// 影のマップへ深度だけ描く SkinnedMeshShadow.hlsl
// どちらで描くかは begin のときの RenderPass で選ぶ。G-Buffer でも影でもない間は描けない
// 全スキンメッシュで1つを共有する（MeshCache が1つ作り、SkinnedMeshData へ貸す）。同じシェーダを何度もコンパイルしないため
// 使い方: begin( 骨の行列 ) → いつも通りパーツを描く（MeshMaterial::apply → drawIndexed）→ end
class SkinningPipeline{
public:
	static const int kMaxBones = 128;	// 1体の骨の数の上限。Skinning.hlsli の MAX_BONES と同じ値にする（定数バッファの大きさが決まる）

	// Graphics::Manager ができた後に作る（シェーダをコンパイルするため）
	SkinningPipeline();
	SkinningPipeline( const SkinningPipeline& ) = delete;
	SkinningPipeline& operator=( const SkinningPipeline& ) = delete;

	static const GameLib::Graphics::VertexFormat& vertexFormat();	// スキンメッシュの頂点の形（位置・法線・色・UV・骨の番号4つ・重み4つ）。並びは SkinnedMeshData の GPU 頂点と同じ

	bool isReady() const;				// 描けるか（RenderPass が GBuffer か Shadow で、そのシェーダが一度でもコンパイルできたか）。描けなければ、呼び出し側はスキンメッシュを描かない
	const char* errorMessage() const;	// シェーダ（G-Buffer の方が先）のコンパイルエラー。問題が無ければ空文字列

	void begin( const std::vector< GameLib::Math::Matrix34 >& skinMatrices );	// 骨の行列（置き場所×ポーズ×bindInverse。数は kMaxBones 以下）を渡し、今の RenderPass のシェーダで描く状態にする
	void end();																	// 固定パイプラインへ戻す（後から描く建物などに残さない）

private:
	const GameLib::Graphics::Shader& currentShader() const;	// 今の RenderPass で使うシェーダ（Shadow でなければ G-Buffer の方）
	GameLib::Graphics::Shader mGBufferShader;				// G-Buffer へ描く（RenderPass が GBuffer のとき）
	GameLib::Graphics::Shader mShadowShader;				// 影のマップへ深度だけ描く（RenderPass が Shadow のとき）
	GameLib::Graphics::ConstantBuffer mBonePalette;
	std::vector< float > mBoneRows;							// 定数バッファへ送る前の詰め直し（骨1本 = 3行 × float4）
};

#endif
