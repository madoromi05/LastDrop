#ifndef INCLUDED_GAMELIB_GRAPHICS_COMPILEDSHADER_H
#define INCLUDED_GAMELIB_GRAPHICS_COMPILEDSHADER_H

#include <windows.h>
#include <cstdint>
#include <string>
#include <vector>

namespace GameLib{
namespace Graphics{
namespace ShaderDetail{
	// 読んだファイルと、そのときの更新時刻と中身のハッシュ。どれかが変わったら作り直す
	// ディスクから読むときは更新時刻で、.pak から読むとき（LASTDROP_PACKED）は更新時刻が無いのでハッシュで比べる
	struct FileStamp{
		std::string path;
		FILETIME writeTime;
		std::uint64_t contentHash;
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
