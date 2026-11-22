#include "GameSrc/Audio/AudioEngine.h"

#include "GameSrc/Audio/AudioFileSystem.h"
#include "ThirdParty/miniaudio/miniaudio.h"
#include "GameLib/GameLib.h"
#include "GameLib/Base/DebugStream.h"
#include "GameLib/Profiling/Profiler.h"

#include <random>
#include <vector>

using namespace GameLib::Math;

namespace{
	const int kMaxOneShots = 32;	// 同時に鳴らせる数。一歩で足音・服・装備の3つ鳴るので、走りながら連続で鳴っても足りる数にする
	const int kMaxLoops = 8;		// 同時に鳴らせるループの数（漁る音・注ぐ音・エンジン・切り替えで重なる BGM 2 曲 ＋ 余裕）
	const ma_uint32 kListener = 0;	// miniaudio は聞き手を複数持てるが、一人称なので1人だけ使う
	const Vector3 kWorldUp( 0.f, 1.f, 0.f );
	const int kStreamedVariation = 0;				// 流して読む音は揺らぎを持たない（1 つ目だけを読む）
	const float kFadeSilent = 0.f;					// 大きくし始める音量（無音）
	const float kFadeFull = 1.f;					// 大きくし終える音量（ma_sound_set_volume で決めた大きさのまま）
	const float kMillisecondsPerSecond = 1000.f;

	ma_uint64 toMilliseconds( float seconds ) {
		return static_cast< ma_uint64 >( seconds * kMillisecondsPerSecond );
	}
}

class AudioEngine::Impl{
public:
	// 鳴らしている音1つ。ma_sound は内部で自分自身の場所を覚えるので、作った後は動かさない（配列の中に置いたままにする）
	struct Voice{
		ma_sound sound;
		bool used;
		bool fadingOut;	// 小さくしながら止めている途中か（止まりきったら releaseFadedLoops が片付ける）
	};

	Impl();
	~Impl();

	ma_sound* chooseSource( SoundId id );														// id の揺らぎを1つ選ぶ。直前と同じものは避ける（続けて同じ音が鳴ると機械的に聞こえるため）。読めた揺らぎが無ければ nullptr
	bool initSound( ma_sound* sound, SoundId id, Bus bus, ma_uint32 flags );					// sound に id の音を用意する（鳴らし始めない）。流して読む音はここでファイルを開く
	// voice に id の音を用意して鳴らし始める。position が nullptr なら 2D。fadeInSeconds が 0 より大きければ無音から大きくする
	bool startVoice( Voice* voice, SoundId id, Bus bus, const Vector3* position, float volume, float pitch, bool looping, float fadeInSeconds );
	void releaseVoice( Voice* voice );
	void releaseFadedLoops();																	// 小さくしながら止めたループのうち、止まりきった物を片付ける
	Voice* findFreeOneShot();
	LoopHandle startLoopVoice( SoundId id, Bus bus, const Vector3* position, float volume, float fadeInSeconds );	// 空いているループの枠で id を鳴らし始め、その枠の番号を返す。position が nullptr なら 2D

