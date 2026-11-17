#ifndef INCLUDED_GAMELIB_GRAPHICS_SHADERDISKCACHE_H
#define INCLUDED_GAMELIB_GRAPHICS_SHADERDISKCACHE_H

#include "Graphics/CompiledShader.h"
#include <string>

namespace GameLib{
namespace Graphics{

// コンパイルしたシェーダ（CompiledShader）を、起動をまたいで使えるようにファイルへ書く・ファイルから読む係（ShaderDiskCache_Plan.txt）
// 1 本のシェーダにつき 1 ファイル（覚える所 + シェーダのパスの区切りを _ にした名前 + .bin）
// 読んだ物が今のファイルと同じか（更新時刻）は見ない。使う側（ShaderImpl::compile）が ShaderDetail::isUnchanged で見る
class ShaderDiskCache{
public:
	// directory：覚える所（末尾に区切りを付ける。無ければ save のときに作る）
	// compileFlags：今のコンパイルの設定（D3DCompile に渡す値。違う設定で覚えた物は読まない）
	ShaderDiskCache( const std::string& directory, unsigned compileFlags );

	bool load( const std::string& shaderFile, ShaderDetail::CompiledShader* out ) const;			// shaderFile を覚えた物があり、形式と設定が今と同じなら out へ読んで true
	void save( const std::string& shaderFile, const ShaderDetail::CompiledShader& compiled ) const;	// 書けなくても何もしない（次の起動でコンパイルし直すだけ）

private:
	std::string pathOf( const std::string& shaderFile ) const;	// shaderFile を覚えるファイルのパス

	std::string mDirectory;
	unsigned mCompileFlags;
};

} //namespace Graphics
} //namespace GameLib

#endif
