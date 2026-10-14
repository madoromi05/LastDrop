#ifndef INCLUDED_SKINNING_PIPELINE_H
#define INCLUDED_SKINNING_PIPELINE_H

#include "GameLib/Graphics/Shader.h"
#include "GameLib/Graphics/ConstantBuffer.h"
#include "GameLib/Graphics/VertexFormat.h"
#include "GameLib/Math/Matrix34.h"
#include <vector>

// スキンメッシュを GPU で変形して描く道具一式（シェーダ Resource/Shader/SkinnedMesh.hlsl と、骨の行列を渡す定数バッファ）。
// 全スキンメッシュで1つを共有する（MeshCache が1つ作り、SkinnedMeshData へ貸す）。同じシェーダを何度もコンパイルしないため。
// 使い方: begin( 骨の行列 ) → いつも通りパーツを描く（MeshMaterial::apply → drawIndexed）→ end
class SkinningPipeline{
public:
	// 1体の骨の数の上限。SkinnedMesh.hlsl の MAX_BONES と同じ値にする（定数バッファの大きさが決まる）。
	static const int kMaxBones = 128;

	// Graphics::Manager ができた後に作る（シェーダをコンパイルするため）。
	SkinningPipeline();
	SkinningPipeline( const SkinningPipeline& ) = delete;
	SkinningPipeline& operator=( const SkinningPipeline& ) = delete;

	// スキンメッシュの頂点の形（位置・法線・色・UV・骨の番号4つ・重み4つ）。並びは SkinnedMeshData の GPU 頂点と同じ。
	static const GameLib::Graphics::VertexFormat& vertexFormat();

	// 描けるか（シェーダが一度でもコンパイルできたか）。描けなければ、呼び出し側はスキンメッシュを描かない。
	bool isReady() const;
	// シェーダのコンパイルエラー。問題が無ければ空文字列。
	const char* errorMessage() const;

	// 骨の行列（置き場所×ポーズ×bindInverse。数は kMaxBones 以下）を渡し、このシェーダで描く状態にする。
	void begin( const std::vector< GameLib::Math::Matrix34 >& skinMatrices );
	// 固定パイプラインへ戻す（後から描く建物などに残さない）。
	void end();

private:
	GameLib::Graphics::Shader mShader;
	GameLib::Graphics::ConstantBuffer mBonePalette;
	std::vector< float > mBoneRows; // 定数バッファへ送る前の詰め直し（骨1本 = 3行 × float4）
};

#endif
