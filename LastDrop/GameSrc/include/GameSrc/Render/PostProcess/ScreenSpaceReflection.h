#ifndef INCLUDED_SCREEN_SPACE_REFLECTION_H
#define INCLUDED_SCREEN_SPACE_REFLECTION_H

#include "GameLib/Graphics/RenderTarget.h"
#include "GameLib/Graphics/DepthTarget.h"
#include "GameLib/Graphics/Shader.h"
#include "GameLib/Graphics/ConstantBuffer.h"
#include "GameLib/Math/Matrix44.h"
#include "GameSrc/Render/PostProcess/HiZBuffer.h"

class GBuffer;

// 映り込み（SSR）を作る（シェーダは Resource/Shader/Ssr.hlsl・SsrBlur.hlsl）。仕様は SummaryPlan/個別仕様/SsrDecalMaterial_Plan.txt 7 章。
// ライティングした後のシーンの色・G-Buffer の法線と粗さ・深度を読み、画面の半分の大きさの描画先（RGBA16F。rgb 映る色 × 確からしさ・
// a 確からしさ）へ描き、奥行きを見たぼかしを横 → 縦の 2 回掛ける（SSAO の AmbientOcclusion と同じ形）。値は SsrParams。
// 進み方は Hi-Z（HiZBuffer。HiZSsr_Plan.txt 6 章）か、今までの直線（F6 の Hi-Z を切ったとき）。
// シーンの色へ混ぜるのは DeferredLighting::drawReflection（G-Buffer と深度を読む全画面のパスなので、ライティング・霧と同じ所に置く）。
// 作るのは Graphics::Manager ができた後（シーンの中）。
class ScreenSpaceReflection{
public:
	ScreenSpaceReflection();
	ScreenSpaceReflection( const ScreenSpaceReflection& ) = delete;
	ScreenSpaceReflection& operator=( const ScreenSpaceReflection& ) = delete;

	// Hi-Z を作る（F6 の Hi-Z を切っている間は何もしない）。draw の前に、同じフレームの深度で呼ぶ。
	// 描画先を自分の描画先へ切り替える（終わった後の描画先は呼んだ側で付け直す）。計測の印を分けるため draw と別にしてある。
	void buildHiZ( GameLib::Graphics::DepthTarget depth );
	// ライティングの後、霧の前に呼ぶ。描画先を自分の描画先へ切り替えて描く（終わった後の描画先は呼んだ側で付け直す）。
	// sceneColor：ライティングした後のシーンの色（描画先から外してあること）。depth：G-Buffer と一緒に描いた深度。
	// projectionView：描いたときのカメラの射影×ビュー。
	// noiseFrame：歩き始めをずらすノイズのフレームの番号（TemporalJitter::frameIndex。TAA でないときは 0。Taa_Plan.txt 8 章）。
	void draw( const GBuffer& gbuffer, const GameLib::Graphics::RenderTarget& sceneColor, GameLib::Graphics::DepthTarget depth,
		const GameLib::Math::Matrix44& projectionView, int noiseFrame );
	// 映り込みを作らなかったフレーム（SSR を切っている間）に、結果を「映り込み無し」で消す（G-Buffer の表示が古い絵を出さないように）。
	void clear();
	// 最後に draw した結果。
	const GameLib::Graphics::RenderTarget& result() const;
	// 最後に作った Hi-Z（G-Buffer の表示の Hi-Z が読む。作っていない間は前に作った物のまま）。
	const GameLib::Graphics::RenderTarget& hiZ() const;
	// シェーダのコンパイルエラー（作るシェーダ → ぼかしのシェーダ → Hi-Z のシェーダの順に、最初に見つかった物）。問題が無ければ空文字列。
	const char* errorMessage() const;

private:
	// source を読んで destination へ、directionX・directionY（1 画素ずつ進む向き）にぼかす。粗さは gbuffer の材質から読む。
	void blur( const GameLib::Graphics::RenderTarget& source, const GameLib::Graphics::RenderTarget& destination,
		const GBuffer& gbuffer, GameLib::Graphics::DepthTarget depth, const GameLib::Math::Matrix44& inverse, const GameLib::Math::Matrix44& projectionView,
		float directionX, float directionY );

	GameLib::Graphics::RenderTarget mReflection;   // 半分の大きさの映り込み（作る先・結果）
	GameLib::Graphics::RenderTarget mBlurScratch;  // 横にぼかした途中（縦にぼかして mReflection へ戻す）
	GameLib::Graphics::Shader mShader;             // Resource/Shader/Ssr.hlsl
	GameLib::Graphics::Shader mBlurShader;         // Resource/Shader/SsrBlur.hlsl
	GameLib::Graphics::ConstantBuffer mConstants;
	GameLib::Graphics::ConstantBuffer mBlurConstants; // ぼかしの向きは回ごとに書き換える
	HiZBuffer mHiZ;                                // 深度の縮小の段（Hi-Z で進むとき）
};

#endif
