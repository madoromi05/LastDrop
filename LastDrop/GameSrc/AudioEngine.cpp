#include "AudioEngine.h"

#include "ThirdParty/miniaudio/miniaudio.h"
#include "GameLib/GameLib.h"
#include "GameLib/Base/DebugStream.h"

#include <random>
#include <vector>

using namespace GameLib::Math;

namespace{
	// 同時に鳴らせる数。一歩で足音・服・装備の3つ鳴るので、走りながら連続で鳴っても足りる数にする。
	const int kMaxOneShots = 32;
	// 同時に鳴らせるループの数（環境音3つ・発電機・飛行機 ＋ 余裕）。
	const int kMaxLoops = 8;
	const ma_uint32 kListener = 0; // miniaudio は聞き手を複数持てるが、一人称なので1人だけ使う
	const Vector3 kWorldUp( 0.f, 1.f, 0.f );
}

class AudioEngine::Impl{
public:
	// 鳴らしている音1つ。ma_sound は内部で自分自身の場所を覚えるので、作った後は動かさない（配列の中に置いたままにする）。
	struct Voice{
		ma_sound sound;
		bool used;
	};

	Impl();
	~Impl();

	// id の揺らぎを1つ選ぶ。直前と同じものは避ける（続けて同じ音が鳴ると機械的に聞こえるため）。読めた揺らぎが無ければ nullptr。
	ma_sound* chooseSource( SoundId id );
	// voice に id の音を用意して鳴らし始める。position が nullptr なら 2D。
	bool startVoice( Voice* voice, SoundId id, Bus bus, const Vector3* position, float volume, float pitch, bool looping );
	void releaseVoice( Voice* voice );
	Voice* findFreeOneShot();
	// 空いているループの枠で id を鳴らし始め、その枠の番号を返す。position が nullptr なら 2D。
	LoopHandle startLoopVoice( SoundId id, Bus bus, const Vector3* position, float volume );

	ma_engine mEngine;
	bool mEngineReady;
	ma_sound_group mBuses[ BUS_NUM ];
	bool mBusReady[ BUS_NUM ];
	// 読み込んだ音の元（揺らぎごと）。鳴らすときはこれを写した Voice を作る（元そのものは鳴らさない）。
	// 写しは波形のデータを共有する（miniaudio の resource manager が持つ）ので、何個鳴らしてもメモリは増えない。
	std::vector< std::unique_ptr< ma_sound > > mSources[ SOUND_NUM ];
	int mLastVariation[ SOUND_NUM ];
	Voice mOneShots[ kMaxOneShots ];
	Voice mLoops[ kMaxLoops ];
	std::minstd_rand mRandom;
};

AudioEngine::Impl::Impl() : mEngineReady( false ) {
	for ( int i = 0; i < BUS_NUM; ++i ) {
		mBusReady[ i ] = false;
	}
	for ( int i = 0; i < SOUND_NUM; ++i ) {
		mLastVariation[ i ] = -1;
	}
	for ( Voice& voice : mOneShots ) {
		voice.used = false;
	}
	for ( Voice& voice : mLoops ) {
		voice.used = false;
	}
	if ( ma_engine_init( nullptr, &mEngine ) != MA_SUCCESS ) {
		GameLib::cout << "AudioEngine: failed to open the audio device (sound is disabled)" << GameLib::endl;
		return;
	}
	mEngineReady = true;
	ma_engine_listener_set_world_up( &mEngine, kListener, kWorldUp.x, kWorldUp.y, kWorldUp.z );
	for ( int i = 0; i < BUS_NUM; ++i ) {
		mBusReady[ i ] = ( ma_sound_group_init( &mEngine, 0, nullptr, &mBuses[ i ] ) == MA_SUCCESS );
	}
	// 目録の音を全部読み込む（まだ数が少ないので、起動時にまとめて読んで展開しておく）。
	for ( int i = 0; i < SOUND_NUM; ++i ) {
		SoundId id = static_cast< SoundId >( i );
		for ( int variation = 0; variation < soundEntry( id ).variationCount; ++variation ) {
			std::string path = soundFilePath( id, variation );
			auto source = std::make_unique< ma_sound >();
			if ( ma_sound_init_from_file( &mEngine, path.c_str(), MA_SOUND_FLAG_DECODE, nullptr, nullptr, source.get() ) != MA_SUCCESS ) {
				// tools/SoundGen を実行していないとここに来る。
				GameLib::cout << "AudioEngine: sound not found: " << path.c_str() << GameLib::endl;
				continue;
			}
			mSources[ i ].push_back( std::move( source ) );
		}
	}
}

