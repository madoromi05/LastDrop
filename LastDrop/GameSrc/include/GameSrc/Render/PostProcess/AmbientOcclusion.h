#ifndef INCLUDED_AMBIENT_OCCLUSION_H
#define INCLUDED_AMBIENT_OCCLUSION_H

#include "GameLib/Graphics/RenderTarget.h"
#include "GameLib/Graphics/DepthTarget.h"
#include "GameLib/Graphics/Shader.h"
#include "GameLib/Graphics/ConstantBuffer.h"
#include "GameLib/Math/Matrix44.h"

class GBuffer;

// SSAO（物の隙間の暗がり）を作る（シェーダは Resource/Shader/Ssao.hlsl・SsaoBlur.hlsl）。仕様は SummaryPlan/個別仕様/グラフィック/SsaoTranslucent_Plan.txt 5・6 章。
// G-Buffer の法線と深度を読み、画面の半分の大きさの描画先（RG16F。r に AO、g に目からの奥行き）へ描き、
// 奥行きを見たぼかしを横 → 縦の 2 回掛ける（1 枚目 → 2 枚目 → 1 枚目）。値は SsaoParams。
// 結果はライティング（環境光にだけ掛ける）と表示（F6 → Render の G-Buffer の「SSAO」）が、元の大きさへ拡大して読む（SsaoUpsample.hlsli）。
// DeferredLighting に入れないのは、自分の描画先（半分の大きさ・ぼかしの行き来）を持つから（霧は描画先も定数もライティングと同じだったので入れた）。
// 作るのは Graphics::Manager ができた後（シーンの中）。
class AmbientOcclusion{
public:
	AmbientOcclusion();
	AmbientOcclusion( const AmbientOcclusion& ) = delete;
	AmbientOcclusion& operator=( const AmbientOcclusion& ) = delete;

	// G-Buffer を描いた後、ライティングの前に呼ぶ。描画先を自分の描画先へ切り替えて描く（終わった後の描画先は呼んだ側で付け直す）。
	// depth：G-Buffer と一緒に描いた深度。projectionView：描いたときのカメラの射影×ビュー。
	// SSAO を切っている間（ssaoEnabled が false）は、結果を AO 1（遮られていない）で消すだけ。
	// noiseFrame：点の並びを回すノイズのフレームの番号（TemporalJitter::frameIndex。TAA でないときは 0。Taa_Plan.txt 8 章）。
	void draw( const GBuffer& gbuffer, GameLib::Graphics::DepthTarget depth, const GameLib::Math::Matrix44& projectionView, int noiseFrame );
	// 最後に draw した結果（r に AO、g に目からの奥行き m）。
	const GameLib::Graphics::RenderTarget& result() const;
	// シェーダのコンパイルエラー（作るシェーダ → ぼかしのシェーダの順に、最初に見つかった物）。問題が無ければ空文字列。
	const char* errorMessage() const;

private:
	// source を読んで destination へ、direction（1 画素ずつ進む向き）にぼかす。
	void blur( const GameLib::Graphics::RenderTarget& source, const GameLib::Graphics::RenderTarget& destination,
		const GameLib::Graphics::ConstantBuffer& direction );

	GameLib::Graphics::RenderTarget mOcclusion;     // 半分の大きさの AO と奥行き（作る先・結果）
	GameLib::Graphics::RenderTarget mBlurScratch;   // 横にぼかした途中（縦にぼかして mOcclusion へ戻す）
	GameLib::Graphics::Shader mShader;              // Resource/Shader/Ssao.hlsl
	GameLib::Graphics::Shader mBlurShader;          // Resource/Shader/SsaoBlur.hlsl
	GameLib::Graphics::ConstantBuffer mConstants;
	GameLib::Graphics::ConstantBuffer mBlurHorizontal; // ぼかす向き（中身は作るときに 1 回だけ書く）
	GameLib::Graphics::ConstantBuffer mBlurVertical;
};

#endif
