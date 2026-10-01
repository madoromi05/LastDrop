#include "Generators.h"

#include <cmath>

using namespace Synth;

// 環境音。ほとんどがゲーム中にループで鳴らすので、継ぎ目が聞こえないことが一番大事。
// 継ぎ目の消し方は2通り:
//  ・周期のある音（エンジン・虫の鳴き声）: ループの長さにちょうど整数回収まる周波数・間隔にする（そのまま頭に戻れる）
//  ・ノイズ（風・機械の雑音）: ループより少し長く作り、お尻を頭に重ねて畳む（makeLoopSeamless）
namespace{
	const float kPi = 3.14159265358979f;
	const float kTwoPi = 2.f * kPi;

	// 正弦波の倍音を重ねる（エンジンの「ブーン」）。倍音 n の大きさは 1/n^rolloff。
	// 位相をずらすのは、全部 0 から始めると波形が尖って「ビー」というブザーのような音になるため。
	void addHarmonics( Buffer* buffer, float fundamentalHz, int count, float rolloff, float gain, Random* random ) {
		for ( int n = 1; n <= count; ++n ) {
			float frequency = fundamentalHz * n;
			if ( frequency * 2.f >= kSampleRate ) {
				break; // 標本化できる上限を超える倍音は折り返して別の高さに聞こえるので足さない
			}
			float amplitude = gain / std::pow( static_cast< float >( n ), rolloff );
			float phase = random->unit() * kTwoPi;
			for ( size_t i = 0; i < buffer->size(); ++i ) {
				float time = static_cast< float >( i ) / kSampleRate;
				( *buffer )[ i ] += std::sin( kTwoPi * frequency * time + phase ) * amplitude;
			}
		}
	}

	// 帯域を絞ったノイズを、ループで継ぎ目なく鳴るように作る。
	Buffer makeSeamlessNoise( float loopLength, float crossfade, float centerHz, float q, Random* random ) {
		Buffer noise = makeBuffer( loopLength + crossfade );
		BandPass filter( centerHz, q );
		for ( float& sample : noise ) {
			sample = filter.process( random->bipolar() );
		}
		makeLoopSeamless( &noise, crossfade );
		return noise;
	}

	// ── 風 ──
	const float kWindLoop = 10.f;
	const float kWindCrossfade = 2.f;
	// 突風の強さ（0〜1）は、周期の違うゆっくりした波を重ねて作る（1つだと規則的すぎて機械っぽい）。
	struct GustWave{ float frequency, phase, weight; };
	const GustWave kGustWaves[] = {
		{ 0.07f, 0.f, 0.5f }, { 0.13f, 1.f, 0.3f }, { 0.21f, 2.f, 0.2f },
	};
	const float kWindBaseCutoff = 250.f;	// 風が弱いときの「ゴー」の高さ
	const float kWindGustCutoff = 500.f;	// 突風で上がる分
	const float kWindMinLevel = 0.4f;		// 風が一番弱いときの大きさ
	const float kWhistleHz = 700.f;			// 物の隙間を抜ける「ヒュー」
	const float kWhistleQ = 8.f;
	const float kWhistleGain = 0.3f;
	const float kWindPeak = 0.5f;

	// ── 鳥 ──
	const float kBirdsLoop = 12.f;
	const float kLongestCall = 1.f;		// 鳴き声1回の最長（ループの終わりで切れないよう、この分手前までに鳴き始める）
	const int kBirdCallCount = 7;
	struct ChirpNote{ float offset, fromHz, toHz, duration; };
	// 下がる口笛「ピーョ」を2回。
	const ChirpNote kWhistleCall[] = {
		{ 0.f, 4200.f, 2600.f, 0.18f }, { 0.25f, 4200.f, 2600.f, 0.18f },
	};
	// 高さの違う2音「ピッ・ピー」。
	const ChirpNote kTwoToneCall[] = {
		{ 0.f, 2800.f, 2900.f, 0.12f }, { 0.16f, 3600.f, 3700.f, 0.14f },
	};
	// さえずり（短い音の連打）。
	const int kTrillNotes = 8;
	const ChirpNote kTrillNote = { 0.035f, 5200.f, 5600.f, 0.025f }; // offset は1音ごとの間隔
	const float kBirdMinGain = 0.3f;	// 遠くの鳥ほど小さい
	const float kBirdEchoDelay = 0.07f;	// 林に反射して返ってくる音
	const float kBirdEchoGain = 0.3f;
	const float kBirdEchoCutoff = 3000.f;
	const float kBirdLateEchoDelay = 0.19f;
	const float kBirdLateEchoGain = 0.15f;
	const float kBirdLateEchoCutoff = 2000.f;
	const float kBirdsPeak = 0.35f;
	enum BirdCall{ CALL_WHISTLE, CALL_TWO_TONE, CALL_TRILL, CALL_NUM };

