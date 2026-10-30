#ifndef INCLUDED_POST_PROCESS_H
#define INCLUDED_POST_PROCESS_H

#include "GameLib/Graphics/RenderTarget.h"
#include "GameLib/Graphics/DepthTarget.h"
#include "GameLib/Graphics/Shader.h"
#include "GameLib/Graphics/ConstantBuffer.h"
#include "GameSrc/Render/Bloom.h"

// 画面全体に掛ける処理（ポストプロセス）の入口。
// beginScene と endScene の間に普段どおり描くと、画面ではなく中間の描画先（色と深度）へ描かれ、
// endScene でシェーダ（Resource/Shader/Present.hlsl）を通して画面へ写す。
// 中間の描画先はリニア（光の量に比例した数）で持ち、beginScene〜endScene の間は Graphics::Manager::enableLinearColor を入れておく。
// 写すときに、明るい所のにじみ（ブルーム。Bloom）を足し、露出を掛けて ACES で 0〜1 に収め（トーンマップ）、
// 周辺を少し暗くして（ビネット）、最後に画面の色（sRGB）へ戻す（LinearToneMap_Plan.txt 6 章）。露出とブルームの値は PostProcessParams。
// ビネットの所に赤を足す強さ・周りのぼかしと暗さ（噛まれた瞬間）と、全体を暗くする量（死んだ後）を外から動かせる。
// 値はこのシーンの PostProcess だけが持つので、シーンを作り直せば 0 に戻る（次のシーンへ漏れない）。
// トーンマップの後にカラーグレーディング、画面の色へ戻した後にディザを掛ける（NightFinish_Plan.txt 6・7 章）。
// FXAA が ON（fxaaEnabled）のときは、写した結果をいったん 8bit の中間（a に輝度）へ置き、Resource/Shader/Fxaa.hlsl で輪郭を
// なめらかにしながら画面へ写す（NightFinish_Plan.txt 8 章）。OFF のときは今までどおり画面へ直接写す。
//
// 写した後の画面には、文字（DebugScreen）など Framework が後から描く物がそのまま重なる（ポストプロセスは掛からない）。
// 作るのは Graphics::Manager ができた後（シーンの中）。
class PostProcess{
public:
	PostProcess();
	PostProcess( const PostProcess& ) = delete;
	PostProcess& operator=( const PostProcess& ) = delete;

	// 描画先を中間の描画先へ切り替え、色と深度を消す。シーンを描く前に呼ぶ。
	// ここから endScene までは、材質・ライト・頂点の色がリニアへ直される（Graphics::Manager::enableLinearColor）。
	void beginScene();
	// 描画先を中間の描画先（色と深度）へ付け直す（消さない）。beginScene の後に別の描画先（G-Buffer）へ描いたとき、
	// 続きをシーンへ描く前に呼ぶ。
	void resumeScene();
	// resumeScene と同じだが、深度は奥行きの判定だけに使い（書き込まない）、同じ深度を t5 から読めるようにも付ける。
	// 半透明の板（LitQuadBatch）を、物の近くで薄くしながら描く間に使う（SsaoTranslucent_Plan.txt 7 章）。
	// 深度を書く物（当たり判定の表示など）を描く前に resumeScene へ戻すこと。
	void resumeSceneReadOnlyDepth();
	// 描画先をシーンの色だけにする（深度は外す）。深度をシェーダから読みながらシーンの色へ描くとき（ディファードのライティング）に呼ぶ。
	void bindSceneColor();
	// シーンの深度。G-Buffer へ描くときに一緒に使い、ライティングと G-Buffer の中身を見るときに読む。
	GameLib::Graphics::DepthTarget sceneDepth() const;
	// 噛まれた瞬間に、画面の周りを赤くする強さ（0 で無し、1 で一番赤い）。次の endScene から効く。
	void setDamageFlash( float amount );
	// 噛まれた瞬間の痛みの強さ（0 で無し、1 で一番強い）。画面の周りをぼかし、ビネットを濃くする。次の endScene から効く。
	void setPain( float amount );
	// 画面全体を暗くする量（0 でそのまま、1 で真っ黒）。死んだ後に暗転するのに使う。次の endScene から効く。
	void setFade( float amount );
	// 中間の描画先を画面へ写す。シーンを描き終わった後に呼ぶ。描画先は画面に戻る。
	// GPU の印を中で打つ：「PostProcess」（ブルームと写すまで）・「FXAA」（FXAA。OFF なら区間はほぼ 0）。
	void endScene();
	// 写すシェーダ → FXAA → ブルームのシェーダの順に、最初に見つかったコンパイルエラー（複数行のことがある）。問題が無ければ空文字列。
	// エラーの間も直前に成功したシェーダで写し続ける（一度も成功していなければ画面に何も写らない）。
	const char* errorMessage() const;

private:
	// 写すシェーダへ渡す値（ビネット・ブルーム・赤み・痛み・暗さ）を定数バッファへ書く（endScene のたび）。
	void updateConstants();
	// 8bit の中間（mFxaaInput）を読み、FXAA を掛けて画面へ写す（endScene の最後。FXAA が ON のときだけ）。
	void drawFxaa();

	GameLib::Graphics::RenderTarget mSceneColor;   // シーンの色（1を超える明るさを持てる形式。後でトーンマップするため）
	GameLib::Graphics::DepthTarget mSceneDepth;    // シーンの深度（後で位置の復元・霧などに読む）
	Bloom mBloom;                                  // シーンの明るい所をにじませた絵を作る
	GameLib::Graphics::Shader mPresentShader;
	GameLib::Graphics::ConstantBuffer mPresentConstants;
	GameLib::Graphics::RenderTarget mFxaaInput;  // 写した結果（画面の色・8bit。a に輝度）。FXAA が ON のときだけ通す
	GameLib::Graphics::Shader mFxaaShader;
	GameLib::Graphics::ConstantBuffer mFxaaConstants;
	float mDamageFlash; // setDamageFlash の値
	float mPain;        // setPain の値
	float mFade;        // setFade の値
};

#endif
