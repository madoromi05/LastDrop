#ifndef INCLUDED_DEFERRED_LIGHTING_H
#define INCLUDED_DEFERRED_LIGHTING_H

#include "GameLib/Graphics/Shader.h"
#include "GameLib/Graphics/ConstantBuffer.h"
#include "GameLib/Graphics/DepthTarget.h"
#include "GameLib/Graphics/RenderTarget.h"
#include "GameLib/Graphics/Texture.h"
#include "GameLib/Math/Matrix44.h"

class GBuffer;

// G-Buffer に描いた中身のどれを画面に出すか（GBuffer_Plan.txt 8-2）。番号は Resource/Shader/DeferredLighting.hlsl と揃える。
enum class GBufferView{
	Final,     // いつもの絵（ライティングした色）
	Albedo,    // RT0 の色
	Normal,    // RT1 の法線（-1〜1 を 0〜1 の色に）
	Roughness, // RT2 g の粗さ（灰色。0 黒＝つるつる・1 白＝ざらざら）
	Metallic,  // RT2 b の金属（0 黒・1 白）
	Emission,  // RT2 a の発光の強さ（0〜1 のまま）
	Depth,     // 目からの距離（灰色。kDepthViewRange で白）
	Position,  // 深度から戻したワールドの位置の小数部（1m の格子）
	Cascade,   // 月の影のどの枚を使っているか（1 枚目 赤・2 枚目 緑・3 枚目 青・範囲の外は色なし。Shadow_Plan.txt 9 章）
	ShadowMaps, // 影のマップ 4 枚を画面を 4 つに分けて（左上 懐中電灯・右上 月の 1 枚目・左下 2 枚目・右下 3 枚目。光に近いほど黒）
	Fog,       // 霧の量（0 黒〜1 白。空は黒。NightFinish_Plan.txt 9 章）
	Ssao,      // SSAO の AO（0 黒＝遮られた〜1 白。空は黒。SsaoTranslucent_Plan.txt 8 章）
	Ssr,       // 映り込みの確からしさ（0 黒〜1 白。SsrDecalMaterial_Plan.txt 9 章）
	SsrColor,  // 映り込みの色（確からしさで割った、映る物の色そのもの。ToneMap を通さないので暗い）
	Motion,    // 動きの種類（RT1 の a。黒 動かない物・青 ゾンビなど動く物・赤 手元。Taa_Plan.txt 6 章）
	HiZ,       // Hi-Z の段（F6 の Hi-Z view level で選ぶ）を、目からの距離の灰色で（Depth と同じ物差し。空は黒。HiZSsr_Plan.txt 8 章）
	Count,
};
// エディタ（F6 の Render）に出す名前。
const char* gbufferViewName( GBufferView );
// 起動中ずっと 1 つの表示の選択（F6 で書き換える。Release では Final のまま）。
GBufferView& gbufferView();

// G-Buffer と深度を読んで画面を作る全画面のパス（シェーダは Resource/Shader/DeferredLighting.hlsl・Fog.hlsl）。
// 仕様は GBuffer_Plan.txt 7・8 章、PBR の式は PbrNightLighting_Plan.txt 4 章、霧は NightFinish_Plan.txt 5 章。
// ライティング（drawLighting。PBR の式でシーンの色を出す）・映り込みを混ぜる（drawReflection）・霧（drawFog）・
// G-Buffer の中身の表示（drawView）の 4 つ。
// 霧と映り込みを混ぜるパスもここに置くのは、深度から位置を戻す定数（射影×ビューの逆）と G-Buffer・深度の番号がライティングと同じだから。
// 作るのは Graphics::Manager ができた後（シーンの中）。
class DeferredLighting{
public:
	DeferredLighting();
	DeferredLighting( const DeferredLighting& ) = delete;
	DeferredLighting& operator=( const DeferredLighting& ) = delete;

