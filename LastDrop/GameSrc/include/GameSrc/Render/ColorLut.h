#ifndef INCLUDED_COLOR_LUT_H
#define INCLUDED_COLOR_LUT_H

#include "GameLib/Graphics/RenderTarget.h"
#include "GameLib/Graphics/Shader.h"
#include "GameLib/Graphics/ConstantBuffer.h"
#include "GameLib/Graphics/Texture.h"

// 色の表（LUT。1024×32 の帯）の読み込みと書き出し（SummaryPlan/個別仕様/MoonStarsLut_Plan.txt 6・7 章）。
//   読む：Resource/UI/lut.tga（外の道具で直した表）。起動時と reload のときに、あれば読む。無ければ「読んでいない」（今と同じ絵）
//   書く：今の色の調整（Grading.hlsli の gradeColor）を表に焼き、Resource/UI/lut_export.tga に書く（Resource/Shader/LutBake.hlsl）
// 表を引くのは Present.hlsl（PostProcess が texture を t4 に付ける）。作るのは Graphics::Manager ができた後（シーンの中）。
class ColorLut{
public:
	static const int kSize = 32; // 1 辺の段の数（Grading.hlsli の kLutSize と揃える）

	ColorLut();
	ColorLut( const ColorLut& ) = delete;
	ColorLut& operator=( const ColorLut& ) = delete;

	// lut.tga を読み直す（外で直した後、起動したまま見比べる）。無ければ「読んでいない」に戻す。
	void reload();
	// 読み込みが終わったかを見て、使えるか（大きさが 1024×32 か）を決める。描く前に毎フレーム呼ぶ。
	void update();
	// 表を引ける（読み終わり、大きさが合っている）。
	bool isLoaded() const;
	// 読んだ表（isLoaded のときだけ使う）。
	const GameLib::Graphics::Texture& texture() const;
	// 今の色の調整を表に焼いて lut_export.tga に書く。presentConstants：Present.hlsl と同じ定数（グレーディングの値を入れた物）。
	// 描画先を切り替える（終わった後の描画先は呼んだ側で付け直す）。GPU が描き終わるのを待つので、ボタンを押したときだけ呼ぶ。
	void exportBaked( const GameLib::Graphics::ConstantBuffer& presentConstants );
	// LutBake.hlsl のコンパイルエラー。問題が無ければ空文字列。
	const char* errorMessage() const;

private:
	GameLib::Graphics::Texture mTexture;     // 読んだ lut.tga（空なら読んでいない）
	bool mLoaded;                            // update が「使える」と決めた
	GameLib::Graphics::RenderTarget mBaked;  // 焼く先（1024×32・RGBA8。中身は画面の色の数）
	GameLib::Graphics::Shader mBakeShader;
};

#endif
