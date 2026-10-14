#ifndef INCLUDED_POST_PROCESS_H
#define INCLUDED_POST_PROCESS_H

#include "GameLib/Graphics/RenderTarget.h"
#include "GameLib/Graphics/DepthTarget.h"
#include "GameLib/Graphics/Shader.h"
#include "GameLib/Graphics/ConstantBuffer.h"

// 画面全体に掛ける処理（ポストプロセス）の入口。
// beginScene と endScene の間に普段どおり描くと、画面ではなく中間の描画先（色と深度）へ描かれ、
// endScene でシェーダ（Resource/Shader/Present.hlsl）を通して画面へ写す。
// 今は描画基盤の動作確認として、周辺を少し暗くする（ビネット）だけ。トーンマップ・カラーグレーディング等はここに足す。
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
	// 中間の描画先を画面へ写す。シーンを描き終わった後に呼ぶ。描画先は画面に戻る。
	void endScene();
	// 写すシェーダのコンパイルエラー（複数行のことがある）。問題が無ければ空文字列。
	// エラーの間も直前に成功したシェーダで写し続ける（一度も成功していなければ画面に何も写らない）。
	const char* errorMessage() const;

private:
	GameLib::Graphics::RenderTarget mSceneColor;   // シーンの色（1を超える明るさを持てる形式。後でトーンマップするため）
	GameLib::Graphics::DepthTarget mSceneDepth;    // シーンの深度（後で位置の復元・霧などに読む）
	GameLib::Graphics::Shader mPresentShader;
	GameLib::Graphics::ConstantBuffer mPresentConstants;
};

#endif
