#ifndef INCLUDED_GAMELIB_SOUND_PLAYER_H
#define INCLUDED_GAMELIB_SOUND_PLAYER_H

namespace GameLib{
namespace Sound{

class Wave;

class Player{
public:
	static Player create( Wave );
	static Player create( int bitsPerSample, int bufferSizeInByte, int frequency, int channelNumber );	///ストリーミング再生用Player

	///再生。無限ループ再生させたければtrueを渡す
	void play( bool looping = false );
	void stop();
	void setVolume( int volume );	///ボリューム設定(100が最大、0が最小)
	bool isPlaying() const;			//再生中ですか？

	bool write( int position, const char* data, int size );	///ストリーミングバッファに書き込む。再生位置が近づきすぎて失敗した場合はfalseを返す。音飛びする
	bool fillSilence( int position, int size );				///ストリーミングバッファに無音を書き込む。再生位置が近づきすぎて失敗した場合はfalseを返す。音飛びする
	int position() const;									///再生中のバッファ位置をバイト単位で返す

	//以下ユーザは意識しない関数群
	Player();
	Player( const Player& );
	~Player();
	operator void*() const;
	Player& operator=( const Player& );
	void release();
	bool operator==( const Player& ) const;
	bool operator!=( const Player& ) const;
private:
	class Impl;
	Impl* mImpl;
};

}
}

#endif
