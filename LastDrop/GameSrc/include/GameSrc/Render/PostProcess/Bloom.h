#ifndef INCLUDED_BLOOM_H
#define INCLUDED_BLOOM_H

#include "GameLib/Graphics/RenderTarget.h"
#include "GameLib/Graphics/Shader.h"
#include "GameLib/Graphics/ConstantBuffer.h"

// 明るい所をにじませる（ブルーム）ための絵を作る。仕様は SummaryPlan/個別仕様/グラフィック/Bloom_Plan.txt
// シーンの色から明るさ 1 を超えた分だけを取り出し、1/2 → 1/4 → 1/8 → 1/16 と縮めながらぼかし、
// 1/16 → 1/2 へ広げながら足し戻す。できた絵をシーンに足すのは呼んだ側（PostProcess / Present.hlsl）
// 1 を超える明るさを持てる描画先（RGBA16F）に描いたシーンでないと、何も取り出せない
// 作るのは Graphics::Manager ができた後（PostProcess と同じ）
class Bloom{
public:
	Bloom();
	Bloom( const Bloom& ) = delete;
	Bloom& operator=( const Bloom& ) = delete;

	// sceneColor からブルームの絵（画面の 1/2 の大きさ）を作って返す。返した描画先は次に apply を呼ぶまで有効
	// 描画先・シェーダ・ブレンドは切り替わったまま戻さない（呼んだ側が次に描く物の設定をする）
	const GameLib::Graphics::RenderTarget& apply( const GameLib::Graphics::RenderTarget& sceneColor );
	// 3 本のシェーダのうち、最初に見つかったコンパイルエラー。問題が無ければ空文字列
	const char* errorMessage() const;

private:
	static const int kLevelCount = 4; // 段の数（1/2・1/4・1/8・1/16）

	// 縮めた絵の段。[0] が 1/2 で、番号が 1 増えるごとに半分。縮めるときに書き、足し戻すときに加算して使い回す
	GameLib::Graphics::RenderTarget mLevels[ kLevelCount ];
	GameLib::Graphics::Shader mExtractShader; // 明るい所の取り出し（シーン → [0]）
	GameLib::Graphics::Shader mDownShader;    // 縮小（[i - 1] → [i]）
	GameLib::Graphics::Shader mUpShader;      // 足し戻し（[i] → [i - 1] へ加算）
	GameLib::Graphics::ConstantBuffer mConstants;
};

#endif
