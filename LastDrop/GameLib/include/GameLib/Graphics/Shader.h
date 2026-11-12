#ifndef INCLUDED_GAMELIB_GRAPHICS_SHADER_H
#define INCLUDED_GAMELIB_GRAPHICS_SHADER_H

#include "GameLib/Graphics/VertexFormat.h"

namespace GameLib{
namespace Graphics{

///HLSL ファイルから作る頂点シェーダとピクセルシェーダの組。Manager::setShader で使う
///ファイルには VSMain（vs_5_0）と PSMain（ps_5_0）を書く。#include "xxx.hlsli" はそのファイルと同じフォルダから探す
///
///Debug ビルドでは、ファイル（#include したものも含む）が書き換わると、次に setShader したときに自動で作り直す
///コンパイルに失敗したら前のシェーダを使い続け、errorMessage() に理由を残す（止めない）
///一度もコンパイルに成功していないシェーダを setShader すると、固定パイプラインで描かれる
///
///定数バッファ b0 は固定パイプラインと同じ中身（行列・ライト）が入る（Resource/Shader/Common.hlsli 参照）
///自分の定数は b1 から、自分のテクスチャは t2 から、サンプラは s0（繰り返し）/s1（端で止める）/s2（影の比較）を使う
class Shader{
public:
	static Shader create( const char* filename, const VertexFormat& format = VertexFormat::standard() );	///filename: HLSL ファイル。format: 頂点の形（全画面描画にしか使わないなら何でもよい）
	bool isError() const;																					///使えるシェーダが無い（一度もコンパイルに成功していない）
	const char* errorMessage() const;																		///最後のコンパイルのエラー文。成功していれば空文字列
	///今すぐファイルを読み直して作り直す。成功すれば true
	bool reload();
	const char* filename() const;

	//以下ユーザは意識しない関数群
	Shader();
	Shader( const Shader& );
	~Shader();
	operator void*() const;
	Shader& operator=( const Shader& );
	void release();
	bool operator==( const Shader& ) const;
	bool operator!=( const Shader& ) const;

	class Impl;
	Shader( const Impl* );
private:
	friend class Manager;
	Impl* mImpl;
};

} //namespace Graphics
} //namespace GameLib

#endif
