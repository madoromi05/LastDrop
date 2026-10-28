#ifndef INCLUDED_GBUFFER_H
#define INCLUDED_GBUFFER_H

#include "GameLib/Graphics/RenderTarget.h"
#include "GameLib/Graphics/DepthTarget.h"
#include "GameLib/Graphics/Shader.h"

// G-Buffer（ディファードで、色を出す前の面の情報を画面ぶん持つ描画先）。仕様は SummaryPlan/個別仕様/GBuffer_Plan.txt 4・6 章。
// 3 枚の並び（何をどの形式で持つか）は Resource/Shader/GBuffer.hlsli に書いてあり、形式はここと揃える。
// 深度は持たず、PostProcess の深度を借りて一緒に描く（後から描く半透明の板も同じ深度で隠れるように）。
//
// 使い方：begin( 深度 ) → いつもどおり不透明な物を描く → end。間は RenderPass が GBuffer になり、
// 静的メッシュとキャラは G-Buffer へ描くシェーダへ切り替わる（StaticMeshPart::draw・SkinningPipeline::begin）。
// 作るのは Graphics::Manager ができた後（シーンの中）。
class GBuffer{
public:
	GBuffer();
	GBuffer( const GBuffer& ) = delete;
	GBuffer& operator=( const GBuffer& ) = delete;

	// 描画先を G-Buffer 3 枚と depth にし、3 枚を消す（depth は消さない。消すのは持ち主の PostProcess::beginScene）。
	void begin( GameLib::Graphics::DepthTarget depth );
	// 描き方を Forward へ戻し、静的メッシュのシェーダを外す。描画先はそのまま（次に描く所で付け直す）。
	void end();

	const GameLib::Graphics::RenderTarget& albedo() const;   // RT0
	const GameLib::Graphics::RenderTarget& normal() const;   // RT1
	const GameLib::Graphics::RenderTarget& specular() const; // RT2
	// 静的メッシュのシェーダのコンパイルエラー。問題が無ければ空文字列。
	const char* errorMessage() const;

private:
	static const int kTargetNumber = 3;
	GameLib::Graphics::RenderTarget mTargets[ kTargetNumber ]; // RT0 色・RT1 法線と鋭さ・RT2 スペキュラの色
	GameLib::Graphics::Shader mStaticMeshShader;               // Resource/Shader/StaticMeshGBuffer.hlsl
};

#endif
