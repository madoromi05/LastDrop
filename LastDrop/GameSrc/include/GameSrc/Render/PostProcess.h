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
// 今は明るい所のにじみ（ブルーム。Bloom）を足し、周辺を少し暗くする（ビネット）。
// ビネットの所に赤を足す強さ・周りのぼかしと暗さ（噛まれた瞬間）と、全体を暗くする量（死んだ後）を外から動かせる。
// 値はこのシーンの PostProcess だけが持つので、シーンを作り直せば 0 に戻る（次のシーンへ漏れない）。トーンマップ・カラーグレーディング等はここに足す。
//
// 写した後の画面には、文字（DebugScreen）など Framework が後から描く物がそのまま重なる（ポストプロセスは掛からない）。
// 作るのは Graphics::Manager ができた後（シーンの中）。
class PostProcess{
public:
	PostProcess();
	PostProcess( const PostProcess& ) = delete;
	PostProcess& operator=( const PostProcess& ) = delete;

	// 描画先を中間の描画先へ切り替え、色と深度を消す。シーンを描く前に呼ぶ。
	void beginScene();
	// 噛まれた瞬間に、画面の周りを赤くする強さ（0 で無し、1 で一番赤い）。次の endScene から効く。
	void setDamageFlash( float amount );
	// 噛まれた瞬間の痛みの強さ（0 で無し、1 で一番強い）。画面の周りをぼかし、ビネットを濃くする。次の endScene から効く。
	void setPain( float amount );
	// 画面全体を暗くする量（0 でそのまま、1 で真っ黒）。死んだ後に暗転するのに使う。次の endScene から効く。
	void setFade( float amount );
	// 中間の描画先を画面へ写す。シーンを描き終わった後に呼ぶ。描画先は画面に戻る。
	void endScene();
	// 写すシェーダ（先）とブルームのシェーダ（後）のうち、最初に見つかったコンパイルエラー（複数行のことがある）。問題が無ければ空文字列。
	// エラーの間も直前に成功したシェーダで写し続ける（一度も成功していなければ画面に何も写らない）。
	const char* errorMessage() const;

private:
	// 写すシェーダへ渡す値（ビネット・ブルーム・赤み・痛み・暗さ）を定数バッファへ書く（endScene のたび）。
	void updateConstants();

	GameLib::Graphics::RenderTarget mSceneColor;   // シーンの色（1を超える明るさを持てる形式。後でトーンマップするため）
	GameLib::Graphics::DepthTarget mSceneDepth;    // シーンの深度（後で位置の復元・霧などに読む）
	Bloom mBloom;                                  // シーンの明るい所をにじませた絵を作る
	GameLib::Graphics::Shader mPresentShader;
	GameLib::Graphics::ConstantBuffer mPresentConstants;
	float mDamageFlash; // setDamageFlash の値
	float mPain;        // setPain の値
	float mFade;        // setFade の値
};

#endif
