#include "Graphics/ShaderDiskCache.h"

#include <cstdint>
#include <cstdio>
#include <cstring>

namespace GameLib{
namespace Graphics{

namespace{
	// ファイルの並び：頭の印 4 バイト → 形式の番号 → コンパイルの設定 → 頂点のバイトコード → 画素のバイトコード → 読んだファイルの数 →
	// （パス → 更新時刻の下位 → 上位）× 数。数と長さは 32 ビット。最後のバイトの後に余りがあるファイルは読まない
	const char kMagic[] = { 'S', 'H', 'D', 'C' };	// 覚えたシェーダのファイルか（違うファイルを読まないため）
	const std::uint32_t kFormatVersion = 1;			// 並びを変えたら上げる（古い並びのファイルは読まない）
	const char* const kExtension = ".bin";
	const char kNameSeparator = '_';				// シェーダのパスの区切りを、1 つのファイル名にするためにこれへ替える
	// 壊れたファイルで大きすぎる確保をしないための上限（どれも今のシェーダの 100 倍以上）
	const std::uint32_t kMaxBytecodeBytes = 16 * 1024 * 1024;
	const std::uint32_t kMaxFileCount = 256;
	const std::uint32_t kMaxPathLength = 4096;

	bool breaksFileName( char c ){
		return c == '/' || c == '\\' || c == ':';
	}

	// directory（末尾に区切り）までのフォルダを、上から順に作る。もうある段は何もしない
	void createDirectories( const std::string& directory ){
		for ( std::string::size_type i = 1; i < directory.size(); ++i ){
			if ( directory[ i ] == '/' || directory[ i ] == '\\' ){
				CreateDirectoryA( directory.substr( 0, i ).c_str(), nullptr );
			}
		}
	}

	// 順に書く。1 つでも書けなかったら ok() が false になる
	class Writer{
	public:
		explicit Writer( FILE* fp ) : mFile( fp ), mOk( true ){}
		void bytes( const void* data, size_t size ){
			if ( mOk && size > 0 && fwrite( data, 1, size, mFile ) != size ){
				mOk = false;
			}
		}
		void u32( std::uint32_t value ){
			bytes( &value, sizeof( value ) );
		}
		void blob( const std::vector< char >& data ){
			u32( static_cast< std::uint32_t >( data.size() ) );
			bytes( data.data(), data.size() );
		}
		void text( const std::string& s ){
			u32( static_cast< std::uint32_t >( s.size() ) );
			bytes( s.data(), s.size() );
		}
		bool ok() const { return mOk; }
	private:
		FILE* mFile;
		bool mOk;
	};

	// 順に読む。足りない・上限を超える長さが 1 つでもあったら ok() が false になり、後は何も読まない
	class Reader{
	public:
		explicit Reader( FILE* fp ) : mFile( fp ), mOk( true ){}
		void bytes( void* out, size_t size ){
			if ( mOk && size > 0 && fread( out, 1, size, mFile ) != size ){
				mOk = false;
			}
		}
		void u32( std::uint32_t* out ){
			bytes( out, sizeof( *out ) );
		}
		void blob( std::vector< char >* out, std::uint32_t maxSize ){
			std::uint32_t size = 0;
			u32( &size );
			if ( !mOk || size > maxSize ){
				mOk = false;
				return;
			}
			out->resize( size );
			bytes( out->data(), size );
		}
		void text( std::string* out, std::uint32_t maxSize ){
			std::vector< char > chars;
			blob( &chars, maxSize );
			out->assign( chars.begin(), chars.end() );
		}
		bool atEnd(){
			char extra;
			return mOk && fread( &extra, 1, 1, mFile ) == 0;
		}
		bool ok() const { return mOk; }
	private:
		FILE* mFile;
		bool mOk;
	};
}

ShaderDiskCache::ShaderDiskCache( const std::string& directory, unsigned compileFlags ) :
mDirectory( directory ),
mCompileFlags( compileFlags ){
}

bool ShaderDiskCache::load( const std::string& shaderFile, ShaderDetail::CompiledShader* out ) const {
	FILE* fp = nullptr;
	if ( fopen_s( &fp, pathOf( shaderFile ).c_str(), "rb" ) != 0 || !fp ){
		return false;
	}
	Reader reader( fp );
	char magic[ sizeof( kMagic ) ] = {};
	std::uint32_t version = 0;
	std::uint32_t flags = 0;
	reader.bytes( magic, sizeof( magic ) );
	reader.u32( &version );
	reader.u32( &flags );
	bool usable = reader.ok() && memcmp( magic, kMagic, sizeof( kMagic ) ) == 0 &&
		version == kFormatVersion && flags == mCompileFlags;
	ShaderDetail::CompiledShader read;
	std::uint32_t fileCount = 0;
	if ( usable ){
		reader.blob( &read.vs, kMaxBytecodeBytes );
		reader.blob( &read.ps, kMaxBytecodeBytes );
		reader.u32( &fileCount );
		usable = reader.ok() && fileCount <= kMaxFileCount;
	}
	for ( std::uint32_t i = 0; usable && i < fileCount; ++i ){
		ShaderDetail::FileStamp stamp;
		std::uint32_t low = 0;
		std::uint32_t high = 0;
		reader.text( &stamp.path, kMaxPathLength );
		reader.u32( &low );
		reader.u32( &high );
		stamp.writeTime.dwLowDateTime = low;
		stamp.writeTime.dwHighDateTime = high;
		read.files.push_back( stamp );
		usable = reader.ok();
	}
	usable = usable && reader.atEnd();
	fclose( fp );
	if ( !usable ){
		return false;
	}
	*out = std::move( read );
	return true;
}

void ShaderDiskCache::save( const std::string& shaderFile, const ShaderDetail::CompiledShader& compiled ) const {
	createDirectories( mDirectory );
	std::string path = pathOf( shaderFile );
	FILE* fp = nullptr;
	if ( fopen_s( &fp, path.c_str(), "wb" ) != 0 || !fp ){
		return;
	}
	Writer writer( fp );
	writer.bytes( kMagic, sizeof( kMagic ) );
	writer.u32( kFormatVersion );
	writer.u32( mCompileFlags );
	writer.blob( compiled.vs );
	writer.blob( compiled.ps );
	writer.u32( static_cast< std::uint32_t >( compiled.files.size() ) );
	for ( const ShaderDetail::FileStamp& stamp : compiled.files ){
		writer.text( stamp.path );
		writer.u32( stamp.writeTime.dwLowDateTime );
		writer.u32( stamp.writeTime.dwHighDateTime );
	}
	bool written = writer.ok();
	fclose( fp );
	if ( !written ){
		DeleteFileA( path.c_str() ); // 書きかけのファイルを残さない（読むときにも弾くが、次の起動で毎回開いて捨てないように）
	}
}

std::string ShaderDiskCache::pathOf( const std::string& shaderFile ) const {
	std::string name = shaderFile;
	for ( char& c : name ){
		if ( breaksFileName( c ) ){
			c = kNameSeparator;
		}
	}
	return mDirectory + name + kExtension;
}

} //namespace Graphics
} //namespace GameLib
