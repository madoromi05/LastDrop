#include "Generators.h"

#include <cmath>

using namespace Synth;

// 環境音。今は脱出のサイレンだけ（風・鳥・虫・発電機・飛行機は 2026-10-23 に消した。ユーザー指定）
namespace{
	const float kPi = 3.14159265358979f;
	const float kTwoPi = 2.f * kPi;

	// ── サイレン ──
	const float kSirenLength = 12.f;
	const float kSirenLowHz = 150.f;
	const float kSirenStartHz = 200.f;
	const float kSirenHighHz = 620.f;
	const float kSirenRiseEnd = 3.f;		// ここまでで回転が上がりきる
	const float kSirenFallStart = 8.f;		// ここから回転が落ちていく
	const float kSirenRiseTime = 0.9f;		// 上がり方の速さ（指数で近づく時定数）
	const float kSirenWobbleHz = 0.5f;		// 回っている間の音程の揺れ
	const float kSirenWobbleDepth = 15.f;
	const float kSirenSecondRotor = 1.19f;	// 2つ目の音（短3度ほど上）。2音が重なると空襲警報らしい不協和になる
	const float kSirenSecondGain = 0.6f;
	const int kSirenHarmonics = 6;
	const float kSirenRolloff = 1.2f;
	const float kSirenFadeIn = 0.4f;
	const float kSirenEchoDelay = 0.35f;	// 建物に跳ね返って返ってくる音（遠くで鳴っている感じ）
	const float kSirenEchoGain = 0.35f;
	const float kSirenEchoCutoff = 1500.f;
	const float kSirenLateEchoDelay = 0.9f;
	const float kSirenLateEchoGain = 0.2f;
	const float kSirenLateEchoCutoff = 1000.f;
	const float kSirenPeak = 0.7f;

	// サイレンの回転の高さ（Hz）。上がりきる → 回り続ける → 落ちる
	float sirenFrequency( float time ) {
		if ( time < kSirenFallStart ) {
			float rise = kSirenStartHz + ( kSirenHighHz - kSirenStartHz ) * ( 1.f - std::exp( -time / kSirenRiseTime ) );
			float wobble = ( time > kSirenRiseEnd ) ? kSirenWobbleDepth * std::sin( kTwoPi * kSirenWobbleHz * time ) : 0.f;
			return rise + wobble;
		}
		float fall = ( time - kSirenFallStart ) / ( kSirenLength - kSirenFallStart );
		return kSirenHighHz + ( kSirenLowHz - kSirenHighHz ) * fall;
	}

	// サイレンの大きさ。回転が落ちていくにつれて小さくなる
	float sirenLevel( float time ) {
		if ( time < kSirenFallStart ) {
			return 1.f;
		}
		return 1.f - ( time - kSirenFallStart ) / ( kSirenLength - kSirenFallStart );
	}
}

Buffer makeSiren( Random* random ) {
	Buffer buffer = makeBuffer( kSirenLength );
	// 音程が動き続けるので、位相を少しずつ進める（時刻×周波数で求めると、周波数が変わった瞬間に波形が飛ぶ）
	float phases[ kSirenHarmonics ] = {};
	float secondPhases[ kSirenHarmonics ] = {};
	for ( int n = 0; n < kSirenHarmonics; ++n ) {
		phases[ n ] = random->unit() * kTwoPi;
		secondPhases[ n ] = random->unit() * kTwoPi;
	}
	for ( size_t i = 0; i < buffer.size(); ++i ) {
		float time = static_cast< float >( i ) / kSampleRate;
		float frequency = sirenFrequency( time );
		float sample = 0.f;
		for ( int n = 0; n < kSirenHarmonics; ++n ) {
			int harmonic = n + 1;
			float amplitude = 1.f / std::pow( static_cast< float >( harmonic ), kSirenRolloff );
			phases[ n ] += kTwoPi * frequency * harmonic / kSampleRate;
			secondPhases[ n ] += kTwoPi * frequency * kSirenSecondRotor * harmonic / kSampleRate;
			sample += ( std::sin( phases[ n ] ) + std::sin( secondPhases[ n ] ) * kSirenSecondGain ) * amplitude;
		}
		buffer[ i ] = sample * sirenLevel( time );
	}
	fadeEdges( &buffer, kSirenFadeIn, 0.f );
	addEcho( &buffer, kSirenEchoDelay, kSirenEchoGain, kSirenEchoCutoff );
	addEcho( &buffer, kSirenLateEchoDelay, kSirenLateEchoGain, kSirenLateEchoCutoff );
	normalize( &buffer, kSirenPeak );
	return buffer;
}
