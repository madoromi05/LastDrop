#ifndef INCLUDED_GAMELIB_BASE_MEMORYMANAGER_H
#define INCLUDED_GAMELIB_BASE_MEMORYMANAGER_H

namespace GameLib{

//newをつかさどるクラス
class MemoryManager{
public:
	static MemoryManager instance();

	void write( const char* filename = 0 );	///情報の書き出し(ファイル名を与えればファイルへ、与えなければデバグ出力へ)
	int totalSize() const;					///全確保容量(内部予約容量を含む)
};
}

void* operator new( size_t size, const char* filename, int line );
void* operator new[]( size_t size, const char* filename, int line );
void operator delete( void*, const char*, int );
void operator delete[]( void*, const char*, int );
void* operator new( size_t size );
void* operator new[]( size_t size );
void operator delete( void* );
void operator delete[]( void* );

#endif
