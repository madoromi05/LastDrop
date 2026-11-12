#ifndef INCLUDED_GAMELIB_GRAPHICS_SHADERIMPL_H
#define INCLUDED_GAMELIB_GRAPHICS_SHADERIMPL_H

#include "GameLib/Graphics/Shader.h"
#include "GameLib/Graphics/VertexFormat.h"
#include "GameLib/Base/Impl/ReferenceType.h"
#include "GameLib/Base/DebugStream.h"
#include "GameLib/Profiling/Profiler.h"

#include <d3dcompiler.h>
#include <cstdio>
#include <list>
#include <map>
#include <string>
#include <vector>

namespace GameLib{
namespace Graphics{

namespace ShaderDetail{
	const char* const kVertexEntry = "VSMain";
	const char* const kPixelEntry  = "PSMain";
	const char* const kVertexTarget = "vs_5_0";
	const char* const kPixelTarget  = "ps_5_0";

	// 読んだファイルと、そのときの更新時刻。どれかが変わったら作り直す
	struct FileStamp{
		std::string path;
		FILETIME writeTime;
	};

	inline bool readWriteTime( const std::string& path, FILETIME* out ){
		WIN32_FILE_ATTRIBUTE_DATA data;
		if ( !GetFileAttributesExA( path.c_str(), GetFileExInfoStandard, &data ) ){
			return false;
		}
		*out = data.ftLastWriteTime;
		return true;
	}

	// path を丸ごと読み、読んだことを files に記録する。開けなければ false
	inline bool readFile( const std::string& path, std::vector< char >* out, std::vector< FileStamp >* files ){
		FILE* fp = nullptr;
		if ( fopen_s( &fp, path.c_str(), "rb" ) != 0 || !fp ){
			return false;
		}
		fseek( fp, 0, SEEK_END );
		long size = ftell( fp );
		rewind( fp );
		out->resize( static_cast< size_t >( size ) );
		size_t read = ( size > 0 ) ? fread( out->data(), 1, out->size(), fp ) : 0;
		fclose( fp );
		if ( read != out->size() ){
			return false;
		}
		FileStamp stamp;
		stamp.path = path;
		if ( !readWriteTime( path, &stamp.writeTime ) ){
			return false;
		}
		files->push_back( stamp );
		return true;
	}

	// path の入っているフォルダ（末尾の区切りまで）。区切りが無ければ空
	inline std::string directoryOf( const std::string& path ){
		std::string::size_type p = path.find_last_of( "/\\" );
		return ( p == std::string::npos ) ? std::string() : path.substr( 0, p + 1 );
	}

	// #include を、元のファイルと同じフォルダから読む。読んだファイルを記録して、ホットリロードの監視対象にする
	// D3D_COMPILE_STANDARD_FILE_INCLUDE でも読めるが、何を読んだかが分からず、.hlsli の変更で作り直せないため自前で持つ
	class IncludeHandler : public ID3DInclude{
	public:
		IncludeHandler( const std::string& directory, std::vector< FileStamp >* files ) :
		mDirectory( directory ),
		mFiles( files ){
		}
		HRESULT __stdcall Open( D3D_INCLUDE_TYPE, LPCSTR name, LPCVOID, LPCVOID* data, UINT* bytes ) override {
			mBuffers.push_back( std::vector< char >() );
			if ( !readFile( mDirectory + name, &mBuffers.back(), mFiles ) ){
				mBuffers.pop_back();
				return E_FAIL;
			}
			*data = mBuffers.back().data();
			*bytes = static_cast< UINT >( mBuffers.back().size() );
			return S_OK;
		}
		HRESULT __stdcall Close( LPCVOID ) override {
			return S_OK; // 中身は mBuffers が持ち、このハンドラと一緒に消える
		}
	private:
		std::string mDirectory;
		std::vector< FileStamp >* mFiles;
		std::list< std::vector< char > > mBuffers; // list なのは、足しても前の中身のアドレスが変わらないため
	};

