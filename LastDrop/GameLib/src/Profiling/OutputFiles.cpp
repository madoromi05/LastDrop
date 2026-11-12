#ifdef ZF_EDITOR

#include <windows.h>
#undef min
#undef max

#include "Profiling/OutputFiles.h"

#include <cstdio>
#include <fstream>
#include <sstream>

namespace GameLib{
namespace Profiling{
namespace OutputFiles{

namespace{
	// 書き出し先。作業フォルダ（VS から実行したときはプロジェクトのフォルダ）からの相対パス
	const char* const kDirectory = "Profiling";
	const char* const kTemporarySuffix = ".tmp";
	const int kTimestampBufferSize = 32;
}

std::string timestamp(){
	SYSTEMTIME t;
	GetLocalTime( &t );
	char buffer[ kTimestampBufferSize ];
	snprintf( buffer, sizeof( buffer ), "%04u%02u%02u_%02u%02u%02u",
		t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond );
	return buffer;
}

std::string dateTime(){
	SYSTEMTIME t;
	GetLocalTime( &t );
	char buffer[ kTimestampBufferSize ];
	snprintf( buffer, sizeof( buffer ), "%04u/%02u/%02u %02u:%02u:%02u",
		t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond );
	return buffer;
}

std::string pathOf( const std::string& fileName ){
	// 最初に呼ばれたときの時刻で 1 回だけ決める（起動中に書くファイルが全部同じフォルダに入る）
	static const std::string launchDirectory = std::string( kDirectory ) + "/" + timestamp();
	// 既にあれば失敗するだけなので、毎回呼んでよい。親から順に作る
	CreateDirectoryA( kDirectory, nullptr );
	CreateDirectoryA( launchDirectory.c_str(), nullptr );
	return launchDirectory + "/" + fileName;
}

std::string rootPathOf( const std::string& fileName ){
	CreateDirectoryA( kDirectory, nullptr );
	return std::string( kDirectory ) + "/" + fileName;
}

bool write( const std::string& path, const std::string& text ){
	std::string temporary = path + kTemporarySuffix;
	{
		std::ofstream out( temporary.c_str(), std::ios::binary | std::ios::trunc );
		if ( !out ){
			return false;
		}
		out.write( text.data(), static_cast< std::streamsize >( text.size() ) );
		if ( !out ){
			return false;
		}
	}
	return MoveFileExA( temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING ) != 0;
}

bool read( const std::string& path, std::string* text ){
	std::ifstream in( path.c_str(), std::ios::binary );
	if ( !in ){
		return false;
	}
	std::ostringstream buffer;
	buffer << in.rdbuf();
	*text = buffer.str();
	return true;
}

} //namespace OutputFiles
} //namespace Profiling
} //namespace GameLib

#endif // ZF_EDITOR
