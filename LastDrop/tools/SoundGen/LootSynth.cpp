#include "Generators.h"

using namespace Synth;

// 漁る音（段階R3。SummaryPlan/個別仕様/出撃と帰還/R3_Plan.txt Q7）
// 開けている間の音はループで鳴らす（開ける秒は場所ごとに違い、取りやめもあるので、長さの決まった 1 回の音にしない）
// ループの中身は「探る手の擦れ（ノイズの上がり下がり）」を何度か並べ、場所ごとに当たる物の音（金属のカチャ・木のコツ・布だけ）を混ぜる
namespace{
	const float kLootDrive = 1.3f; // 仕上げの潰し具合（FoleySynth と同じ。擦れのピークを丸める）

	// ── ループ（開けている間）──
	const float kLoopLength = 2.2f;      // 畳む前の長さ（継ぎ目を畳む kLoopCrossfade の分だけ短くなる）
	const float kLoopCrossfade = 0.2f;
	const int kRummageCount = 7;         // 探る手の「ガサッ」の数
	const float kRummageMinDuration = 0.12f;
	const float kRummageMaxDuration = 0.3f;
	const float kRummageQ = 0.8f;
	const float kRummageGain = 0.5f;
	struct RummageBand{ float lowHz, highHz; };
	const RummageBand kMetalRummage = { 900.f, 2600.f };  // 中の物をかき分ける（紙・袋・布が混ざる）
	const RummageBand kWoodRummage = { 700.f, 2000.f };
	const RummageBand kClothRummage = { 1200.f, 3500.f }; // 服のポケットを探る（服の擦れと同じ帯域）
	// 金属：缶や工具が当たる「カチャ」
	const int kMetalClinkCount = 5;
	const Mode kMetalClinkModes[] = { { 1850.f, 0.05f, 1.0f }, { 3100.f, 0.03f, 0.6f }, { 4700.f, 0.02f, 0.4f } };
	const int kMetalClinkModeCount = sizeof( kMetalClinkModes ) / sizeof( kMetalClinkModes[ 0 ] );
	const float kMetalClinkGain = 0.25f;
	// 木：板に物が当たる「コツ」（低い一打と、短い木の響き）
	const int kWoodKnockCount = 4;
	const float kWoodKnockHz = 220.f;
	const float kWoodKnockDecay = 0.04f;
	const float kWoodKnockGain = 0.5f;
	const float kWoodTapHz = 900.f;
	const float kWoodTapQ = 2.f;
	const float kWoodTapGain = 0.35f;
	// 布：シワが伸びる細かい粒（服の擦れと同じ）
	const float kCrinkleHz = 3000.f;
	const float kCrinkleQ = 1.5f;
	const float kCrinkleDecay = 0.006f;
	const float kCrinkleGain = 0.15f;
	const int kCrinkleCount = 30;
	const float kLoopPeak = 0.5f;
	const float kWoodTapAttack = 0.001f;

	// ループの長さの中に、探る手の擦れを散らす
	void addRummage( Buffer* buffer, const RummageBand& band, Random* random ) {
		for ( int i = 0; i < kRummageCount; ++i ) {
			float duration = random->range( kRummageMinDuration, kRummageMaxDuration );
			float start = random->range( 0.f, kLoopLength - duration );
			float from = random->range( band.lowHz, band.highHz );
			float to = random->range( band.lowHz, band.highHz );
			addNoiseSweep( buffer, start, duration, from, to, kRummageQ, kRummageGain, random );
		}
	}
}

Buffer makeLootSearch( SoundId id, Random* random ) {
	Buffer buffer = makeBuffer( kLoopLength );
	switch ( id ) {
	case SOUND_LOOT_SEARCH_METAL:
		addRummage( &buffer, kMetalRummage, random );
		for ( int i = 0; i < kMetalClinkCount; ++i ) {
			addModes( &buffer, random->range( 0.f, kLoopLength ), kMetalClinkModes, kMetalClinkModeCount, kMetalClinkGain );
		}
		break;
	case SOUND_LOOT_SEARCH_WOOD:
		addRummage( &buffer, kWoodRummage, random );
		for ( int i = 0; i < kWoodKnockCount; ++i ) {
			float start = random->range( 0.f, kLoopLength );
			addThump( &buffer, start, kWoodKnockHz, kWoodKnockDecay, kWoodKnockGain );
			addNoiseHit( &buffer, start, kWoodTapAttack, kWoodKnockDecay, kWoodTapHz, kWoodTapQ, kWoodTapGain, random );
		}
		break;
	case SOUND_LOOT_SEARCH_CLOTH:
		addRummage( &buffer, kClothRummage, random );
		addGrains( &buffer, 0.f, kLoopLength, kCrinkleCount, kCrinkleHz, kCrinkleQ, kCrinkleDecay, kCrinkleGain, random );
		break;
	default:
		return Buffer(); // 漁る音ではない（main で知らせる）
	}
	saturate( &buffer, kLootDrive );
	makeLoopSeamless( &buffer, kLoopCrossfade ); // ループなので頭とお尻は畳む（fadeEdges はしない）
	normalize( &buffer, kLoopPeak );
	return buffer;
}

