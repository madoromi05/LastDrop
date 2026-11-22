#ifndef INCLUDED_POST_PROCESS_H
#define INCLUDED_POST_PROCESS_H

#include "GameLib/Graphics/RenderTarget.h"
#include "GameLib/Graphics/DepthTarget.h"
#include "GameLib/Graphics/Shader.h"
#include "GameLib/Graphics/ConstantBuffer.h"
#include "GameSrc/Render/PostProcess/Bloom.h"
#include "GameSrc/Render/PostProcess/ColorLut.h"

// 画面全体に掛ける処理（ポストプロセス）の入口
// beginScene と endScene の間に普段どおり描くと、画面ではなく中間の描画先（色と深度）へ描かれ、
// endScene でシェーダ（Resource/Shader/Present.hlsl）を通して画面へ写す
// 中間の描画先はリニア（光の量に比例した数）で持ち、beginScene〜endScene の間は Graphics::Manager::enableLinearColor を入れておく
// 写すときに、明るい所のにじみ（ブルーム。Bloom）を足し、露出を掛けて ACES で 0〜1 に収め（トーンマップ）、
// 周辺を少し暗くして（ビネット）、最後に画面の色（sRGB）へ戻す（LinearToneMap_Plan.txt 6 章）。露出とブルームの値は PostProcessParams
// ビネットの所に赤を足す強さ・周りのぼかしと暗さ（攻撃された瞬間）と、全体を暗くする量（死んだ後）を外から動かせる
// 値はこのシーンの PostProcess だけが持つので、シーンを作り直せば 0 に戻る（次のシーンへ漏れない）
// トーンマップの後にカラーグレーディング、画面の色へ戻した後にディザを掛ける
// アンチエイリアスが FXAA（antiAliasingMode）のときは、写した結果をいったん 8bit の中間（a に輝度）へ置き、Resource/Shader/Fxaa.hlsl で輪郭を
// なめらかにしながら画面へ写す。OFF のときは今までどおり画面へ直接写す
//
// 写した後の画面には、文字（DebugScreen）など Framework が後から描く物がそのまま重なる（ポストプロセスは掛からない）
// 作るのは Graphics::Manager ができた後（シーンの中）
class PostProcess{
public:
	PostProcess();
	PostProcess( const PostProcess& ) = delete;
	PostProcess& operator=( const PostProcess& ) = delete;

	// 描画先を中間の描画先へ切り替え、色と深度を消す。シーンを描く前に呼ぶ
	// ここから endScene までは、材質・ライト・頂点の色がリニアへ直される（Graphics::Manager::enableLinearColor）
	void beginScene();
	// 描画先を中間の描画先（色と深度）へ付け直す（消さない）。beginScene の後に別の描画先（G-Buffer）へ描いたとき、
	// 続きをシーンへ描く前に呼ぶ
	void resumeScene();
	// resumeScene と同じだが、深度は奥行きの判定だけに使い（書き込まない）、同じ深度を t5 から読めるようにも付ける
	// 半透明の板（LitQuadBatch）を、物の近くで薄くしながら描く間に使う
	// 深度を書く物（当たり判定の表示など）を描く前に resumeScene へ戻すこと
	void resumeSceneReadOnlyDepth();
	void bindSceneColor();										// 描画先をシーンの色だけにする（深度は外す）。深度をシェーダから読みながらシーンの色へ描くとき（ディファードのライティング）に呼ぶ
	GameLib::Graphics::DepthTarget sceneDepth() const;			// シーンの深度。G-Buffer へ描くときに一緒に使い、ライティングと G-Buffer の中身を見るときに読む
	const GameLib::Graphics::RenderTarget& sceneColor() const;	// シーンの色（HDR・リニア）。照らした後の色を読むパス（映り込み。ScreenSpaceReflection）が、描画先から外した後に読む
	void setDamageFlash( float amount );						// 攻撃された瞬間に、画面の周りを赤くする強さ（0 で無し、1 で一番赤い）。次の endScene から効く
	void setPain( float amount );								// 攻撃された瞬間の痛みの強さ（0 で無し、1 で一番強い）。画面の周りをぼかし、ビネットを濃くする。次の endScene から効く
	void setFade( float amount );								// 画面全体を暗くする量（0 でそのまま、1 で真っ黒）。死んだ後に暗転するのに使う。次の endScene から効く
	void setBrightnessExponent( float exponent );				// 設定画面の明るさ（GameSettings::brightnessExponent）。グレーディングの後の色に掛けるガンマの指数。次の endScene から効く
	void setNoiseFrame( int frame );							// ディザのノイズのフレームの番号（TemporalJitter::frameIndex。TAA でないときは 0）。次の endScene から効く
	// 中間の描画先を画面へ写す。シーンを描き終わった後に呼ぶ。描画先は画面に戻る
	// GPU の印を中で打つ：「PostProcess」（ブルームと写すまで）・「FXAA」（FXAA。OFF なら区間はほぼ 0）
	void endScene();
	// 写すシェーダ → FXAA → ブルーム → 色の表を焼くシェーダの順に、最初に見つかったコンパイルエラー（複数行のことがある）。問題が無ければ空文字列
	// エラーの間も直前に成功したシェーダで写し続ける（一度も成功していなければ画面に何も写らない）
	const char* errorMessage() const;

private:
	// 写すシェーダへ渡す値（ビネット・ブルーム・赤み・痛み・暗さ・グレーディング・色の表の強さ）を定数バッファへ書く（endScene のたび）
	// gradingOn：false ならグレーディングに「何も変えない値」を入れる（F6 の Color grading を切った間）。表を焼くときは今の値を入れるため true
	void updateConstants( bool gradingOn );
	void updateColorLut();	// F6 の色の表のボタン（colorLutControl）の要求を行い、表の読み込みを進める（endScene の始め）
	void drawFxaa();		// 8bit の中間（mFxaaInput）を読み、FXAA を掛けて画面へ写す（endScene の最後。FXAA が ON のときだけ）

	GameLib::Graphics::RenderTarget mSceneColor;   // シーンの色（1を超える明るさを持てる形式。後でトーンマップするため）
	GameLib::Graphics::DepthTarget mSceneDepth;    // シーンの深度（後で位置の復元・霧などに読む）
	Bloom mBloom;                                  // シーンの明るい所をにじませた絵を作る
	ColorLut mColorLut;                            // 色の表（LUT）。読んでいれば写すときに t4 に付ける
	GameLib::Graphics::Shader mPresentShader;
	GameLib::Graphics::ConstantBuffer mPresentConstants;
	GameLib::Graphics::RenderTarget mFxaaInput;  // 写した結果（画面の色・8bit。a に輝度）。FXAA が ON のときだけ通す
	GameLib::Graphics::Shader mFxaaShader;
	GameLib::Graphics::ConstantBuffer mFxaaConstants;
	float mDamageFlash; // setDamageFlash の値
	float mPain;        // setPain の値
	float mFade;        // setFade の値
	float mBrightnessExponent;	// setBrightnessExponent の値
	int mNoiseFrame;    // setNoiseFrame の値
};

#endif
