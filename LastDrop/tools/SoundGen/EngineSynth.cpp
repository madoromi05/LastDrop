#include "Generators.h"

#include <cmath>

using namespace Synth;

// 脱出の車のエンジン（SummaryPlan/個別仕様/出撃と帰還/FuelEscapeLoading_Plan.txt Q5）。手元に車の録音が無いので合成する
// エンジンの音は「燃える回数（毎秒）の低い周期音」と、その倍音、こもったノイズでできている（4 気筒・アイドリング 約 800 回転なら、
// 毎秒 約 27 回燃える）。掛ける音は、セル（スターター）が数回クランクを回す「グッ・グッ」の後に、燃え始めてアイドリングへ移る
namespace{
	const float kPi = 3.14159265f;
	const float kTwoPi = 2.f * kPi;
	const float kEngineDrive = 1.6f; // 仕上げの潰し具合（金属の振動の荒さ）

	// ── アイドリング ──
	const float kIdleFiringHz = 27.f;     // 燃える回数（毎秒）
	const int kIdleHarmonics = 8;
	const float kIdleRolloff = 1.1f;      // 倍音が上へ行くほど小さくなる割合
	const float kIdlePulseDepth = 0.5f;   // 1 回燃えるごとの膨らみ
	const float kIdlePulseSharpness = 4.f;
	const float kIdlePulseRate = 0.5f;    // |sin| は 1 周期に 2 回膨らむので、半分の速さで回して 1 回燃えるごとに 1 回にする
	const float kIdleJitter = 0.03f;      // 回転のむら（一定だと機械の合成音に聞こえる）
	const float kIdleJitterHz = 1.7f;
	const float kIdleNoiseCutoff = 350.f; // こもった排気の音
	const float kIdleNoiseGain = 0.35f;
	const float kIdleLoopLength = 2.4f;   // 畳む前の長さ
	const float kIdleLoopCrossfade = 0.3f;
	const float kIdlePeak = 0.5f;

	// ── 掛ける音 ──
	const float kStartLength = 2.4f;
	const int kCrankCount = 5;            // セルがクランクを回す「グッ」の数
	const float kCrankInterval = 0.17f;
	const float kCrankThumpHz = 70.f;
	const float kCrankThumpDecay = 0.06f;
	const float kCrankThumpGain = 0.7f;
	const float kCrankNoiseAttack = 0.004f;
	const float kCrankNoiseDecay = 0.07f;
	const float kCrankNoiseHz = 500.f;
	const float kCrankNoiseQ = 1.2f;
	const float kCrankNoiseGain = 0.4f;
	const float kStarterWhineHz = 210.f;  // セルのモーターのうなり
	const float kStarterWhineGain = 0.12f;
	const float kCatchSeconds = kCrankCount * kCrankInterval; // 燃え始める時刻
	const float kCatchBoomAttack = 0.01f;
	const float kCatchBoomDecay = 0.25f;
	const float kCatchBoomCutoff = 220.f;
	const float kCatchBoomGain = 0.8f;
	const float kCatchRevSeconds = 0.5f;  // 掛かった直後に回転が上がって、アイドリングへ落ちるまで
	const float kCatchRevScale = 1.6f;    // 上がったときの回転（アイドリングの何倍か）
	const float kStartFadeOut = 0.4f;     // 終わりはアイドリングのループへつなぐので、消えるように
	const float kStartPeak = 0.6f;

