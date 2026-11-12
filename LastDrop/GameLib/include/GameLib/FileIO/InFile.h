#ifndef INCLUDED_GAMELIB_FILEIO_INFILE_H
#define INCLUDED_GAMELIB_FILEIO_INFILE_H

namespace GameLib{
namespace FileIO{

///読み込みファイルハンドル
class InFile{
public:
	static InFile create( const char* filename );	///ファイルロードを開始する
	int size() const;								///ロード容量ゲット
	const char* data() const;						///ロード内容ゲット
	bool isFinished() const;						///ロード終わった？
	bool isError() const;							///エラー出てます？
	//値ゲット関数群
	int getInt( int position ) const;
	unsigned getUnsigned( int position ) const;
	short getShort( int position ) const;
	unsigned short getUnsignedShort( int position ) const;

	//以下ユーザは意識しない関数群
	InFile();
	InFile( const InFile& );
	~InFile();
	operator void*() const;
	InFile& operator=( const InFile& );
	void release();
	bool operator==( const InFile& ) const;
	bool operator!=( const InFile& ) const;

	class Impl;
private:
	Impl* mImpl;
};

} //namespace FileIO
} //namespace GameLib

#endif