AudioEngine::Impl::~Impl() {
	// 写し（Voice）→ 元 → バス → エンジン の順に片付ける（後のものが前のものを参照しているため）。
	for ( Voice& voice : mOneShots ) {
		releaseVoice( &voice );
	}
	for ( Voice& voice : mLoops ) {
		releaseVoice( &voice );
	}
	for ( auto& sources : mSources ) {
		for ( auto& source : sources ) {
			ma_sound_uninit( source.get() );
		}
	}
	for ( int i = 0; i < BUS_NUM; ++i ) {
		if ( mBusReady[ i ] ) {
			ma_sound_group_uninit( &mBuses[ i ] );
		}
	}
	if ( mEngineReady ) {
		ma_engine_uninit( &mEngine );
	}
}

ma_sound* AudioEngine::Impl::chooseSource( SoundId id ) {
	int count = static_cast< int >( mSources[ id ].size() );
	if ( count == 0 ) {
		return nullptr;
	}
	int variation = 0;
	if ( count > 1 ) {
		// 直前の揺らぎを除いた count - 1 個から選び、直前以降の番号は1つずらす。
		int previous = mLastVariation[ id ];
		variation = std::uniform_int_distribution< int >( 0, count - 2 )( mRandom );
		if ( previous >= 0 && variation >= previous ) {
			++variation;
		}
	}
	mLastVariation[ id ] = variation;
	return mSources[ id ][ variation ].get();
}

bool AudioEngine::Impl::startVoice( Voice* voice, SoundId id, Bus bus, const Vector3* position, float volume, float pitch, bool looping ) {
	ma_sound* source = chooseSource( id );
	if ( !source || !mBusReady[ bus ] ) {
		return false;
	}
	ma_uint32 flags = position ? 0 : MA_SOUND_FLAG_NO_SPATIALIZATION;
	if ( ma_sound_init_copy( &mEngine, source, flags, &mBuses[ bus ], &voice->sound ) != MA_SUCCESS ) {
		return false;
	}
	voice->used = true;
	const SoundEntry& entry = soundEntry( id );
	float jitter = std::uniform_real_distribution< float >( -entry.pitchJitter, entry.pitchJitter )( mRandom );
	ma_sound_set_volume( &voice->sound, volume );
	ma_sound_set_pitch( &voice->sound, pitch * ( 1.f + jitter ) );
	ma_sound_set_looping( &voice->sound, looping ? MA_TRUE : MA_FALSE );
	if ( position ) {
		ma_sound_set_position( &voice->sound, position->x, position->y, position->z );
		ma_sound_set_min_distance( &voice->sound, entry.minDistance );
		ma_sound_set_max_distance( &voice->sound, entry.maxDistance );
	}
	if ( entry.startSeconds > 0.f ) {
		// 位置はサンプルの番号で渡すので、素材のサンプリング周波数で秒から直す。取れなければ頭から鳴らす。
		ma_uint32 sampleRate = 0;
		if ( ma_sound_get_data_format( &voice->sound, nullptr, nullptr, &sampleRate, nullptr, 0 ) == MA_SUCCESS ) {
			ma_sound_seek_to_pcm_frame( &voice->sound, static_cast< ma_uint64 >( entry.startSeconds * sampleRate ) );
		}
	}
	ma_sound_start( &voice->sound );
	return true;
}

