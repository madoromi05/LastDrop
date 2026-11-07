#include "Generators.h"

using namespace Synth;

// 倒れたゾンビの体が地面に着く音（HitFeel_Plan.txt Q5）。
// 「体が着く低い一打」「地面の鈍さ」「服の擦れ」と、少し遅れて「腕が地面を打つ」小さい一打でできている。
// 数値は初期値。ゲームの中で聞いて直す。
namespace{
	const float kLength = 0.6f;          // 音の長さ（秒）
	const float kNoFadeIn = 0.f;
	const float kFadeOut = 0.05f;        // お尻を切るときのフェード（秒）
	const float kPeak = 0.8f;            // 正規化の大きさ
	const float kTimingJitter = 0.01f;   // 別テイクごとに、腕の一打の始まりをずらす（秒）

	// 体が着く：低い正弦波の一打（重さ）
	const float kBodyHz = 70.f;
	const float kBodyDecay = 0.12f;
	const float kBodyGain = 1.f;
	// 地面の鈍さ：低音だけのノイズ
	const float kGroundAttack = 0.003f;
	const float kGroundDecay = 0.15f;
	const float kGroundCutoffHz = 200.f;
	const float kGroundGain = 0.7f;
	// 服の擦れ：中くらいの高さの短いノイズ
	const float kClothAttack = 0.002f;
	const float kClothDecay = 0.06f;
	const float kClothHz = 500.f;
	const float kClothQ = 0.7f;
	const float kClothGain = 0.4f;
	// 腕が地面を打つ：体より少し後・少し高く・小さく
	const float kArmDelay = 0.12f;
	const float kArmHz = 90.f;
	const float kArmDecay = 0.06f;
	const float kArmGain = 0.4f;
}

Buffer makeZombieBodyFall( Random* random ) {
	Buffer buffer = makeBuffer( kLength );
	addThump( &buffer, 0.f, kBodyHz, kBodyDecay, kBodyGain );
	addBoom( &buffer, 0.f, kGroundAttack, kGroundDecay, kGroundCutoffHz, kGroundGain, random );
	addNoiseHit( &buffer, 0.f, kClothAttack, kClothDecay, kClothHz, kClothQ, kClothGain, random );
	addThump( &buffer, kArmDelay + random->range( 0.f, kTimingJitter ), kArmHz, kArmDecay, kArmGain );
	fadeEdges( &buffer, kNoFadeIn, kFadeOut );
	normalize( &buffer, kPeak );
	return buffer;
}