	inline D3D11_INPUT_ELEMENT_DESC toInputElement( VertexElement e, int offset ){
		D3D11_INPUT_ELEMENT_DESC d = {};
		d.InputSlot         = 0;
		d.AlignedByteOffset = static_cast< UINT >( offset );
		d.InputSlotClass    = D3D11_INPUT_PER_VERTEX_DATA;
		switch ( e ){
			case VERTEX_POSITION4:     d.SemanticName = "POSITION";     d.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; break;
			case VERTEX_NORMAL3:       d.SemanticName = "NORMAL";       d.Format = DXGI_FORMAT_R32G32B32_FLOAT;    break;
			case VERTEX_COLOR:         d.SemanticName = "COLOR";        d.Format = DXGI_FORMAT_B8G8R8A8_UNORM;     break;
			case VERTEX_TEXCOORD2:     d.SemanticName = "TEXCOORD";     d.Format = DXGI_FORMAT_R32G32_FLOAT;       break;
			case VERTEX_TANGENT4:      d.SemanticName = "TANGENT";      d.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; break;
			case VERTEX_BONE_INDICES4: d.SemanticName = "BLENDINDICES"; d.Format = DXGI_FORMAT_R8G8B8A8_UINT;      break;
			case VERTEX_BONE_WEIGHTS4: d.SemanticName = "BLENDWEIGHT";  d.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; break;
		}
		return d;
	}

	inline UINT compileFlags(){
		UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
		flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION; // グラフィックスデバッガで HLSL の行を追えるように
#else
		flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif
		return flags;
	}

	inline std::string blobText( ID3DBlob* b ){
		return b ? std::string( static_cast< const char* >( b->GetBufferPointer() ), b->GetBufferSize() ) : std::string();
	}

	template< class T > inline void safeRelease( T*& p ){
		if ( p ){ p->Release(); p = 0; }
	}

	// コンパイルした結果（バイトコード）を、起動中はファイル名ごとに覚えておく（FuelEscapeLoading_Plan.txt 5 章 L4）
	// 出撃のたびにシーンが同じシェーダを作り直すので、2 回目からはコンパイルを飛ばす（Profile のロードの半分がコンパイルだった）
	// 読んだファイル（#include も）の更新時刻を一緒に覚え、どれかが変わっていたら使わずにコンパイルし直す（ホットリロードはそのまま効く）
	// 入口・ターゲット・フラグはどのシェーダも同じ（kVertexEntry など）なので、ファイル名だけで引ける
	struct CompiledShader{
		std::vector< char > vs;
		std::vector< char > ps;
		std::vector< FileStamp > files;
	};

	inline std::map< std::string, CompiledShader >& compiledShaders(){
		static std::map< std::string, CompiledShader > sCompiled; // メインスレッドだけで使う（シェーダを作るのはメインスレッド）
		return sCompiled;
	}

	inline bool isUnchanged( const std::vector< FileStamp >& files ){
		for ( const FileStamp& f : files ){
			FILETIME now;
			if ( !readWriteTime( f.path, &now ) || CompareFileTime( &now, &f.writeTime ) != 0 ){
				return false;
			}
		}
		return true;
	}

	inline std::vector< char > blobBytes( ID3DBlob* b ){
		const char* data = static_cast< const char* >( b->GetBufferPointer() );
		return std::vector< char >( data, data + b->GetBufferSize() );
	}
}

class Shader::Impl : public ReferenceType{
public:
	Impl( const char* filename, const VertexFormat& format, ID3D11Device* device ) :
	mVS( 0 ),
	mPS( 0 ),
	mLayout( 0 ),
	mFilename( filename ),
	mFormat( format ),
	mLastCheckFrame( 0 ),
	mDevice( device ){
		compile();
	}
	~Impl(){
		ShaderDetail::safeRelease( mLayout );
		ShaderDetail::safeRelease( mPS );
		ShaderDetail::safeRelease( mVS );
	}
	bool isValid() const {
		return mVS && mPS;
	}
	// 読んだファイル（#include 含む）のどれかが、前回のコンパイルの後に変わったか
	bool isSourceChanged() const {
		for ( const ShaderDetail::FileStamp& f : mFiles ){
			FILETIME now;
			if ( !ShaderDetail::readWriteTime( f.path, &now ) ){
				continue; // 保存の途中で一瞬消えることがある。次に見たとき判断する
			}
			if ( CompareFileTime( &now, &f.writeTime ) != 0 ){
				return true;
			}
		}
		return false;
	}
	// ファイルを読んでコンパイルし、成功したら作ったシェーダに差し替える。失敗したら今のシェーダのまま mError に理由を残す
	bool compile(){
		using namespace ShaderDetail;
		// 起動中に HLSL をコンパイルしているので、ロードの中でも重い所になりやすい
		LOAD_SCOPE( "shader", mFilename.c_str() );
		// 起動中に同じファイルをコンパイル済みで、読んだファイルが変わっていなければ、その結果から作る
		auto cached = compiledShaders().find( mFilename );
		if ( cached != compiledShaders().end() && isUnchanged( cached->second.files ) ){
			const CompiledShader& c = cached->second;
			return createShaders( c.vs.data(), c.vs.size(), c.ps.data(), c.ps.size(), c.files );
		}
		std::vector< FileStamp > files;
		std::vector< char > source;
		if ( !readFile( mFilename, &source, &files ) ){
			// 無いファイルも、更新時刻0として見張る（後から置かれたら作り直すため）
			FileStamp missing = {};
			missing.path = mFilename;
			files.push_back( missing );
			fail( "cannot open " + mFilename, files );
			return false;
		}
		IncludeHandler include( directoryOf( mFilename ), &files );
		ID3DBlob* vsBlob = 0;
		ID3DBlob* psBlob = 0;
		ID3DBlob* error = 0;
		HRESULT hr = D3DCompile( source.data(), source.size(), mFilename.c_str(), nullptr, &include,
			kVertexEntry, kVertexTarget, compileFlags(), 0, &vsBlob, &error );
		if ( FAILED( hr ) ){
			fail( blobText( error ), files );
			safeRelease( error );
			return false;
		}
		safeRelease( error );
		hr = D3DCompile( source.data(), source.size(), mFilename.c_str(), nullptr, &include,
			kPixelEntry, kPixelTarget, compileFlags(), 0, &psBlob, &error );
		if ( FAILED( hr ) ){
			fail( blobText( error ), files );
			safeRelease( error );
			safeRelease( vsBlob );
			return false;
		}
		safeRelease( error );

		CompiledShader compiled;
		compiled.vs = blobBytes( vsBlob );
		compiled.ps = blobBytes( psBlob );
		compiled.files = files;
		safeRelease( vsBlob );
		safeRelease( psBlob );
		if ( !createShaders( compiled.vs.data(), compiled.vs.size(), compiled.ps.data(), compiled.ps.size(), files ) ){
			return false;
		}
		compiledShaders()[ mFilename ] = std::move( compiled ); // 作れた物だけ覚える
		return true;
	}

