#ifndef INCLUDED_TEMPORAL_ANTI_ALIASING_H
#define INCLUDED_TEMPORAL_ANTI_ALIASING_H

#include "GameLib/Graphics/RenderTarget.h"
#include "GameLib/Graphics/DepthTarget.h"
#include "GameLib/Graphics/Shader.h"
#include "GameLib/Graphics/ConstantBuffer.h"
#include "GameLib/Math/Matrix44.h"

class GBuffer;

// TAA（時間をまたいだアンチエイリアス。シェーダは Resource/Shader/Taa.hlsl・TaaCopy.hlsl）。仕様は SummaryPlan/個別仕様/グラフィック/Taa_Plan.txt 7 章。
// 履歴（前のフレームまでに混ぜた色。RGBA16F・画面の大きさ）を 2 枚持って行き来し、今のシーンの色と混ぜた物を新しい履歴へ書き、
// それをシーンの色へ写し戻す（シーンの色を読みながらシーンの色へは書けないため）。値は TaaParams。
// 描く行列のずらしは TemporalJitter（GameScene）。ここは前のフレームの「ずらしていない射影×ビュー」を覚えておく。
// 作るのは Graphics::Manager ができた後（シーンの中）。
class TemporalAntiAliasing{
public:
	TemporalAntiAliasing();
	TemporalAntiAliasing( const TemporalAntiAliasing& ) = delete;
	TemporalAntiAliasing& operator=( const TemporalAntiAliasing& ) = delete;

	// 霧の後・半透明の板の前に呼ぶ。sceneColor を読んで混ぜ、sceneColor へ写し戻す（描画先は sceneColor だけになる。
	// 続きを描く側が付け直す）。gbuffer：RT1 の a の動きの種類を読む。depth：G-Buffer と一緒に描いた深度。
	// jitteredProjectionView：このフレームに描いた（ずらした）射影×ビュー。projectionView：同じカメラのずらしていない射影×ビュー。
	// 履歴が無い（最初・invalidate の後・画面の大きさが変わった）フレームは、今の色をそのまま履歴にする。
	void resolve( const GameLib::Graphics::RenderTarget& sceneColor, const GBuffer& gbuffer, GameLib::Graphics::DepthTarget depth,
		const GameLib::Math::Matrix44& jitteredProjectionView, const GameLib::Math::Matrix44& projectionView );
	// 履歴を捨てる（次の resolve は今の色だけ）。TAA を使っていないフレーム・カメラが飛んだときに呼ぶ。
	void invalidate();
	// シェーダのコンパイルエラー（混ぜるシェーダ → 写し戻すシェーダの順に、最初に見つかった物）。問題が無ければ空文字列。
	const char* errorMessage() const;

private:
	static const int kHistoryNumber = 2;
	GameLib::Graphics::RenderTarget mHistory[ kHistoryNumber ]; // 前の履歴と新しい履歴を、フレームごとに入れ替える
	int mCurrent;                                               // 前のフレームに書いた履歴の番号
	bool mValid;                                                // 前の履歴が使えるか
	int mWidth;                                                 // 前の履歴を書いたときの画面の大きさ（変わったら捨てる）
	int mHeight;
	GameLib::Math::Matrix44 mPreviousProjectionView;            // 前のフレームの「ずらしていない」射影×ビュー
	GameLib::Graphics::Shader mShader;                          // Resource/Shader/Taa.hlsl
	GameLib::Graphics::Shader mCopyShader;                      // Resource/Shader/TaaCopy.hlsl
	GameLib::Graphics::ConstantBuffer mConstants;
};

#endif
