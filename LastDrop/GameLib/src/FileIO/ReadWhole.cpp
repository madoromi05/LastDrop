#include "GameLib/GameLib.h"
#include "GameLib/FileIO/ReadWhole.h"
#include "GameLib/FileIO/InFile.h"
#include "GameLib/FileIO/Manager.h"
#include "GameLib/Threading/Functions.h"

namespace GameLib{
namespace FileIO{

namespace{
	const int kWaitMilliSeconds = 1;	// 読み込みのスレッドが終えるのを待つ間隔
}

bool readWhole( const char* filename, std::vector< char >* out ){
	if ( !Manager().exists( filename ) ){
		return false;
	}
	InFile file = InFile::create( filename );
	while ( !file.isFinished() ){
		Threading::sleep( kWaitMilliSeconds );
	}
	if ( file.isError() ){
		return false;
	}
	out->assign( file.data(), file.data() + file.size() );
	return true;
}

} //namespace FileIO
} //namespace GameLib
