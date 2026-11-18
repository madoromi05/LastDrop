#include "GameSrc/Audio/MusicPlayer.h"

#include "GameLib/GameLib.h"

namespace{
	const float kMusicVolume = 1.f;		// 曲の大きさは書き出しのときに揃えてあるので、そのまま鳴らす（つまみは BUS_MUSIC の音量）
	const float kFadeInSeconds = 1.f;	// 鳴らし始めに大きくしていく時間。画面が出てすぐ大きな音で始まらないように
	const float kFadeOutSeconds = 0.8f;	// 止めるときに小さくしていく時間。次の曲と少しだけ重ねて、切れ目を目立たせない
}

MusicPlayer::MusicPlayer( AudioEngine* audio ) :
	mAudio( audio ),
	mCurrent( SOUND_NUM ),
	mLoop( AudioEngine::kInvalidLoop ) {
	ASSERT( mAudio && "MusicPlayer : audio is null." );
}

void MusicPlayer::play( SoundId music ) {
	if ( mLoop != AudioEngine::kInvalidLoop && music == mCurrent ) {
		return;
	}
	stop();
	mCurrent = music;
	mLoop = mAudio->startLoop( music, AudioEngine::BUS_MUSIC, kMusicVolume, kFadeInSeconds );
}

void MusicPlayer::stop() {
	mAudio->stopLoop( mLoop, kFadeOutSeconds );
	mLoop = AudioEngine::kInvalidLoop;
}