	AudioFileSystem mFileSystem;	// 音のファイルを FileIO（Release は .pak）から渡す。mEngine が使うので前に置き、後に消す
	ma_engine mEngine;
	bool mEngineReady;
	ma_sound_group mBuses[ BUS_NUM ];
	bool mBusReady[ BUS_NUM ];
	// 読み込んだ音の元（揺らぎごと）。鳴らすときはこれを写した Voice を作る（元そのものは鳴らさない）
	// 写しは波形のデータを共有する（miniaudio の resource manager が持つ）ので、何個鳴らしてもメモリは増えない
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
		voice.fadingOut = false;
	}
	for ( Voice& voice : mLoops ) {
		voice.used = false;
		voice.fadingOut = false;
	}
	ma_result engineResult;
	{
		LOAD_SCOPE( "device", "ma_engine_init" );
		ma_engine_config config = ma_engine_config_init();
		config.pResourceManagerVFS = mFileSystem.vfs();
		engineResult = ma_engine_init( &config, &mEngine );
	}
	if ( engineResult != MA_SUCCESS ) {
		GameLib::cout << "AudioEngine: failed to open the audio device (sound is disabled)" << GameLib::endl;
		return;
	}
	mEngineReady = true;
	ma_engine_listener_set_world_up( &mEngine, kListener, kWorldUp.x, kWorldUp.y, kWorldUp.z );
	for ( int i = 0; i < BUS_NUM; ++i ) {
		mBusReady[ i ] = ( ma_sound_group_init( &mEngine, 0, nullptr, &mBuses[ i ] ) == MA_SUCCESS );
	}
	// 目録の音を全部読み込む（まだ数が少ないので、起動時にまとめて読んで展開しておく）。流して読む音は鳴らすときに開く
	for ( int i = 0; i < SOUND_NUM; ++i ) {
		SoundId id = static_cast< SoundId >( i );
		if ( soundStreamed( id ) ) {
			continue;
		}
		for ( int variation = 0; variation < soundEntry( id ).variationCount; ++variation ) {
			std::string path = soundFilePath( id, variation );
			LOAD_SCOPE( "sound", path.c_str() ); // 読み込みと展開（MA_SOUND_FLAG_DECODE）。起動のロードに入る
			auto source = std::make_unique< ma_sound >();
			if ( ma_sound_init_from_file( &mEngine, path.c_str(), MA_SOUND_FLAG_DECODE, nullptr, nullptr, source.get() ) != MA_SUCCESS ) {
				// tools/SoundGen を実行していないとここに来る
				GameLib::cout << "AudioEngine: sound not found: " << path.c_str() << GameLib::endl;
				continue;
			}
			mSources[ i ].push_back( std::move( source ) );
		}
	}
}

AudioEngine::Impl::~Impl() {
	// 写し（Voice）→ 元 → バス → エンジン の順に片付ける（後のものが前のものを参照しているため）
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
		// 直前の揺らぎを除いた count - 1 個から選び、直前以降の番号は1つずらす
		int previous = mLastVariation[ id ];
		variation = std::uniform_int_distribution< int >( 0, count - 2 )( mRandom );
		if ( previous >= 0 && variation >= previous ) {
			++variation;
		}
	}
	mLastVariation[ id ] = variation;
	return mSources[ id ][ variation ].get();
}

bool AudioEngine::Impl::initSound( ma_sound* sound, SoundId id, Bus bus, ma_uint32 flags ) {
	if ( !mBusReady[ bus ] ) {
		return false;
	}
	if ( soundStreamed( id ) ) {
		std::string path = soundFilePath( id, kStreamedVariation );	// 写しの元を持たないので、鳴らすたびにファイルを開く。開いたファイルは releaseVoice で閉じる
		if ( ma_sound_init_from_file( &mEngine, path.c_str(), flags | MA_SOUND_FLAG_STREAM, &mBuses[ bus ], nullptr, sound ) != MA_SUCCESS ) {
			GameLib::cout << "AudioEngine: sound not found: " << path.c_str() << GameLib::endl;
			return false;
		}
		return true;
	}
	ma_sound* source = chooseSource( id );
	return source && ma_sound_init_copy( &mEngine, source, flags, &mBuses[ bus ], sound ) == MA_SUCCESS;
}

bool AudioEngine::Impl::startVoice( Voice* voice, SoundId id, Bus bus, const Vector3* position, float volume, float pitch, bool looping, float fadeInSeconds ) {
	ma_uint32 flags = position ? 0 : MA_SOUND_FLAG_NO_SPATIALIZATION;
	if ( !initSound( &voice->sound, id, bus, flags ) ) {
		return false;
	}
	voice->used = true;
	voice->fadingOut = false;
	const SoundEntry& entry = soundEntry( id );
	float jitter = std::uniform_real_distribution< float >( -entry.pitchJitter, entry.pitchJitter )( mRandom );
	ma_sound_set_volume( &voice->sound, volume );
	if ( fadeInSeconds > 0.f ) {
		ma_sound_set_fade_in_milliseconds( &voice->sound, kFadeSilent, kFadeFull, toMilliseconds( fadeInSeconds ) );	// 鳴らし始める前に決める（始めの一瞬が大きく鳴らないように）
	}
	ma_sound_set_pitch( &voice->sound, pitch * ( 1.f + jitter ) );
	ma_sound_set_looping( &voice->sound, looping ? MA_TRUE : MA_FALSE );
	if ( position ) {
		ma_sound_set_position( &voice->sound, position->x, position->y, position->z );
		ma_sound_set_min_distance( &voice->sound, entry.minDistance );
		ma_sound_set_max_distance( &voice->sound, entry.maxDistance );
	}
	if ( entry.startSeconds > 0.f ) {
		// 位置はサンプルの番号で渡すので、素材のサンプリング周波数で秒から直す。取れなければ頭から鳴らす
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
		voice->fadingOut = false;
	}
}

