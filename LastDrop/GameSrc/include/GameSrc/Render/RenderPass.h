#ifndef INCLUDED_RENDER_PASS_H
#define INCLUDED_RENDER_PASS_H

namespace GameLib{
namespace Graphics{
	class Shader;
}
}

// 今どの描き方で物を描いているか（SummaryPlan/個別仕様/GBuffer_Plan.txt 6-2）。
//   Forward：描いたその場で色を出す（固定パイプライン・SkinnedMesh.hlsl）
//   GBuffer：色を出さず、面の情報（色・法線・材質）を G-Buffer へ書く
// 変えるのは GBuffer::begin / end だけ。読むのはシェーダを選ぶ 2 か所（StaticMeshPart::draw・SkinningPipeline::begin）だけで、
// それ以外の物（建物・ゾンビなど）の draw は、どちらで描かれているかを知らない。
// 段階 4 の影のパス（深度だけ描く）も、ここへ種類を足して同じ 2 か所で選ぶ。
enum class RenderPassKind{
	Forward,
	GBuffer,
};

namespace RenderPass{

RenderPassKind current();
// G-Buffer へ描き始める。staticMeshShader：静的メッシュを G-Buffer へ描くシェーダ（GBuffer が持つ。end まで生きていること）。
void beginGBuffer( const GameLib::Graphics::Shader& staticMeshShader );
// Forward へ戻す。
void endGBuffer();
// beginGBuffer で渡されたシェーダ。GBuffer の間だけ呼べる。
const GameLib::Graphics::Shader& staticMeshShader();

} //namespace RenderPass

#endif