	// ── 虫 ──
	// 間隔もキャリアもループ 6 秒に整数回収まる値にしてある（6 / 0.5 = 12 回、4600Hz × 6 秒 = 27600 周期）。
	const float kInsectLoop = 6.f;
	const float kInsectCrossfade = 1.f;
	struct Cricket{ float carrierHz, chirpInterval, chirpOffset; int pulses; float pulseInterval, pulseLength, gain; };
	const Cricket kCrickets[] = {
		{ 4600.f, 0.5f, 0.f,   3, 0.025f, 0.015f, 1.0f },
		{ 4200.f, 0.6f, 0.23f, 4, 0.022f, 0.012f, 0.6f },
	};
	const float kInsectBedHz = 6000.f;	// 遠くの虫がまとまった「シー」
	const float kInsectBedQ = 0.5f;
	const float kInsectBedGain = 0.15f;
	const float kInsectsPeak = 0.3f;

	// ── 発電機 ──
	// 回転 1500rpm = 25Hz。倍音は全部 25Hz の整数倍なので、2 秒ループに 50 周期ぴったり収まる。
	const float kGeneratorLoop = 2.f;
	const float kGeneratorCrossfade = 0.5f;
	const float kRotationHz = 25.f;
	const int kGeneratorHarmonics = 24;
	const float kGeneratorRolloff = 1.f;
	const float kFiringBoost = 1.5f;		// 爆発の周期（2倍音）を強めて「ドドド」を出す
	const float kChugDepth = 0.35f;			// 爆発ごとの音量の揺れ
	const float kMechanicalHz = 1200.f;		// 部品がこすれる「シャー」
	const float kMechanicalQ = 1.f;
	const float kMechanicalGain = 0.3f;
	const float kGeneratorPeak = 0.6f;

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

	// ── 飛行機（4発のプロペラ機）──
	// エンジンごとに少しずつ回転がずれて「ウォンウォン」とうなる。どれも 4 秒ループに整数回収まる周波数。
	const float kAirplaneLoop = 4.f;
	const float kAirplaneCrossfade = 1.f;
	const float kPropellerHz[] = { 68.f, 68.5f, 67.75f, 68.25f };
	const int kPropellerHarmonics = 20;
	const float kPropellerRolloff = 1.f;
	const float kPropellerGain = 0.25f;
	const float kTurbineHz = 2100.f;		// タービンの「キーン」
	const float kTurbineGain = 0.03f;
	const float kAirflowHz = 400.f;			// 風切りの「ゴー」
	const float kAirflowQ = 0.5f;
	const float kAirflowGain = 0.6f;
	const float kAirplanePeak = 0.6f;

	// 鳴き声1回ぶんの音符を start から並べる。
	void addCall( Buffer* buffer, const ChirpNote* notes, int count, float start, float gain ) {
		for ( int i = 0; i < count; ++i ) {
			const ChirpNote& note = notes[ i ];
			addChirp( buffer, start + note.offset, note.fromHz, note.toHz, note.duration, gain );
		}
	}

	// 虫1匹ぶんの鳴き声を、ループの長さで折り返しながら足す（最後の鳴き声がはみ出しても頭につながる）。
	void addCricket( Buffer* buffer, const Cricket& cricket ) {
		int loopSamples = static_cast< int >( buffer->size() );
		int pulseSamples = toSamples( cricket.pulseLength );
		int chirps = static_cast< int >( std::lround( kInsectLoop / cricket.chirpInterval ) );
		for ( int chirp = 0; chirp < chirps; ++chirp ) {
			for ( int pulse = 0; pulse < cricket.pulses; ++pulse ) {
				float start = cricket.chirpOffset + chirp * cricket.chirpInterval + pulse * cricket.pulseInterval;
				int first = toSamples( start );
				for ( int i = 0; i < pulseSamples; ++i ) {
					int index = ( first + i ) % loopSamples;
					float time = static_cast< float >( index ) / kSampleRate; // 位相は折り返した後の時刻で決める（継ぎ目で位相が飛ばない）
					float envelope = std::sin( kPi * i / pulseSamples );
					( *buffer )[ index ] += std::sin( kTwoPi * cricket.carrierHz * time ) * envelope * cricket.gain;
				}
			}
		}
	}

	// サイレンの回転の高さ（Hz）。上がりきる → 回り続ける → 落ちる。
	float sirenFrequency( float time ) {
		if ( time < kSirenFallStart ) {
			float rise = kSirenStartHz + ( kSirenHighHz - kSirenStartHz ) * ( 1.f - std::exp( -time / kSirenRiseTime ) );
			float wobble = ( time > kSirenRiseEnd ) ? kSirenWobbleDepth * std::sin( kTwoPi * kSirenWobbleHz * time ) : 0.f;
			return rise + wobble;
		}
		float fall = ( time - kSirenFallStart ) / ( kSirenLength - kSirenFallStart );
		return kSirenHighHz + ( kSirenLowHz - kSirenHighHz ) * fall;
	}

	// サイレンの大きさ。回転が落ちていくにつれて小さくなる。
	float sirenLevel( float time ) {
		if ( time < kSirenFallStart ) {
			return 1.f;
		}
		return 1.f - ( time - kSirenFallStart ) / ( kSirenLength - kSirenFallStart );
	}
}

