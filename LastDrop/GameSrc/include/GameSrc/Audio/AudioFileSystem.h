#ifndef INCLUDED_GAMESRC_AUDIO_AUDIOFILESYSTEM_H
#define INCLUDED_GAMESRC_AUDIO_AUDIOFILESYSTEM_H

#include "ThirdParty/miniaudio/miniaudio.h"

// miniaudio に音のファイルを FileIO から渡す係（ResourcePack_Plan.txt）
// miniaudio は自分でディスクを開くので、そのままだと Release の .pak を見に行かない。ファイルの開き方（ma_vfs）をこれに差し替える
// 開くときに FileIO でファイル全体を読み、あとはメモリから渡す。AudioEngine だけが持つ（miniaudio.h を読むのは Audio の .cpp だけ）
class AudioFileSystem{
public:
	AudioFileSystem();
	AudioFileSystem( const AudioFileSystem& ) = delete;
	AudioFileSystem& operator=( const AudioFileSystem& ) = delete;

	ma_vfs* vfs();	// ma_engine_config の pResourceManagerVFS に渡す。エンジンより長く生かしておく

private:
	ma_vfs_callbacks mCallbacks;	// miniaudio は ma_vfs* をこの形の先頭として読むので、最初のメンバに置く
};

#endif
