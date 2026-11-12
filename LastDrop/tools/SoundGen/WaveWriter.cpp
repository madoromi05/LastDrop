#include "WaveWriter.h"

#include <algorithm>
#include <cstdint>
#include <fstream>

namespace{
	const uint16_t kFormatPcm = 1;
	const uint16_t kChannelCount = 1;
	const uint16_t kBitsPerSample = 16;
	const uint16_t kBytesPerSample = kBitsPerSample / 8;
	const uint32_t kFormatChunkSize = 16;	// PCM の fmt チャンクの中身の大きさ
	const uint32_t kRiffHeaderRest = 36;	// RIFF の大きさ = 36 + データの大きさ（"WAVE" + fmt チャンク + data の見出し）
	const float kSampleMax = 32767.f;

	// WAV はリトルエンディアン。Windows（x86/x64）のメモリの並びのまま書けばよい
	template< class T > void writeValue( std::ofstream& stream, T value ) {
		stream.write( reinterpret_cast< const char* >( &value ), sizeof( T ) );
	}
}

bool writeWave( const std::string& path, const Synth::Buffer& buffer ) {
	std::ofstream stream( path, std::ios::binary );
	if ( !stream ) {
		return false;
	}
	uint32_t dataSize = static_cast< uint32_t >( buffer.size() ) * kBytesPerSample;
	uint32_t byteRate = Synth::kSampleRate * kChannelCount * kBytesPerSample;
	uint16_t blockAlign = kChannelCount * kBytesPerSample;

	stream.write( "RIFF", 4 );
	writeValue< uint32_t >( stream, kRiffHeaderRest + dataSize );
	stream.write( "WAVE", 4 );
	stream.write( "fmt ", 4 );
	writeValue< uint32_t >( stream, kFormatChunkSize );
	writeValue< uint16_t >( stream, kFormatPcm );
	writeValue< uint16_t >( stream, kChannelCount );
	writeValue< uint32_t >( stream, Synth::kSampleRate );
	writeValue< uint32_t >( stream, byteRate );
	writeValue< uint16_t >( stream, blockAlign );
	writeValue< uint16_t >( stream, kBitsPerSample );
	stream.write( "data", 4 );
	writeValue< uint32_t >( stream, dataSize );
	for ( float sample : buffer ) {
		float clamped = std::max( -1.f, std::min( 1.f, sample ) );
		writeValue< int16_t >( stream, static_cast< int16_t >( clamped * kSampleMax ) );
	}
	return static_cast< bool >( stream );
}