	// start 秒目から duration 秒、エンジンの燃える音を足す。rpmScale( 経った秒 ) で回転を変え、level( 経った秒 ) で大きさを変える
	template< class RpmScale, class Level >
	void addEngineHum( Buffer* buffer, float start, float duration, RpmScale rpmScale, Level level, Random* random ) {
		float phase = random->unit() * kTwoPi;
		float harmonicPhases[ kIdleHarmonics ];
		for ( int n = 0; n < kIdleHarmonics; ++n ) {
			harmonicPhases[ n ] = random->unit() * kTwoPi;
		}
		OnePoleLowPass rumble( kIdleNoiseCutoff );
		int first = toSamples( start );
		int count = toSamples( duration );
		for ( int i = 0; i < count && first + i < static_cast< int >( buffer->size() ); ++i ) {
			float time = static_cast< float >( i ) / kSampleRate;
			float jitter = 1.f + kIdleJitter * std::sin( kTwoPi * kIdleJitterHz * time );
			float frequency = kIdleFiringHz * rpmScale( time ) * jitter;
			// 位相を少しずつ進める（回転が変わった瞬間に波形が飛ばないように。サイレンと同じ）
			phase += kTwoPi * frequency / kSampleRate;
			float sample = 0.f;
			for ( int n = 0; n < kIdleHarmonics; ++n ) {
				int harmonic = n + 1;
				float amplitude = 1.f / std::pow( static_cast< float >( harmonic ), kIdleRolloff );
				sample += std::sin( phase * harmonic + harmonicPhases[ n ] ) * amplitude;
			}
			// 1 回燃えるごとに膨らむ（低い周期の「ドッドッ」）
			float pulse = 1.f - kIdlePulseDepth + kIdlePulseDepth * std::pow( std::fabs( std::sin( phase * kIdlePulseRate ) ), kIdlePulseSharpness );
			float noise = rumble.process( random->bipolar() ) * kIdleNoiseGain;
			( *buffer )[ first + i ] += ( sample * pulse + noise * pulse ) * level( time );
		}
	}
}

Buffer makeEngineIdle( Random* random ) {
	Buffer buffer = makeBuffer( kIdleLoopLength );
	addEngineHum( &buffer, 0.f, kIdleLoopLength, []( float ) { return 1.f; }, []( float ) { return 1.f; }, random );
	saturate( &buffer, kEngineDrive );
	makeLoopSeamless( &buffer, kIdleLoopCrossfade ); // ループなので頭とお尻は畳む（漁る音と同じ）
	normalize( &buffer, kIdlePeak );
	return buffer;
}

Buffer makeEngineStart( Random* random ) {
	Buffer buffer = makeBuffer( kStartLength );
	// セルが回る間：クランクの「グッ」と、モーターのうなり
	for ( int i = 0; i < kCrankCount; ++i ) {
		float start = i * kCrankInterval;
		addThump( &buffer, start, kCrankThumpHz, kCrankThumpDecay, kCrankThumpGain );
		addNoiseHit( &buffer, start, kCrankNoiseAttack, kCrankNoiseDecay, kCrankNoiseHz, kCrankNoiseQ, kCrankNoiseGain, random );
	}
	int whineSamples = toSamples( kCatchSeconds );
	for ( int i = 0; i < whineSamples && i < static_cast< int >( buffer.size() ); ++i ) {
		float time = static_cast< float >( i ) / kSampleRate;
		buffer[ i ] += std::sin( kTwoPi * kStarterWhineHz * time ) * kStarterWhineGain;
	}
	// 燃え始め：低い「ボン」と、一度回転が上がってアイドリングへ落ちる音
	addBoom( &buffer, kCatchSeconds, kCatchBoomAttack, kCatchBoomDecay, kCatchBoomCutoff, kCatchBoomGain, random );
	auto rpmScale = []( float time ) {
		float t = ( time < kCatchRevSeconds ) ? time / kCatchRevSeconds : 1.f;
		return kCatchRevScale + ( 1.f - kCatchRevScale ) * t; // 上がった回転から、アイドリングへ
	};
	auto level = []( float ) { return 1.f; };
	addEngineHum( &buffer, kCatchSeconds, kStartLength - kCatchSeconds, rpmScale, level, random );
	saturate( &buffer, kEngineDrive );
	fadeEdges( &buffer, 0.f, kStartFadeOut );
	normalize( &buffer, kStartPeak );
	return buffer;
}