	// バイトコードからシェーダと頂点の並びを作り、成功したら今のシェーダに差し替える。files: そのとき読んだファイル（見張る物）
	bool createShaders( const void* vsCode, size_t vsSize, const void* psCode, size_t psSize,
		const std::vector< ShaderDetail::FileStamp >& files ){
		using namespace ShaderDetail;
		ID3D11VertexShader* vs = 0;
		ID3D11PixelShader* ps = 0;
		ID3D11InputLayout* layout = 0;
		bool created =
			SUCCEEDED( mDevice->CreateVertexShader( vsCode, vsSize, nullptr, &vs ) ) &&
			SUCCEEDED( mDevice->CreatePixelShader( psCode, psSize, nullptr, &ps ) ) &&
			createLayout( vsCode, vsSize, &layout );
		if ( !created ){
			safeRelease( layout );
			safeRelease( ps );
			safeRelease( vs );
			fail( mFilename + " : cannot create shader (VSMain's input does not match the VertexFormat?)", files );
			return false;
		}
		safeRelease( mLayout );
		safeRelease( mPS );
		safeRelease( mVS );
		mVS = vs;
		mPS = ps;
		mLayout = layout;
		mFiles = files;
		mError.clear();
		return true;
	}

	ID3D11VertexShader* mVS;
	ID3D11PixelShader*  mPS;
	ID3D11InputLayout*  mLayout;
	std::string mFilename;
	std::string mError;
	VertexFormat mFormat;
	unsigned mLastCheckFrame; // 最後に isSourceChanged を見たフレーム（Manager が使う）
private:
	bool createLayout( const void* vsCode, size_t vsSize, ID3D11InputLayout** out ){
		D3D11_INPUT_ELEMENT_DESC elements[ VertexFormat::MAX_ELEMENT ];
		int n = mFormat.elementNumber();
		for ( int i = 0; i < n; ++i ){
			elements[ i ] = ShaderDetail::toInputElement( mFormat.element( i ), mFormat.offset( i ) );
		}
		// 頂点を読まないシェーダ（SV_VertexID だけの全画面描画）でも作れる。余った要素は使われないだけ
		return SUCCEEDED( mDevice->CreateInputLayout( elements, static_cast< UINT >( n ),
			vsCode, vsSize, out ) );
	}
	// 失敗を記録する。読めたファイルは監視対象にする（直して保存したら、もう一度コンパイルするため）
	void fail( const std::string& message, const std::vector< ShaderDetail::FileStamp >& files ){
		mError = message;
		if ( !files.empty() ){
			mFiles = files;
		}
		GameLib::cout << "Shader : " << mError.c_str() << GameLib::endl;
	}
	std::vector< ShaderDetail::FileStamp > mFiles;
	ID3D11Device* mDevice;
};

} //namespace Graphics
} //namespace GameLib

#endif
