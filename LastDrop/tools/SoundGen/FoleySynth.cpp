#include "Generators.h"

using namespace Synth;

// 装備・動作音。どれも「布が擦れる『シュッ』」だけでできている（帯域を動かすノイズ addNoiseSweep と、細かい粒 addGrains）
// 金具が当たる金属音は、出し入れで「ゴン」と目立ったので作らない。装備の揺れ（gear_weapon）は録音素材を使う
namespace{
	const float kFadeOut = 0.03f;
	// 仕上げの潰し具合（Synth::saturate）。擦れのピークを少し丸めて、耳に刺さらないようにする
	// 強くすると歪んでざらつく（1.8 では強すぎた）
	const float kFoleyDrive = 1.3f;

	struct SwishRecipe{ float start, duration, fromHz, toHz, q, gain; };	// 布の擦れ「シュッ」

	// ── 服の擦れ（一歩ごと）──
	const float kRustleLength = 0.35f;
	const float kRustleMinHz = 1500.f;	// 擦れの帯域の下限と上限。揺らぎごとにこの間で動かし方を変える
	const float kRustleMaxHz = 3500.f;
	const float kRustleQ = 0.7f;
	const float kRustleGain = 0.6f;
	const float kRustleDuration = 0.28f;
	// 生地のシワが伸びる「パリッ」という細かい粒
	const int kCrinkleCount = 15;
	const float kCrinkleHz = 3000.f;
	const float kCrinkleQ = 1.5f;
	const float kCrinkleDecay = 0.006f;
	const float kCrinkleGain = 0.2f;
	const float kRustlePeak = 0.5f;

	// ── 武器を出す・しまう ──
	// どちらも服の「シュッ」だけ。金具の「ガチャ」と重さの低音は入れない
	// （低音がゲーム側で音程を下げられて「ゴン」と目立ったため。覗く音と同じやり方にそろえる）
	// 長さは長い方のシュッ（しまう）が入りきる長さ
	const float kWeaponLength = 0.45f;
	// 銃を振る「シュッ」は厚手のスリングと袖が擦れる音。帯域を 1.5kHz までに抑える（高いと軽く、甲高く聞こえる）
	// 出す: スリングから引き出す「シュッ」（上がる）
	const SwishRecipe kDrawSwish = { 0.f, 0.3f, 500.f, 1500.f, 0.8f, 0.6f };
	// しまう: 背中へ回す「シュッ」（下がる）
	const SwishRecipe kHolsterSwish = { 0.f, 0.4f, 1500.f, 500.f, 0.8f, 0.6f };
	const float kWeaponPeak = 0.7f;

	// ── 覗く・覗くのをやめる ──
	const float kAimLength = 0.25f;
	// 覗く: 腕を上げて頬付けする服の「シュッ」だけ。金具の「カチャ」は入れない
	// （覗くたびに金属音が鳴るとうるさく、覗くのは静かな動作なので）
	const SwishRecipe kAimInSwish = { 0.f, 0.18f, 600.f, 1200.f, 0.9f, 0.25f };
	// 覗くのをやめる: 腕を下ろす服の「シュッ」だけ（覗くと逆向きに、高い方から低い方へ）。重い「ドン」は入れない
	const SwishRecipe kAimOutSwish = { 0.f, 0.18f, 1200.f, 600.f, 0.9f, 0.5f };
	const float kAimInPeak = 0.5f;
	const float kAimOutPeak = 0.4f;

	// ── 銃の懐中電灯のスイッチ ──
	// 押しボタンのスイッチ「カチッ」。押し込む音と、指を離して戻る音の 2 つ（戻る方が小さく少し高い）
	// どちらも高い帯域の短いノイズ（樹脂がはじける）と、小さな樹脂の筐体の短い響きでできている
	// 金属の響き（着弾音の kMetalModes）より高く短く、空撃ちの「カチッ」（録音）より軽い音にする
	const float kClickLength = 0.12f;
	const float kReleaseDelay = 0.045f;		// 押してから戻るまで（秒）。揺らぎごとに少し変える
	const float kReleaseDelayJitter = 0.01f;
	struct ClickRecipe{ float centerHz, q, attack, decay, gain, modeGain; };
	const ClickRecipe kPressClick = { 4500.f, 1.2f, 0.0005f, 0.004f, 1.0f, 0.35f };
	const ClickRecipe kReleaseClick = { 5500.f, 1.2f, 0.0005f, 0.003f, 0.55f, 0.2f };
	const Mode kSwitchModes[] = { { 2300.f, 0.012f, 1.0f }, { 3700.f, 0.008f, 0.6f } };
	const int kSwitchModeCount = sizeof( kSwitchModes ) / sizeof( kSwitchModes[ 0 ] );
	const float kClickPeak = 0.6f;

	void addClick( Buffer* buffer, float start, const ClickRecipe& recipe, Random* random ) {
		addNoiseHit( buffer, start, recipe.attack, recipe.decay, recipe.centerHz, recipe.q, recipe.gain, random );
		addModes( buffer, start, kSwitchModes, kSwitchModeCount, recipe.modeGain );
	}

	void addSwish( Buffer* buffer, const SwishRecipe& recipe, Random* random ) {
		addNoiseSweep( buffer, recipe.start, recipe.duration, recipe.fromHz, recipe.toHz, recipe.q, recipe.gain, random );
	}

	Buffer finish( Buffer buffer, float peak ) {
		fadeEdges( &buffer, 0.f, kFadeOut );
		saturate( &buffer, kFoleyDrive );
		normalize( &buffer, peak );
		return buffer;
	}
}

Buffer makeClothRustle( Random* random ) {
	Buffer buffer = makeBuffer( kRustleLength );
	// 足を出すたびに生地が引っぱられて戻る。行きと帰りで擦れの高さが逆に動く
	float from = random->range( kRustleMinHz, kRustleMaxHz );
	float to = random->range( kRustleMinHz, kRustleMaxHz );
	addNoiseSweep( &buffer, 0.f, kRustleDuration, from, to, kRustleQ, kRustleGain, random );
	addGrains( &buffer, 0.f, kRustleDuration, kCrinkleCount, kCrinkleHz, kCrinkleQ, kCrinkleDecay, kCrinkleGain, random );
	return finish( buffer, kRustlePeak );
}

Buffer makeWeaponDraw( Random* random ) {
	Buffer buffer = makeBuffer( kWeaponLength );
	addSwish( &buffer, kDrawSwish, random );
	return finish( buffer, kWeaponPeak );
}

Buffer makeWeaponHolster( Random* random ) {
	Buffer buffer = makeBuffer( kWeaponLength );
	addSwish( &buffer, kHolsterSwish, random );
	return finish( buffer, kWeaponPeak );
}

Buffer makeAimIn( Random* random ) {
	Buffer buffer = makeBuffer( kAimLength );
	addSwish( &buffer, kAimInSwish, random );
	return finish( buffer, kAimInPeak );
}

Buffer makeAimOut( Random* random ) {
	Buffer buffer = makeBuffer( kAimLength );
	addSwish( &buffer, kAimOutSwish, random );
	return finish( buffer, kAimOutPeak );
}

Buffer makeFlashlightClick( Random* random ) {
	Buffer buffer = makeBuffer( kClickLength );
	addClick( &buffer, 0.f, kPressClick, random );
	float release = kReleaseDelay + random->range( -kReleaseDelayJitter, kReleaseDelayJitter );
	addClick( &buffer, release, kReleaseClick, random );
	return finish( buffer, kClickPeak );
}
