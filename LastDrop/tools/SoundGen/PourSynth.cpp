#include "Generators.h"

#include <cmath>

using namespace Synth;

// 給油している間の音（FuelEscapeLoading_Plan.txt 改訂 2）。缶を傾けてガソリンを給油口へ注ぐ音のループ。
// 「流れ落ちる水の音（帯域を絞ったノイズ。中心をゆっくり揺らす）」＋「缶へ空気が戻るゴボッ（低く上がる音と鈍い一打）」
// ＋「タンクの中で泡がはじける小さなポコッ」を重ねる。給油の秒は RaidParams で変わり、取りやめもあるのでループにする。
namespace{
	const float kLoopLength = 2.4f;      // 畳む前の長さ（継ぎ目を畳む kLoopCrossfade の分だけ短くなる）
	const float kLoopCrossfade = 0.2f;
	const float kPourDrive = 1.2f;       // 仕上げの潰し具合（ゴボッの頭を丸める）
	const float kLoopPeak = 0.5f;        // 漁る音と同じ大きさにそろえる

	// ── 流れ落ちる音（ずっと鳴っている）──
	const float kStreamLowHz = 700.f;    // 流れの中心が揺れる幅（水が細くなる・太くなる）
	const float kStreamHighHz = 1600.f;
	const float kStreamQ = 0.9f;
	const float kStreamGain = 0.6f;
	const float kStreamWanderSeconds = 0.15f; // 中心の行き先を選び直す間隔
	const int kFilterUpdateInterval = 32;     // 中心を動かすサンプルの間隔（毎サンプルは重い）
	// タンクの中で鳴る低いこもり（空のタンクに落ちる音の胴鳴り）。
	const float kHollowCutoffHz = 320.f;
	const float kHollowGain = 0.9f;
	// 流れの太さのゆっくりした揺れ（0〜1 の割合で大きさを下げる）。
	const float kSwellHz = 1.7f;
	const float kSwellDepth = 0.35f;

	// ── 缶へ空気が戻る「ゴボッ」──
	const float kGlugMinGap = 0.38f;     // ゴボッの間隔（缶の口の大きさで決まる。少し揺らす）
	const float kGlugMaxGap = 0.55f;
	const float kGlugFromHz = 140.f;     // 低い所から上がる（泡が口を抜ける）
	const float kGlugToHz = 330.f;
	const float kGlugDuration = 0.09f;
	const float kGlugGain = 0.7f;
	const float kGlugBoomAttack = 0.004f;
	const float kGlugBoomDecay = 0.05f;
	const float kGlugBoomCutoffHz = 260.f;
	const float kGlugBoomGain = 0.5f;
	const float kGlugDuck = 0.5f;        // ゴボッの間は流れが一瞬細くなる（空気と入れ替わる）
	const float kGlugDuckSeconds = 0.08f;

	// ── 泡の「ポコッ」──
	const int kBubbleCount = 14;
	const float kBubbleMinHz = 600.f;
	const float kBubbleMaxHz = 1400.f;
	const float kBubbleRise = 2.2f;      // はじける間に上がる音程の比
	const float kBubbleMinDuration = 0.02f;
	const float kBubbleMaxDuration = 0.04f;
	const float kBubbleGain = 0.15f;

	const float kTwoPi = 6.2831853f;

	// ゴボッの時刻を並べる（頭から間隔を揺らして、ループの長さの中に収まる分だけ）。
	std::vector< float > glugTimes( Random* random ) {
		std::vector< float > times;
		float at = random->range( 0.f, kGlugMinGap );
		while ( at + kGlugDuration < kLoopLength ) {
			times.push_back( at );
			at += random->range( kGlugMinGap, kGlugMaxGap );
		}
		return times;
	}

	// その時刻がゴボッの直後か（流れを細くする割合 0〜1 を返す。1 は細くしない）。
	float duckAt( float time, const std::vector< float >& glugs ) {
		for ( float at : glugs ) {
			if ( time >= at && time < at + kGlugDuckSeconds ) {
				return kGlugDuck;
			}
		}
		return 1.f;
	}

	// 流れ落ちる音とこもりを、ループの長さ全部に足す。
	void addStream( Buffer* buffer, const std::vector< float >& glugs, Random* random ) {
		BandPass stream( kStreamLowHz, kStreamQ );
		OnePoleLowPass hollowFirst( kHollowCutoffHz );
		OnePoleLowPass hollowSecond( kHollowCutoffHz );
		const int wanderSamples = toSamples( kStreamWanderSeconds );
		float fromHz = random->range( kStreamLowHz, kStreamHighHz );
		float toHz = random->range( kStreamLowHz, kStreamHighHz );
		for ( int i = 0; i < static_cast< int >( buffer->size() ); ++i ) {
			int step = i % wanderSamples;
			if ( step == 0 && i > 0 ) {
				fromHz = toHz;
				toHz = random->range( kStreamLowHz, kStreamHighHz );
			}
			if ( i % kFilterUpdateInterval == 0 ) {
				float progress = static_cast< float >( step ) / wanderSamples;
				stream.setCenter( fromHz + ( toHz - fromHz ) * progress );
			}
			float time = static_cast< float >( i ) / kSampleRate;
			float swell = 1.f - kSwellDepth * ( 0.5f + 0.5f * std::sin( kTwoPi * kSwellHz * time ) );
			float noise = random->bipolar();
			float hollow = hollowSecond.process( hollowFirst.process( noise ) );
			( *buffer )[ i ] += ( stream.process( noise ) * kStreamGain + hollow * kHollowGain ) * swell * duckAt( time, glugs );
		}
	}
}

Buffer makeRefuelPour( Random* random ) {
	Buffer buffer = makeBuffer( kLoopLength );
	std::vector< float > glugs = glugTimes( random );
	addStream( &buffer, glugs, random );
	for ( float at : glugs ) {
		addChirp( &buffer, at, kGlugFromHz, kGlugToHz, kGlugDuration, kGlugGain );
		addBoom( &buffer, at, kGlugBoomAttack, kGlugBoomDecay, kGlugBoomCutoffHz, kGlugBoomGain, random );
	}
	for ( int i = 0; i < kBubbleCount; ++i ) {
		float duration = random->range( kBubbleMinDuration, kBubbleMaxDuration );
		float fromHz = random->range( kBubbleMinHz, kBubbleMaxHz );
		addChirp( &buffer, random->range( 0.f, kLoopLength - duration ), fromHz, fromHz * kBubbleRise, duration, kBubbleGain );
	}
	saturate( &buffer, kPourDrive );
	makeLoopSeamless( &buffer, kLoopCrossfade ); // ループなので頭とお尻は畳む（fadeEdges はしない）
	normalize( &buffer, kLoopPeak );
	return buffer;
}