	// G-Buffer と深度を読み、PBR の式で照らした色を今の描画先（シーンの色）へ描く。何も描かれていない画素には空を描く（ライトの一覧の空の色）。
	// 描画先に depth を付けたまま呼ばないこと（同じ深度を描画先とシェーダの両方には付けられない。PostProcess::bindSceneColor）。
	// ライト・空・環境光は付けてあるライトの一覧（LightList::bind。b2）、目の位置は Graphics::Manager に設定済みの物（カメラ）を読む。
	// occlusion：SSAO の結果（AmbientOcclusion::result）。元の大きさへ拡大して、環境光にだけ掛ける（SsaoTranslucent_Plan.txt 6-4）。
	void drawLighting( const GBuffer& gbuffer, const GameLib::Graphics::RenderTarget& occlusion, GameLib::Graphics::DepthTarget depth,
		const GameLib::Math::Matrix44& projectionView );
	// 深度を読み、霧の色を霧の量の割合で今の描画先（シーンの色）へ混ぜる。drawLighting の直後、同じ描画先のまま呼ぶ。
	// 霧の値と色は付けてあるライトの一覧（b2）から読む。空の画素は変えない。
	void drawFog( GameLib::Graphics::DepthTarget depth, const GameLib::Math::Matrix44& projectionView );
	// 映り込み（SSR の結果 reflection。ScreenSpaceReflection::result）を、今の描画先（シーンの色）へ混ぜる。ライティングの後・霧の前に、
	// 描画先をシーンの色だけにして呼ぶ。環境光の鏡面（空と地面の色）を、映り込みの確からしさの分だけ SSR の色へ入れ替える
	// （SsrDecalMaterial_Plan.txt 7-3。BLEND_ADDITIVE）。occlusion：SSAO の結果（環境光の鏡面と同じ AO を掛けるため）。
	void drawReflection( const GBuffer& gbuffer, const GameLib::Graphics::RenderTarget& occlusion, const GameLib::Graphics::RenderTarget& reflection,
		GameLib::Graphics::DepthTarget depth, const GameLib::Math::Matrix44& projectionView );
	// view（Final 以外）を、今の描画先へ全画面で写す。ToneMap を通さず中身の数のまま見るため、PostProcess::endScene の後
	// （描画先が画面に戻った後）に呼ぶ。occlusion：SSAO の結果（AmbientOcclusion::result。表示の SSAO が読む）。
	// reflection：SSR の結果（表示の SSR・SSR color が読む）。hiZ：Hi-Z の全部の段（表示の Hi-Z が読む。ScreenSpaceReflection::hiZ）。
	// depth：G-Buffer と一緒に描いた深度。projectionView：描いたときのカメラの射影×ビュー。
	void drawView( GBufferView view, const GBuffer& gbuffer, const GameLib::Graphics::RenderTarget& occlusion,
		const GameLib::Graphics::RenderTarget& reflection, const GameLib::Graphics::RenderTarget& hiZ, GameLib::Graphics::DepthTarget depth,
		const GameLib::Math::Matrix44& projectionView );
	// シェーダのコンパイルエラー（ライティング → 霧 → 映り込みを混ぜるシェーダの順に、最初に見つかった物）。問題が無ければ空文字列。
	const char* errorMessage() const;

private:
	// view と、深度から位置を戻す逆行列を定数に入れて b1 に付ける（3 つのパスの共通部分）。
	void setConstants( GBufferView view, const GameLib::Math::Matrix44& projectionView );
	// view を定数に入れ、G-Buffer と SSAO の結果と深度を付けて全画面を 1 回描く（drawLighting と drawView の共通部分）。
	void drawFullScreen( GBufferView view, const GBuffer& gbuffer, const GameLib::Graphics::RenderTarget& occlusion,
		GameLib::Graphics::DepthTarget depth, const GameLib::Math::Matrix44& projectionView );
	GameLib::Graphics::Shader mShader;    // ライティングと G-Buffer の表示（DeferredLighting.hlsl）
	GameLib::Graphics::Shader mFogShader; // 霧（Fog.hlsl）
	GameLib::Graphics::Shader mReflectionShader; // 映り込みを混ぜる（SsrComposite.hlsl）
	GameLib::Graphics::ConstantBuffer mConstants;
	GameLib::Graphics::Texture mMoonTexture; // 空に描く月の模様（Resource/moon.dds。SkyDetail.hlsli の t14。MoonStarsLut_Plan.txt 4 章）
};

#endif