void AudioEngine::Impl::releaseFadedLoops() {
	for ( Voice& voice : mLoops ) {
		if ( voice.used && voice.fadingOut && !ma_sound_is_playing( &voice.sound ) ) {
			releaseVoice( &voice );
		}
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

AudioEngine::LoopHandle AudioEngine::Impl::startLoopVoice( SoundId id, Bus bus, const Vector3* position, float volume, float fadeInSeconds ) {
	if ( !mEngineReady ) {
		return kInvalidLoop;
	}
	releaseFadedLoops();	// メニューの画面では update が呼ばれないので、枠を探す前にここでも片付ける
	for ( int i = 0; i < kMaxLoops; ++i ) {
		if ( !mLoops[ i ].used ) {
			return startVoice( &mLoops[ i ], id, bus, position, volume, 1.f, true, fadeInSeconds ) ? i : kInvalidLoop;
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
	// 鳴り終わった音を片付けて、次の音に使えるようにする。片付けは音声のスレッドではなくここ（ゲームのスレッド）で行う
	for ( Impl::Voice& voice : mImpl->mOneShots ) {
		if ( voice.used && ma_sound_at_end( &voice.sound ) ) {
			mImpl->releaseVoice( &voice );
		}
	}
	mImpl->releaseFadedLoops();
}

void AudioEngine::setPaused( bool paused ) {
	PROFILE_SCOPE( "Audio.setPaused" ); // 画面を開く瞬間に毎回通るので、別の区間で測る
	if ( !mImpl->mEngineReady ) {
		return;
	}
	// 出力の装置は動かしたまま、バスごと止める。止めたバスは中の音を読まないので、鳴っている途中の音はその位置のまま止まって続きから鳴る
	// 装置ごと止める ma_engine_stop は、音のスレッドが応えるまでゲームのスレッドを待たせるので使わない
	for ( int i = 0; i < BUS_NUM; ++i ) {
		if ( !mImpl->mBusReady[ i ] ) {
			continue;
		}
		if ( paused ) {
			ma_sound_group_stop( &mImpl->mBuses[ i ] );
		} else {
			ma_sound_group_start( &mImpl->mBuses[ i ] );
		}
	}
}

void AudioEngine::setMasterVolume( float gain ) {
	if ( !mImpl->mEngineReady ) {
		return;
	}
	ma_engine_set_volume( &mImpl->mEngine, gain );
}

void AudioEngine::setBusVolume( Bus bus, float gain ) {
	if ( !mImpl->mBusReady[ bus ] ) {
		return;
	}
	ma_sound_group_set_volume( &mImpl->mBuses[ bus ], gain );
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
		mImpl->startVoice( voice, id, bus, &position, volume, pitch, false, 0.f );
	}
}

AudioEngine::LoopHandle AudioEngine::startLoop( SoundId id, Bus bus, float volume, float fadeInSeconds ) {
	return mImpl->startLoopVoice( id, bus, nullptr, volume, fadeInSeconds );
}

AudioEngine::LoopHandle AudioEngine::startLoopAt( SoundId id, Bus bus, const Vector3& position, float volume ) {
	return mImpl->startLoopVoice( id, bus, &position, volume, 0.f );
}

void AudioEngine::stopLoop( LoopHandle handle, float fadeOutSeconds ) {
	if ( handle < 0 || handle >= kMaxLoops ) {
		return;
	}
	Impl::Voice& voice = mImpl->mLoops[ handle ];
	if ( fadeOutSeconds > 0.f && voice.used ) {
		ma_sound_stop_with_fade_in_milliseconds( &voice.sound, toMilliseconds( fadeOutSeconds ) );
		voice.fadingOut = true;	// 止まりきるまで枠は使ったまま（次のループに渡さない）。片付けは releaseFadedLoops
		return;
	}
	mImpl->releaseVoice( &voice );
}