void AudioEngine::Impl::releaseVoice( Voice* voice ) {
	if ( voice->used ) {
		ma_sound_uninit( &voice->sound );
		voice->used = false;
	}
}

AudioEngine::Impl::Voice* AudioEngine::Impl::findFreeOneShot() {
	for ( Voice& voice : mOneShots ) {
		if ( !voice.used ) {
			return &voice;
		}
	}
	return nullptr;
}

AudioEngine::LoopHandle AudioEngine::Impl::startLoopVoice( SoundId id, Bus bus, const Vector3* position, float volume ) {
	if ( !mEngineReady ) {
		return kInvalidLoop;
	}
	for ( int i = 0; i < kMaxLoops; ++i ) {
		if ( !mLoops[ i ].used ) {
			return startVoice( &mLoops[ i ], id, bus, position, volume, 1.f, true ) ? i : kInvalidLoop;
		}
	}
	return kInvalidLoop;
}

// ── AudioEngine ──
AudioEngine::AudioEngine() : mImpl( std::make_unique< Impl >() ) {
}

AudioEngine::~AudioEngine() {
}

bool AudioEngine::isReady() const {
	return mImpl->mEngineReady;
}

void AudioEngine::update() {
	// 鳴り終わった音を片付けて、次の音に使えるようにする。片付けは音声のスレッドではなくここ（ゲームのスレッド）で行う。
	for ( Impl::Voice& voice : mImpl->mOneShots ) {
		if ( voice.used && ma_sound_at_end( &voice.sound ) ) {
			mImpl->releaseVoice( &voice );
		}
	}
}

void AudioEngine::setListener( const Vector3& position, const Vector3& forward ) {
	if ( !isReady() ) {
		return;
	}
	ma_engine_listener_set_position( &mImpl->mEngine, kListener, position.x, position.y, position.z );
	ma_engine_listener_set_direction( &mImpl->mEngine, kListener, forward.x, forward.y, forward.z );
}

void AudioEngine::play( SoundId id, Bus bus, const Vector3& position, float volume, float pitch ) {
	Impl::Voice* voice = isReady() ? mImpl->findFreeOneShot() : nullptr;
	if ( voice ) {
		mImpl->startVoice( voice, id, bus, &position, volume, pitch, false );
	}
}

void AudioEngine::play2D( SoundId id, Bus bus, float volume, float pitch ) {
	Impl::Voice* voice = isReady() ? mImpl->findFreeOneShot() : nullptr;
	if ( voice ) {
		mImpl->startVoice( voice, id, bus, nullptr, volume, pitch, false );
	}
}

AudioEngine::LoopHandle AudioEngine::startLoop( SoundId id, Bus bus, float volume ) {
	return mImpl->startLoopVoice( id, bus, nullptr, volume );
}

AudioEngine::LoopHandle AudioEngine::startLoopAt( SoundId id, Bus bus, const Vector3& position, float volume ) {
	return mImpl->startLoopVoice( id, bus, &position, volume );
}

void AudioEngine::setLoopPosition( LoopHandle handle, const Vector3& position, const Vector3& velocity ) {
	if ( handle < 0 || handle >= kMaxLoops || !mImpl->mLoops[ handle ].used ) {
		return;
	}
	ma_sound* sound = &mImpl->mLoops[ handle ].sound;
	ma_sound_set_position( sound, position.x, position.y, position.z );
	ma_sound_set_velocity( sound, velocity.x, velocity.y, velocity.z );
}

void AudioEngine::stopLoop( LoopHandle handle ) {
	if ( handle < 0 || handle >= kMaxLoops ) {
		return;
	}
	mImpl->releaseVoice( &mImpl->mLoops[ handle ] );
}
