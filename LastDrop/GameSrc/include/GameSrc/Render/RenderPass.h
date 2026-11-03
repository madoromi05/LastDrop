#ifndef INCLUDED_RENDER_PASS_H
#define INCLUDED_RENDER_PASS_H

#include "GameLib/Graphics/Texture.h"

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
// 材質の画像（ORM）がある材質は、粗さ・金属に画像の値が掛かる（SsrDecalMaterial_Plan.txt 5-1）。
struct SurfaceParams{
	float roughness; // 粗さ（0〜1）
	float metallic;  // 金属（0〜1）
	float emission;  // 発光の強さ（0〜1）
	float puddle;    // 雨上がりの水たまりを書くか（0 か 1。道路だけ 1。SsrDecalMaterial_Plan.txt 6 章）
};

namespace RenderPass{

RenderPassKind current();
// G-Buffer へ描き始める。staticMeshShader：静的メッシュを G-Buffer へ描くシェーダ。
// surfaceConstants：材質値の入れ物（GBuffer.hlsli の b3 に付けてあること）。
// defaultOrm：材質の画像が無い材質に付ける白 1 画素の画像（掛けても値が変わらない）。どれも GBuffer が持ち、end まで生きていること。
void beginGBuffer( const GameLib::Graphics::Shader& staticMeshShader, GameLib::Graphics::ConstantBuffer* surfaceConstants,
	const GameLib::Graphics::Texture* defaultOrm );
// Forward へ戻す。
void endGBuffer();
// 影のマップへ描き始める。staticMeshShader：静的メッシュの深度だけを描くシェーダ（ShadowMap が持ち、end まで生きていること）。
void beginShadow( const GameLib::Graphics::Shader& staticMeshShader );
// Forward へ戻す。
void endShadow();
// beginGBuffer・beginShadow で渡されたシェーダ。GBuffer・Shadow の間だけ呼べる。
const GameLib::Graphics::Shader& staticMeshShader();
// 次に描く物の材質値と材質の画像（ORM。空なら白 1 画素）を G-Buffer のシェーダへ渡す。GBuffer の間だけ呼べる。
void setSurface( const SurfaceParams&, GameLib::Graphics::Texture orm );
// 直前に送った材質値と画像を忘れ、次の setSurface で必ず送り直させる。GBuffer の間だけ呼べる。
// G-Buffer の途中で描画先を付け直したとき（デカール）に呼ぶ。付け直すと t2 以降の画像が全部外れ、材質の画像（t11）も外れるため。
void forgetSentSurface();
// メッシュを 1 回描くたびに drawIndexed の直前で呼ぶ。今のパスの描いた回数（Draw.gbuffer・Draw.shadow）を足す（OptimizationA_Plan.txt 5 章）。
// triangleNumber：この 1 回で描く三角形の数（まとめて描くなら全部の分）。G-Buffer の間だけ Draw.triangles に足す（LOD の効きを見る。OptimizationC_Plan.txt 5 章）。
// Forward では数えない。
void countDraw( int triangleNumber );

} //namespace RenderPass

#endif
