#include "Synth.h"

#include <algorithm>
#include <cmath>

namespace Synth{

namespace{
	const float kPi = 3.14159265358979f;
	const float kTwoPi = 2.f * kPi;
	// 指数減衰は decay の何倍まで計算するか（e^-7 ≈ 0.0009。これより先は聞こえない）。
	const float kDecayTail = 7.f;
	// setCenter で係数を計算し直す間隔（サンプル）。毎サンプルだと重く、耳ではこの細かさで十分。
	const int kSweepUpdateInterval = 32;
	// 低い一打で、叩いた瞬間だけ音程を上げる量と長さ（打撃のアタック感）。
	const float kThumpPitchBend = 0.6f;
	const float kThumpBendTime = 0.012f;
	const float kThumpAttack = 0.002f;
	// 粒のばらつき（中心周波数と大きさ）。
	const float kGrainPitchSpread = 0.3f;
	const float kGrainMinGain = 0.3f;
	const unsigned kRandomFallbackSeed = 0x9E3779B9u; // xorshift は種 0 だと 0 しか出ないため
	const float kRandomScale = 1.f / 4294967296.f;    // 2^32 分の1
	// ローパスを通したノイズは元のノイズよりずっと小さくなるので、他の部品と同じ gain で釣り合うよう持ち上げる。
	// （200Hz の1極ローパス2段は、白色ノイズの通る帯域が約 160Hz 分＝全帯域の約 1/140 なので、振幅はおよそ 1/12 になる）
	const float kBoomMakeup = 10.f;
}

// ── Random ──
Random::Random( unsigned seed ) : mState( seed ? seed : kRandomFallbackSeed ) {
}

float Random::unit() {
	mState ^= mState << 13;
	mState ^= mState >> 17;
	mState ^= mState << 5;
	return static_cast< float >( mState ) * kRandomScale;
}

float Random::bipolar() {
	return unit() * 2.f - 1.f;
}

float Random::range( float low, float high ) {
	return low + ( high - low ) * unit();
}

int Random::index( int count ) {
	return std::min( count - 1, static_cast< int >( unit() * count ) );
}

// ── OnePoleLowPass ──
OnePoleLowPass::OnePoleLowPass( float cutoffHz ) : mCoefficient( 0.f ), mState( 0.f ) {
	setCutoff( cutoffHz );
}

void OnePoleLowPass::setCutoff( float cutoffHz ) {
	mCoefficient = 1.f - std::exp( -kTwoPi * cutoffHz / kSampleRate );
}

float OnePoleLowPass::process( float input ) {
	mState += mCoefficient * ( input - mState );
	return mState;
}

// ── BandPass ──
BandPass::BandPass( float centerHz, float q ) :
	mQ( q ), mB0( 0.f ), mB2( 0.f ), mA1( 0.f ), mA2( 0.f ),
	mX1( 0.f ), mX2( 0.f ), mY1( 0.f ), mY2( 0.f ) {
	setCenter( centerHz );
}

void BandPass::setCenter( float centerHz ) {
	// RBJ Audio EQ Cookbook の「中心で 0dB」のバンドパス。
	float w0 = kTwoPi * centerHz / kSampleRate;
	float alpha = std::sin( w0 ) / ( 2.f * mQ );
	float a0 = 1.f + alpha;
	mB0 = alpha / a0;
	mB2 = -alpha / a0;
	mA1 = -2.f * std::cos( w0 ) / a0;
	mA2 = ( 1.f - alpha ) / a0;
}

float BandPass::process( float input ) {
	float output = mB0 * input + mB2 * mX2 - mA1 * mY1 - mA2 * mY2;
	mX2 = mX1;
	mX1 = input;
	mY2 = mY1;
	mY1 = output;
	return output;
}

// ── 基本 ──
int toSamples( float seconds ) {
	return static_cast< int >( seconds * kSampleRate );
}

Buffer makeBuffer( float seconds ) {
	return Buffer( toSamples( seconds ), 0.f );
}

float attackDecay( float time, float attack, float decay ) {
	if ( time < attack ) {
		return time / attack;
	}
	return std::exp( -( time - attack ) / decay );
}

// ── 足し込む部品 ──
void addNoiseHit( Buffer* buffer, float start, float attack, float decay, float centerHz, float q, float gain, Random* random ) {
	BandPass filter( centerHz, q );
	int first = toSamples( start );
	int length = toSamples( attack + decay * kDecayTail );
	for ( int i = 0; i < length && first + i < static_cast< int >( buffer->size() ); ++i ) {
		float time = static_cast< float >( i ) / kSampleRate;
		( *buffer )[ first + i ] += filter.process( random->bipolar() ) * attackDecay( time, attack, decay ) * gain;
	}
}

void addNoiseSweep( Buffer* buffer, float start, float duration, float fromHz, float toHz, float q, float gain, Random* random ) {
	BandPass filter( fromHz, q );
	int first = toSamples( start );
	int length = toSamples( duration );
	for ( int i = 0; i < length && first + i < static_cast< int >( buffer->size() ); ++i ) {
		float progress = static_cast< float >( i ) / length;
		if ( i % kSweepUpdateInterval == 0 ) {
			filter.setCenter( fromHz + ( toHz - fromHz ) * progress );
		}
		// 山なりの包絡（sin の半周）。擦れは始まりも終わりもなだらか。
		float envelope = std::sin( kPi * progress );
		( *buffer )[ first + i ] += filter.process( random->bipolar() ) * envelope * gain;
	}
}

void addModes( Buffer* buffer, float start, const Mode* modes, int count, float gain ) {
	int first = toSamples( start );
	for ( int m = 0; m < count; ++m ) {
		const Mode& mode = modes[ m ];
		int length = toSamples( mode.decay * kDecayTail );
		for ( int i = 0; i < length && first + i < static_cast< int >( buffer->size() ); ++i ) {
			float time = static_cast< float >( i ) / kSampleRate;
			( *buffer )[ first + i ] += std::sin( kTwoPi * mode.frequency * time )
				* std::exp( -time / mode.decay ) * mode.amplitude * gain;
		}
	}
}

void addThump( Buffer* buffer, float start, float frequency, float decay, float gain ) {
	int first = toSamples( start );
	int length = toSamples( kThumpAttack + decay * kDecayTail );
	float phase = 0.f;
	for ( int i = 0; i < length && first + i < static_cast< int >( buffer->size() ); ++i ) {
		float time = static_cast< float >( i ) / kSampleRate;
		float bend = 1.f + kThumpPitchBend * std::exp( -time / kThumpBendTime );
		phase += kTwoPi * frequency * bend / kSampleRate;
		( *buffer )[ first + i ] += std::sin( phase ) * attackDecay( time, kThumpAttack, decay ) * gain;
	}
}

void addBoom( Buffer* buffer, float start, float attack, float decay, float cutoffHz, float gain, Random* random ) {
	// 1段のローパスだと高音がかなり漏れて「ザッ」が混ざるので、2段重ねて低音だけにする。
	OnePoleLowPass first( cutoffHz );
	OnePoleLowPass second( cutoffHz );
	int begin = toSamples( start );
	int length = toSamples( attack + decay * kDecayTail );
	for ( int i = 0; i < length && begin + i < static_cast< int >( buffer->size() ); ++i ) {
		float time = static_cast< float >( i ) / kSampleRate;
		float low = second.process( first.process( random->bipolar() ) );
		( *buffer )[ begin + i ] += low * kBoomMakeup * attackDecay( time, attack, decay ) * gain;
	}
}

void addGrains( Buffer* buffer, float start, float duration, int count, float centerHz, float q, float grainDecay, float gain, Random* random ) {
	const float kGrainAttack = 0.0003f; // 粒は一瞬で立ち上がる（ジャリッの硬さ）
	for ( int g = 0; g < count; ++g ) {
		float at = start + random->unit() * duration;
		float center = centerHz * ( 1.f + kGrainPitchSpread * random->bipolar() );
		float grainGain = gain * random->range( kGrainMinGain, 1.f );
		addNoiseHit( buffer, at, kGrainAttack, grainDecay, center, q, grainGain, random );
	}
}

void addChirp( Buffer* buffer, float start, float fromHz, float toHz, float duration, float gain ) {
	int first = toSamples( start );
	int length = toSamples( duration );
	float phase = 0.f;
	for ( int i = 0; i < length && first + i < static_cast< int >( buffer->size() ); ++i ) {
		float progress = static_cast< float >( i ) / length;
		// 周波数は比で動かす（耳は音程を比で感じるので、直線より自然に上がって聞こえる）。
		float frequency = fromHz * std::pow( toHz / fromHz, progress );
		phase += kTwoPi * frequency / kSampleRate;
		float envelope = std::sin( kPi * progress );
		( *buffer )[ first + i ] += std::sin( phase ) * envelope * gain;
	}
}

void addEcho( Buffer* buffer, float delay, float gain, float cutoffHz ) {
	int offset = toSamples( delay );
	OnePoleLowPass filter( cutoffHz );
	// 後ろから足すと、足した結果をもう一度拾って二重に返してしまうので、元の音を写してから足す。
	Buffer source = *buffer;
	for ( int i = 0; i + offset < static_cast< int >( buffer->size() ); ++i ) {
		( *buffer )[ i + offset ] += filter.process( source[ i ] ) * gain;
	}
}

// ── 仕上げ ──
void normalize( Buffer* buffer, float peak ) {
	float loudest = 0.f;
	for ( float sample : *buffer ) {
		loudest = std::max( loudest, std::fabs( sample ) );
	}
	if ( loudest <= 0.f ) {
		return;
	}
	float scale = peak / loudest;
	for ( float& sample : *buffer ) {
		sample *= scale;
	}
}

void saturate( Buffer* buffer, float drive ) {
	// 先に一番大きい所を 1 にそろえてから潰す（元の音量で潰れ方が変わらないように）。
	normalize( buffer, 1.f );
	float scale = 1.f / std::tanh( drive ); // 潰した後も一番大きい所が 1 のままになるように
	for ( float& sample : *buffer ) {
		sample = std::tanh( sample * drive ) * scale;
	}
}

void fadeEdges( Buffer* buffer, float fadeIn, float fadeOut ) {
	int size = static_cast< int >( buffer->size() );
	int inLength = std::min( size, toSamples( fadeIn ) );
	int outLength = std::min( size, toSamples( fadeOut ) );
	for ( int i = 0; i < inLength; ++i ) {
		( *buffer )[ i ] *= static_cast< float >( i ) / inLength;
	}
	for ( int i = 0; i < outLength; ++i ) {
		( *buffer )[ size - 1 - i ] *= static_cast< float >( i ) / outLength;
	}
}

void makeLoopSeamless( Buffer* buffer, float crossfade ) {
	int fade = toSamples( crossfade );
	int loopLength = static_cast< int >( buffer->size() ) - fade;
	// 頭の fade サンプルを「お尻の続き」から「元の頭」へ入れ替えていく。こうすると最後のサンプルの次に頭が来ても
	// 波形が途切れない。ノイズ同士は相関が無いので、音量が中ほどで下がらない等パワーの重み（平方根）で混ぜる。
	for ( int i = 0; i < fade; ++i ) {
		float weight = static_cast< float >( i ) / fade;
		( *buffer )[ i ] = ( *buffer )[ i ] * std::sqrt( weight ) + ( *buffer )[ loopLength + i ] * std::sqrt( 1.f - weight );
	}
	buffer->resize( loopLength );
}

void mix( Buffer* dst, const Buffer& src, float gain ) {
	size_t length = std::min( dst->size(), src.size() );
	for ( size_t i = 0; i < length; ++i ) {
		( *dst )[ i ] += src[ i ] * gain;
	}
}

} // namespace Synth
