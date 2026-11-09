#ifndef INCLUDED_GBUFFER_H
#define INCLUDED_GBUFFER_H

#include "GameLib/Graphics/RenderTarget.h"
#include "GameLib/Graphics/DepthTarget.h"
#include "GameLib/Graphics/Shader.h"
#include "GameLib/Graphics/ConstantBuffer.h"
#include "GameLib/Graphics/Texture.h"

// G-Buffer に描く物の動きの種類（Taa_Plan.txt 6 章）。RT1 の a に書き、TAA が前のフレームのどこを読むかを分ける。
enum class MotionKind{
	Static,      // 動かない物（建物・車・置物・地面）。深度と前のフレームのカメラから前の位置を出す
	Moving,      // 動く物（ゾンビ・千切れた手足）。同じく深度から出し、近傍の色で強めに抑える
	FirstPerson, // 手元（自キャラの体・腕・銃・弾）。カメラと一緒に動くので、画面の上ではほぼ止まっている
};

// G-Buffer（ディファードで、色を出す前の面の情報を画面ぶん持つ描画先）。仕様は SummaryPlan/個別仕様/グラフィック/GBuffer_Plan.txt 4・6 章。
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
	// デカール（DecalBatch。SsrDecalMaterial_Plan.txt 8 章）を描く間。動かない物を描いた後・動く物の前に、begin〜end の中で呼ぶ。
	// 描画先を RT0（色）と RT1（法線）だけにし、depth は描画先から外して t5 から読めるようにする（デカールは深度から位置を戻す）。
	void beginDecals( GameLib::Graphics::DepthTarget depth );
	// 描画先を G-Buffer 3 枚と depth へ付け直す（消さない）。続けて動く物を描ける。
	void endDecals( GameLib::Graphics::DepthTarget depth );
	// これから描く物の動きの種類（begin で Static に戻る）。描く物の種類が変わる所で呼ぶ（GameScene::draw）。
	void setMotion( MotionKind );
	// 描き方を Forward へ戻し、静的メッシュのシェーダを外す。描画先はそのまま（次に描く所で付け直す）。
	void end();

	const GameLib::Graphics::RenderTarget& albedo() const;   // RT0
	const GameLib::Graphics::RenderTarget& normal() const;   // RT1
	const GameLib::Graphics::RenderTarget& material() const; // RT2（AO・粗さ・金属・発光）
	// 静的メッシュのシェーダのコンパイルエラー。問題が無ければ空文字列。
	const char* errorMessage() const;

private:
	// 水たまりの値（PuddleParams）と動きの種類を定数に入れて b4 に付け、模様を t12 に付ける（begin の中。描画先を付けた後）。
	void setPassConstants();

	static const int kTargetNumber = 3;
	GameLib::Graphics::RenderTarget mTargets[ kTargetNumber ]; // RT0 色・RT1 法線・RT2 AO と粗さと金属と発光
	GameLib::Graphics::Shader mStaticMeshShader;               // Resource/Shader/StaticMeshGBuffer.hlsl
	GameLib::Graphics::ConstantBuffer mSurfaceConstants;       // 次に描く物の材質値（RenderPass::setSurface が書き換える）
	GameLib::Graphics::Texture mDefaultOrm;                    // 材質の画像が無い材質に付ける白 1 画素（RenderPass::setSurface）
	GameLib::Graphics::Texture mPuddleMask;                    // 雨上がりの水たまりの模様（Resource/puddle_mask。tools/puddle_gen.py）
	GameLib::Graphics::ConstantBuffer mPassConstants;          // 水たまりの値（PuddleParams）と動きの種類（begin・setMotion のたびに書く）
	MotionKind mMotion;                                        // 今描いている物の動きの種類
};

#endif
