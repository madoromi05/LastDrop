#ifndef INCLUDED_DEFERRED_LIGHTING_H
#define INCLUDED_DEFERRED_LIGHTING_H

#include "GameLib/Graphics/Shader.h"
#include "GameLib/Graphics/ConstantBuffer.h"
#include "GameLib/Graphics/DepthTarget.h"
#include "GameLib/Math/Matrix44.h"

class GBuffer;

// G-Buffer に描いた中身のどれを画面に出すか（GBuffer_Plan.txt 8-2）。番号は Resource/Shader/DeferredLighting.hlsl と揃える。
enum class GBufferView{
	Final,     // いつもの絵（描き方は RenderPath で決まる）
	Albedo,    // RT0 の色
	Normal,    // RT1 の法線（-1〜1 を 0〜1 の色に）
	Specular,  // RT2 のスペキュラの色
	Sharpness, // RT1 a の鋭さ（灰色。kSharpnessViewMax で白）
	Depth,     // 目からの距離（灰色。kDepthViewRange で白）
	Position,  // 深度から戻したワールドの位置の小数部（1m の格子）
	Count,
};
// エディタ（F6 の Render）に出す名前。
const char* gbufferViewName( GBufferView );
// 起動中ずっと 1 つの表示の選択（F6 で書き換える。Release では Final のまま）。
GBufferView& gbufferView();

// いつもの絵をどちらの描き方で描くか（GBuffer_Plan.txt 8-1）。フォワードは段階 3 の画面確認まで、見比べと計測のために残す。
enum class RenderPath{
	Deferred, // G-Buffer へ描いてから、全画面 1 回でライティングする
	Forward,  // 描いたその場で色を出す（2-a までの描き方）
};
// 起動中ずっと 1 つの描き方の選択（F6 で書き換える。Release では Deferred のまま）。
RenderPath& renderPath();

// G-Buffer を読んで画面を作る全画面のパス（シェーダは Resource/Shader/DeferredLighting.hlsl）。仕様は GBuffer_Plan.txt 7・8 章。
// ライティング（drawLighting。今の描き方と同じ式でシーンの色を出す）と、G-Buffer の中身の表示（drawView）の 2 つ。
// 作るのは Graphics::Manager ができた後（シーンの中）。
class DeferredLighting{
public:
	DeferredLighting();
	DeferredLighting( const DeferredLighting& ) = delete;
	DeferredLighting& operator=( const DeferredLighting& ) = delete;

	// G-Buffer と深度を読み、今と同じ式で照らした色を今の描画先（シーンの色）へ描く。何も描かれていない画素は触らない。
	// 描画先に depth を付けたまま呼ばないこと（同じ深度を描画先とシェーダの両方には付けられない。PostProcess::bindSceneColor）。
	// ライト・目の位置は Graphics::Manager に設定済みの物（GameScene::setupLights・カメラ）をそのまま読む。
	void drawLighting( const GBuffer& gbuffer, GameLib::Graphics::DepthTarget depth, const GameLib::Math::Matrix44& projectionView );
	// view（Final 以外）を、今の描画先へ全画面で写す。ToneMap を通さず中身の数のまま見るため、PostProcess::endScene の後
	// （描画先が画面に戻った後）に呼ぶ。depth：G-Buffer と一緒に描いた深度。projectionView：描いたときのカメラの射影×ビュー。
	void drawView( GBufferView view, const GBuffer& gbuffer, GameLib::Graphics::DepthTarget depth,
		const GameLib::Math::Matrix44& projectionView );
	// シェーダのコンパイルエラー。問題が無ければ空文字列。
	const char* errorMessage() const;

private:
	// view を定数に入れ、G-Buffer と深度を付けて全画面を 1 回描く（drawLighting と drawView の共通部分）。
	void drawFullScreen( GBufferView view, const GBuffer& gbuffer, GameLib::Graphics::DepthTarget depth,
		const GameLib::Math::Matrix44& projectionView );
	GameLib::Graphics::Shader mShader;
	GameLib::Graphics::ConstantBuffer mConstants;
};

#endif
