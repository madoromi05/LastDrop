#ifndef INCLUDED_STATIC_MESH_INSTANCING_H
#define INCLUDED_STATIC_MESH_INSTANCING_H

#include "GameLib/Graphics/Shader.h"
#include "GameLib/Graphics/ConstantBuffer.h"
#include "GameLib/Math/Matrix34.h"
#include <vector>

// 静的メッシュをインスタンシングで描く道具一式（シェーダと、置き場所の行列を渡す定数バッファ。OptimizationA_Plan.txt 8 章）
// 作りはゾンビの骨の行列（SkinningPipeline）と同じ：行列を b1 に並べ、シェーダが SV_InstanceID で何軒目かを読む
// シェーダは 2 つ：G-Buffer へ描く Resource/Shader/StaticMeshInstancedGBuffer.hlsl、影のマップへ深度だけ描く StaticMeshInstancedShadow.hlsl
// どちらで描くかは begin のときの RenderPass で選ぶ（Forward では使えない）
// 集めた物を描く側（StaticMeshBatch）が借りる。同じシェーダを何度もコンパイルしないため、描く係（WorldRenderer）が 1 つだけ持つ
// 使い方: begin → ( setPlacements → StaticMeshData::drawInstanced ) を繰り返す → end
class StaticMeshInstancing{
public:
	// 1 回の描画で置ける数の上限。StaticMeshInstancing.hlsli の MAX_INSTANCES と同じ値にする（定数バッファの大きさが決まる。128 × 48 バイト ＝ 6KB）
	static const int kMaxInstances = 128;

	// Graphics::Manager ができた後に作る（シェーダをコンパイルするため）
	StaticMeshInstancing();
	StaticMeshInstancing( const StaticMeshInstancing& ) = delete;
	StaticMeshInstancing& operator=( const StaticMeshInstancing& ) = delete;

	// 今の RenderPass で描けるか（G-Buffer か影の間で、そのシェーダがコンパイルできたか）
	// 描けなければ、呼び出し側は 1 つずつの描き方（StaticMeshInstance::draw）で描く
	bool isReady() const;
	// シェーダ（G-Buffer の方が先）のコンパイルエラー。問題が無ければ空文字列
	const char* errorMessage() const;

	// 今の RenderPass のシェーダと置き場所の定数バッファを付け、ワールド行列を単位行列にする（置き場所は b1 から読むため）。isReady() のときだけ呼ぶ
	void begin();
	// 次の描画の置き場所（モデルの空間 → ワールド）。number は 1〜kMaxInstances。begin〜end の間だけ呼ぶ。placements: nullptr 不可
	void setPlacements( const GameLib::Math::Matrix34* placements, int number );
	// シェーダと定数バッファを外す（後から描く物に残さない）
	void end();

private:
	// 今の RenderPass で使うシェーダ
	const GameLib::Graphics::Shader& currentShader() const;

	GameLib::Graphics::Shader mGBufferShader; // G-Buffer へ描く（RenderPass が GBuffer のとき）
	GameLib::Graphics::Shader mShadowShader;  // 影のマップへ深度だけ描く（RenderPass が Shadow のとき）
	GameLib::Graphics::ConstantBuffer mPlacementBuffer;
	std::vector< float > mPlacementRows; // 定数バッファへ送る前の詰め直し（1 軒 = 3 行 × float4）
};

// 建物をインスタンシングで描くか（F6 → Render の Instancing。見比べと計測用）
// false の間は 1 軒ずつ描く（今までの描き方）。起動中ずっと 1 つ（StaticMeshInstancing.cpp が持つ。初期値 true。Release では true のまま）
bool& instancingEnabled();

#endif
