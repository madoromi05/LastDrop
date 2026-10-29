#ifndef INCLUDED_RENDER_PASS_H
#define INCLUDED_RENDER_PASS_H

namespace GameLib{
namespace Graphics{
	class Shader;
	class ConstantBuffer;
}
}

// 今どの描き方で物を描いているか（SummaryPlan/個別仕様/GBuffer_Plan.txt 6-2）。
//   Forward：描いたその場で色を出す（固定パイプライン・SkinnedMesh.hlsl。ゲーム本編の外のテスト用シーン）
//   GBuffer：色を出さず、面の情報（色・法線・材質）を G-Buffer へ書く
//   Shadow ：光の側から見た深度だけを影のマップへ書く（Shadow_Plan.txt 4 章）
// 変えるのは GBuffer::begin / end と ShadowMap::begin / end だけ。読むのはシェーダを選ぶ 2 か所（StaticMeshPart::draw・SkinningPipeline::begin）と、
// 材質値を渡す MeshMaterial::apply だけで、それ以外の物（建物・ゾンビなど）の draw は、どちらで描かれているかを知らない。
enum class RenderPassKind{
	Forward,
	GBuffer,
	Shadow,
};

// G-Buffer へ書く PBR の材質値（PbrNightLighting_Plan.txt 5 章）。並びは Resource/Shader/GBuffer.hlsli の GBufferMaterial と揃える。
struct SurfaceParams{
	float roughness; // 粗さ（0〜1）
	float metallic;  // 金属（0〜1）
	float emission;  // 発光の強さ（0〜1）
};

namespace RenderPass{

RenderPassKind current();
// G-Buffer へ描き始める。staticMeshShader：静的メッシュを G-Buffer へ描くシェーダ。
// surfaceConstants：材質値の入れ物（GBuffer.hlsli の b3 に付けてあること）。どちらも GBuffer が持ち、end まで生きていること。
void beginGBuffer( const GameLib::Graphics::Shader& staticMeshShader, GameLib::Graphics::ConstantBuffer* surfaceConstants );
// Forward へ戻す。
void endGBuffer();
// 影のマップへ描き始める。staticMeshShader：静的メッシュの深度だけを描くシェーダ（ShadowMap が持ち、end まで生きていること）。
void beginShadow( const GameLib::Graphics::Shader& staticMeshShader );
// Forward へ戻す。
void endShadow();
// beginGBuffer・beginShadow で渡されたシェーダ。GBuffer・Shadow の間だけ呼べる。
const GameLib::Graphics::Shader& staticMeshShader();
// 次に描く物の材質値を G-Buffer のシェーダへ渡す。GBuffer の間だけ呼べる。
void setSurface( const SurfaceParams& );

} //namespace RenderPass

#endif
