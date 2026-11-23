#ifndef INCLUDED_MUSIC_PLAYER_H
#define INCLUDED_MUSIC_PLAYER_H

#include "GameSrc/Audio/AudioEngine.h"
#include "GameSrc/Audio/SoundCatalog.h"

// 画面の BGM を 1 曲だけ、位置なしのループで鳴らす係
// ・曲を替えると、前の曲を小さくしながら止め、新しい曲を無音から大きくしながら鳴らし始める（2 曲は少しの間だけ重なる）
// ・同じ曲を続けて頼まれたら何もしない。タイトルから設定のページへ行っても、曲が頭に戻らない
// シーンをまたいで鳴り続けるので、持ち主はシーンではなく GameResources
class MusicPlayer{
public:
	// audio: 鳴らす装置。所有権は受け取らない。nullptr 不可。この係より長く生きていること
	explicit MusicPlayer( AudioEngine* audio );
	MusicPlayer( const MusicPlayer& ) = delete;
	MusicPlayer& operator=( const MusicPlayer& ) = delete;

	void play( SoundId music );	// music をループで鳴らす。同じ曲が鳴っていれば何もしない
	void stop();				// 今の曲を小さくしながら止める。鳴っていなければ何もしない

private:
	AudioEngine* mAudio;
	SoundId mCurrent;					// 鳴らしている曲。mLoop が kInvalidLoop の間は意味を持たない
	AudioEngine::LoopHandle mLoop;		// 鳴らしている曲のループの番号。鳴っていなければ kInvalidLoop
};

#endif
