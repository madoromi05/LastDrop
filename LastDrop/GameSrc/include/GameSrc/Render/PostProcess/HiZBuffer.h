#ifndef INCLUDED_HI_Z_BUFFER_H
#define INCLUDED_HI_Z_BUFFER_H

#include "GameLib/Graphics/RenderTarget.h"
#include "GameLib/Graphics/DepthTarget.h"
#include "GameLib/Graphics/Shader.h"
#include "GameLib/Graphics/ConstantBuffer.h"

// Hi-Z：シーンの深度を段ごとに半分へ縮めた物。どの段の 1 画素も「重なる所の一番手前の深度」を持つ（Uludag 2014）
// 映り込み（Ssr.hlsl）が、何も無い所を粗い段で大きく飛ばして進むのに使う。シェーダは Resource/Shader/HiZBuild.hlsl
// 描画先は画面の半分の大きさ（SSR と同じ）・R32F・1 画素までの全部の段（1920×1080 なら 960×540 から 10 段）
// 作るのは Graphics::Manager ができた後（シーンの中）。持ち主は ScreenSpaceReflection
class HiZBuffer{
public:
	HiZBuffer();
	HiZBuffer( const HiZBuffer& ) = delete;
	HiZBuffer& operator=( const HiZBuffer& ) = delete;

	void draw( GameLib::Graphics::DepthTarget depth );		// depth（G-Buffer と一緒に描いた深度）から全部の段を作る。描画先を自分の描画先へ切り替える（終わった後の描画先は呼んだ側で付け直す）
	const GameLib::Graphics::RenderTarget& result() const;	// 作った段（全部の段を読む。シェーダは Load( int3( 画素, 段 ) ) で段を選ぶ）
	int mipLevels() const;									// 段の数
	const char* errorMessage() const;						// シェーダのコンパイルエラー。問題が無ければ空文字列

private:
	void setTargetSize( int mip );	// 段 mip の大きさを定数に書いて付ける（シェーダが、描く先の 1 画素に重なる元の画素の範囲を出すのに使う）

	GameLib::Graphics::RenderTarget mHiZ;       // 半分の大きさ・全部の段
	GameLib::Graphics::Shader mShader;          // Resource/Shader/HiZBuild.hlsl
	GameLib::Graphics::ConstantBuffer mConstants; // 描く先の段の大きさ（段ごとに書き換える）
};

// Hi-Z で進むか（F6 → Render の Hi-Z）。false の間は Hi-Z を作らず、映り込みは直線の進み方
// 起動中ずっと 1 つ（HiZBuffer.cpp が持つ。初期値 true。Release では true のまま）
bool& hiZEnabled();
// G-Buffer の表示の Hi-Z で見る段（F6 → Render の Hi-Z view level。0 が一番細かい。段の数を超えたら一番粗い段）
// 起動中ずっと 1 つ（HiZBuffer.cpp が持つ。初期値 0）
int& hiZViewLevel();

#endif