Buffer makeWind( Random* random ) {
	Buffer buffer = makeBuffer( kWindLoop + kWindCrossfade );
	OnePoleLowPass body( kWindBaseCutoff );
	BandPass whistle( kWhistleHz, kWhistleQ );
	for ( size_t i = 0; i < buffer.size(); ++i ) {
		float time = static_cast< float >( i ) / kSampleRate;
		float gust = 0.f;
		for ( const GustWave& wave : kGustWaves ) {
			gust += std::sin( kTwoPi * wave.frequency * time + wave.phase ) * wave.weight;
		}
		gust = 0.5f + 0.5f * gust; // -1〜1 → 0〜1
		float noise = random->bipolar();
		body.setCutoff( kWindBaseCutoff + kWindGustCutoff * gust );
		float level = kWindMinLevel + ( 1.f - kWindMinLevel ) * gust;
		// 隙間の「ヒュー」は強い突風のときだけ聞こえるよう、強さの2乗で効かせる。
		buffer[ i ] = body.process( noise ) * level + whistle.process( noise ) * gust * gust * kWhistleGain;
	}
	makeLoopSeamless( &buffer, kWindCrossfade );
	normalize( &buffer, kWindPeak );
	return buffer;
}

Buffer makeBirds( Random* random ) {
	Buffer buffer = makeBuffer( kBirdsLoop );
	for ( int i = 0; i < kBirdCallCount; ++i ) {
		float start = random->unit() * ( kBirdsLoop - kLongestCall );
		float gain = random->range( kBirdMinGain, 1.f );
		switch ( random->index( CALL_NUM ) ) {
		case CALL_WHISTLE:
			addCall( &buffer, kWhistleCall, sizeof( kWhistleCall ) / sizeof( kWhistleCall[ 0 ] ), start, gain );
			break;
		case CALL_TWO_TONE:
			addCall( &buffer, kTwoToneCall, sizeof( kTwoToneCall ) / sizeof( kTwoToneCall[ 0 ] ), start, gain );
			break;
		default: // CALL_TRILL
			for ( int n = 0; n < kTrillNotes; ++n ) {
				addChirp( &buffer, start + kTrillNote.offset * n, kTrillNote.fromHz, kTrillNote.toHz, kTrillNote.duration, gain );
			}
			break;
		}
	}
	addEcho( &buffer, kBirdEchoDelay, kBirdEchoGain, kBirdEchoCutoff );
	addEcho( &buffer, kBirdLateEchoDelay, kBirdLateEchoGain, kBirdLateEchoCutoff );
	normalize( &buffer, kBirdsPeak );
	return buffer;
}

Buffer makeInsects( Random* random ) {
	Buffer buffer = makeSeamlessNoise( kInsectLoop, kInsectCrossfade, kInsectBedHz, kInsectBedQ, random );
	for ( float& sample : buffer ) {
		sample *= kInsectBedGain;
	}
	for ( const Cricket& cricket : kCrickets ) {
		addCricket( &buffer, cricket );
	}
	normalize( &buffer, kInsectsPeak );
	return buffer;
}

Buffer makeGenerator( Random* random ) {
	Buffer buffer = makeBuffer( kGeneratorLoop );
	addHarmonics( &buffer, kRotationHz, kGeneratorHarmonics, kGeneratorRolloff, 1.f, random );
	addHarmonics( &buffer, kRotationHz * 2.f, 1, 0.f, kFiringBoost - 1.f, random );
	Buffer mechanical = makeSeamlessNoise( kGeneratorLoop, kGeneratorCrossfade, kMechanicalHz, kMechanicalQ, random );
	mix( &buffer, mechanical, kMechanicalGain );
	// 爆発ごとの揺れ。25Hz なので 2 秒に 50 回、ループの頭とお尻で揃う。
	for ( size_t i = 0; i < buffer.size(); ++i ) {
		float time = static_cast< float >( i ) / kSampleRate;
		buffer[ i ] *= 1.f + kChugDepth * std::sin( kTwoPi * kRotationHz * time );
	}
	normalize( &buffer, kGeneratorPeak );
	return buffer;
}

Buffer makeSiren( Random* random ) {
	Buffer buffer = makeBuffer( kSirenLength );
	// 音程が動き続けるので、位相を少しずつ進める（時刻×周波数で求めると、周波数が変わった瞬間に波形が飛ぶ）。
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

Buffer makeAirplane( Random* random ) {
	Buffer buffer = makeBuffer( kAirplaneLoop );
	for ( float propellerHz : kPropellerHz ) {
		addHarmonics( &buffer, propellerHz, kPropellerHarmonics, kPropellerRolloff, kPropellerGain, random );
	}
	addHarmonics( &buffer, kTurbineHz, 1, 0.f, kTurbineGain, random );
	Buffer airflow = makeSeamlessNoise( kAirplaneLoop, kAirplaneCrossfade, kAirflowHz, kAirflowQ, random );
	mix( &buffer, airflow, kAirflowGain );
	normalize( &buffer, kAirplanePeak );
	return buffer;
}
