#ifndef INCLUDED_GAMELIB_GRAPHICS_COMPILEDSHADER_H
#define INCLUDED_GAMELIB_GRAPHICS_COMPILEDSHADER_H

#include <windows.h>
#include <string>
#include <vector>

namespace GameLib{
namespace Graphics{
namespace ShaderDetail{
	// 読んだファイルと、そのときの更新時刻。どれかが変わったら作り直す
	struct FileStamp{
		std::string path;
		FILETIME writeTime;
	};
	// コンパイルした結果（頂点と画素のバイトコード）と、そのとき読んだファイル（#include も）
	// 起動中はメモリ（ShaderImpl.h の compiledShaders）、起動をまたいではファイル（ShaderDiskCache）に覚える
	struct CompiledShader{
		std::vector< char > vs;
		std::vector< char > ps;
		std::vector< FileStamp > files;
	};
}
} //namespace Graphics
} //namespace GameLib

#endif
