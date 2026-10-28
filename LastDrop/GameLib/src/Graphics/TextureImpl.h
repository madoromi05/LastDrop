#ifndef INCLUDED_GAMELIB_GRAPHICS_TEXTUREIMPL_H
#define INCLUDED_GAMELIB_GRAPHICS_TEXTUREIMPL_H

#include "GameLib/FileIO/Manager.h"
#include "GameLib/FileIO/InFile.h"
#include "GameLib/Base/Impl/ReferenceType.h"
#include "GameLib/PseudoXml/Element.h"
#include "GameLib/PseudoXml/Attribute.h"
#include "GameLib/Base/RefString.h"
#include "GameLib/Profiling/Profiler.h"
#include "Graphics/ColorSpace.h"
#include "Graphics/DdsReader.h"
#include "GameLib/Base/DebugStream.h"
#include <string>
#include <vector>
#include <algorithm>

namespace GameLib{
using namespace std;
using namespace GameLib::PseudoXml;
namespace Graphics{

class Texture::Impl : public ReferenceType{
public:
	enum Extension{ EXT_TGA, EXT_DDS, EXT_UNKNOWN };

	struct MipLevel{
		vector< unsigned > pixels;
		int width;
		int height;
	};

	// Constructor from PseudoXml element (async file load)
	Impl( ConstElement e, const char* path, ID3D11Device* device, ID3D11DeviceContext* context ) :
	mGpuTexture( 0 ),
	mSRV( 0 ),
	mDevice( device ),
	mContext( context ),
	mIsError( false ),
	mWidth( 0 ),
	mHeight( 0 ),
	mOriginalWidth( 0 ),
	mOriginalHeight( 0 ),
	mNeedsUpload( false ),
	mExtension( EXT_UNKNOWN ),
	mName( 0 ),
	mRequestedAt( 0 ),
	mColorSpace( TEXTURE_COLOR_SPACE_LINEAR ){
		mIsError = true;
		int an = e.attributeNumber();
		for ( int i = 0; i < an; ++i ){
			ConstAttribute a = e.attribute( i );
			RefString name( a.name() );
			if ( name == "name" ){
				RefString tmpName( a.value() );
				mName = tmpName.copyOriginal();
			}else if ( name == "filename" ){
				mExtension = getExtension( a.value() );
				if ( mExtension != EXT_UNKNOWN ){
					string filename = ( path ) ? path : "";
					if ( filename.size() > 0 ){
						char last = filename[ filename.size() - 1 ];
						if ( last != '\\' && last != '/' ) filename += '/';
					}
					filename += a.value();
					mRequestedAt = Profiling::now();
					mProfileName = filename;
					mFile = FileIO::InFile::create( filename.c_str() );
					mIsError = false;
				}
			}
		}
	}
	// Constructor from filename (async file load)
	Impl( const char* filename, TextureColorSpace colorSpace, ID3D11Device* device, ID3D11DeviceContext* context ) :
	mGpuTexture( 0 ),
	mSRV( 0 ),
	mDevice( device ),
	mContext( context ),
	mIsError( false ),
	mWidth( 0 ),
	mHeight( 0 ),
	mOriginalWidth( 0 ),
	mOriginalHeight( 0 ),
	mNeedsUpload( false ),
	mExtension( EXT_UNKNOWN ),
	mName( 0 ),
	mRequestedAt( 0 ),
	mColorSpace( colorSpace ){
		mExtension = getExtension( filename );
		if ( mExtension != EXT_UNKNOWN ){
			mRequestedAt = Profiling::now();
			mProfileName = filename;
			mFile = FileIO::InFile::create( filename );
		}else{
			mIsError = true;
		}
	}
	// Constructor for procedural (pre-allocated) texture
	Impl( int w, int h, bool createMipChain, ID3D11Device* device, ID3D11DeviceContext* context ) :
	mGpuTexture( 0 ),
	mSRV( 0 ),
	mDevice( device ),
	mContext( context ),
	mIsError( false ),
	mWidth( 0 ),
	mHeight( 0 ),
	mOriginalWidth( w ),
	mOriginalHeight( h ),
	mNeedsUpload( false ),
	mExtension( EXT_UNKNOWN ),
	mName( 0 ),
	mRequestedAt( 0 ),
	mColorSpace( TEXTURE_COLOR_SPACE_LINEAR ){
		createStagingObject( w, h, createMipChain );
	}
	// Constructor from raw data buffer
	Impl( const char* data, int size, ID3D11Device* device, ID3D11DeviceContext* context ) :
	mGpuTexture( 0 ),
	mSRV( 0 ),
	mDevice( device ),
	mContext( context ),
	mIsError( false ),
	mWidth( 0 ),
	mHeight( 0 ),
	mOriginalWidth( 0 ),
	mOriginalHeight( 0 ),
	mNeedsUpload( false ),
	mExtension( EXT_UNKNOWN ),
	mName( 0 ),
	mRequestedAt( 0 ),
	mColorSpace( TEXTURE_COLOR_SPACE_LINEAR ){
		if ( data[ 0 ] == 'D' && data[ 1 ] == 'D' && data[ 2 ] == 'S' && data[ 3 ] == ' ' ){
			readDDS( data, size );
		}else{
			readTGA( data, size );
		}
	}
	~Impl(){
		if ( mSRV ){ mSRV->Release(); mSRV = 0; }
		if ( mGpuTexture ){ mGpuTexture->Release(); mGpuTexture = 0; }
		SAFE_DELETE_ARRAY( mName );
	}

	// Allocate CPU staging arrays for lock/unlock
	void createStagingObject( int w, int h, bool createMipChain ){
		int mipLevels = createMipChain ? mipmapNumber( w, h ) : 1;
		mMips.resize( mipLevels );
		int mw = w, mh = h;
		for ( int i = 0; i < mipLevels; ++i ){
			mMips[ i ].width  = mw;
			mMips[ i ].height = mh;
			mMips[ i ].pixels.assign( mw * mh, 0 );
			mw = ( mw > 1 ) ? mw / 2 : 1;
			mh = ( mh > 1 ) ? mh / 2 : 1;
		}
		mWidth  = w;
		mHeight = h;
		mNeedsUpload = false;
	}

	// Upload CPU staging data to GPU (called lazily before first use)
	void ensureGpuUploaded(){
		if ( !mNeedsUpload && mGpuTexture ) return;
		if ( mMips.empty() && mCompressed.levels.empty() ) return;

		if ( mSRV       ){ mSRV->Release();        mSRV = 0; }
		if ( mGpuTexture){ mGpuTexture->Release(); mGpuTexture = 0; }

		// 送る中身は、圧縮した DDS（ブロックのまま）か、CPU で展開した画素（TGA・古い形の DDS・lock で書いた物）のどちらか。
		const bool compressed = !mCompressed.levels.empty();
		int mipLevels = static_cast< int >( compressed ? mCompressed.levels.size() : mMips.size() );
		vector< D3D11_SUBRESOURCE_DATA > initData( mipLevels );
		for ( int i = 0; i < mipLevels; ++i ){
			if ( compressed ){
				initData[ i ].pSysMem     = mCompressed.levels[ i ].bytes.data();
				initData[ i ].SysMemPitch = mCompressed.levels[ i ].rowPitch;
			}else{
				initData[ i ].pSysMem     = mMips[ i ].pixels.data();
				initData[ i ].SysMemPitch = mMips[ i ].width * 4;
			}
			initData[ i ].SysMemSlicePitch = 0;
		}

		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width          = static_cast< UINT >( compressed ? mCompressed.levels[ 0 ].width : mMips[ 0 ].width );
		desc.Height         = static_cast< UINT >( compressed ? mCompressed.levels[ 0 ].height : mMips[ 0 ].height );
		desc.MipLevels      = static_cast< UINT >( mipLevels );
		desc.ArraySize      = 1;
		// 色の画像は _SRGB にして、シェーダが読むときに GPU がリニアへ直す（線形のフィルタもリニアで掛かる）。
		// 圧縮した DDS は、ファイルに書いてある形式のまま（_SRGB かどうかも texconv が決めた。readCompressedDds で指定と比べてある）。
		if ( compressed ){
			desc.Format = mCompressed.format;
		}else{
			desc.Format = ( mColorSpace == TEXTURE_COLOR_SPACE_SRGB ) ? DXGI_FORMAT_B8G8R8A8_UNORM_SRGB : DXGI_FORMAT_B8G8R8A8_UNORM;
		}
		desc.SampleDesc.Count = 1;
		desc.Usage          = D3D11_USAGE_DEFAULT;
		desc.BindFlags      = D3D11_BIND_SHADER_RESOURCE;

		// 中身は CreateTexture2D の初期データで渡さず、空で作ってから UpdateSubresource で書き込む。
		// Intel Iris Xe（内蔵 GPU）では、初期データ付きで作ったテクスチャをシェーダーで読むと、何かでコピー元に使われるまで
		// 全画素が白・不透明に見えた（フォントの文字が四角く塗りつぶされた）。WARP では起きない。ドライバー側の問題とみて回避する。
		HRESULT hr = mDevice->CreateTexture2D( &desc, nullptr, &mGpuTexture );
		ASSERT( SUCCEEDED( hr ) && "CreateTexture2D failed" );
		for ( int i = 0; i < mipLevels; ++i ){
			mContext->UpdateSubresource( mGpuTexture, static_cast< UINT >( i ), nullptr, initData[ i ].pSysMem, initData[ i ].SysMemPitch, 0 );
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format                    = desc.Format;
		srvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels       = static_cast< UINT >( mipLevels );
		hr = mDevice->CreateShaderResourceView( mGpuTexture, &srvDesc, &mSRV );
		ASSERT( SUCCEEDED( hr ) && "CreateShaderResourceView failed" );

		// 画像ごとの GPU の大きさと、ここまでの合計を出す（1-b の前後で比べる。LinearToneMap_Plan.txt 10 章）。
		// 名前のある画像（ファイルから読んだ物）だけ。lock で書く画像は書き直すたびに来るので数えない。
		if ( !mProfileName.empty() ){
			size_t bytes = 0;
			for ( int i = 0; i < mipLevels; ++i ){
				int rows = compressed ? ( mCompressed.levels[ i ].height + DdsReader::kBlockPixels - 1 ) / DdsReader::kBlockPixels : mMips[ i ].height;
				bytes += static_cast< size_t >( initData[ i ].SysMemPitch ) * rows;
			}
			// 起動中に読んだ画像の合計。消した画像は引かない（読み込みの量を見る数で、今あるメモリの量ではない）。
			static size_t sUploadedBytes = 0;
			sUploadedBytes += bytes;
			const size_t kKiB = 1024;
			GameLib::cout << "Texture: upload " << mProfileName.c_str() << " " << static_cast< int >( desc.Width ) << "x" << static_cast< int >( desc.Height )
				<< ( compressed ? " BC" : " BGRA8" ) << " " << static_cast< int >( bytes / kKiB ) << " KiB (total "
				<< static_cast< int >( sUploadedBytes / ( kKiB * kKiB ) ) << " MiB)" << GameLib::endl;
		}

		mMips.clear();
		mCompressed.levels.clear();
		mCompressed.levels.shrink_to_fit(); // GPU へ送った後は CPU に残さない（4096² の BC7 で 22MB）
		mNeedsUpload = false;
	}

	// Lock / unlock into CPU staging buffer
	void lock( unsigned** b, int* pitch, int mipLevel ){
		ASSERT( mipLevel < static_cast< int >( mMips.size() ) );
		*b     = mMips[ mipLevel ].pixels.data();
		*pitch = mMips[ mipLevel ].width * 4;
	}
	void lock( unsigned** b, int* pitch, int x, int y, int /*w*/, int /*h*/, int mipLevel ){
		ASSERT( mipLevel < static_cast< int >( mMips.size() ) );
		*pitch = mMips[ mipLevel ].width * 4;
		*b = mMips[ mipLevel ].pixels.data() + y * mMips[ mipLevel ].width + x;
	}
	void unlock( int /*mipLevel*/ ){
		mNeedsUpload = true;
	}

	static Extension getExtension( const char* filename ){
		int l = static_cast< int >( strlen( filename ) );
		if ( l < 4 ) return EXT_UNKNOWN;
		Array< char > name( l );
		for ( int i = 0; i < l; ++i ) name[ i ] = static_cast< char >( tolower( filename[ i ] ) );
		if ( name[l-4]=='.' && name[l-3]=='t' && name[l-2]=='g' && name[l-1]=='a' ) return EXT_TGA;
		if ( name[l-4]=='.' && name[l-3]=='d' && name[l-2]=='d' && name[l-1]=='s' ) return EXT_DDS;
		return EXT_UNKNOWN;
	}

	// ---- file writing helpers (same logic as original) ----
	static void writeTga( const char* filename, int width, int height, int pitch, const unsigned* src, bool alpha ){
		static const char* footerString = "TRUEVISION-XFILE.";
		const int footerLen = static_cast< int >( strlen( footerString ) );
		char pixelSize = alpha ? 4 : 3;
		int fileSize = width * height * pixelSize + 18 + 8 + footerLen + 1;
		char* data = NEW char[ fileSize ];
		for ( int i = 0; i < 18; ++i ) data[ i ] = 0;
		unsigned char* dst = reinterpret_cast< unsigned char* >( data );
		dst[0x2] = 2;
		dst[0xc] = static_cast< char >( width & 0xff );
		dst[0xd] = static_cast< char >( ( width & 0xff00 ) >> 8 );
		dst[0xe] = static_cast< char >( height & 0xff );
		dst[0xf] = static_cast< char >( ( height & 0xff00 ) >> 8 );
		dst[0x10] = pixelSize * 8;
		dst[0x11] = 0x20 + ( alpha ? 8 : 0 );
		dst += 0x12;
		int pitchInPixels = pitch / 4;
		if ( alpha ){
			for ( int y = 0; y < height; ++y ){
				for ( int x = 0; x < width; ++x ){
					unsigned char* d = dst + 4 * x;
					unsigned s = src[ x ];
					d[0] = static_cast< unsigned char >( ( s ) & 0xff );
					d[1] = static_cast< unsigned char >( ( s >> 8 ) & 0xff );
					d[2] = static_cast< unsigned char >( ( s >> 16 ) & 0xff );
					d[3] = static_cast< unsigned char >( ( s >> 24 ) & 0xff );
				}
				dst += width * 4;
				src += pitchInPixels;
			}
		}else{
			for ( int y = 0; y < height; ++y ){
				for ( int x = 0; x < width; ++x ){
					unsigned char* d = dst + 3 * x;
					unsigned s = src[ x ];
					d[0] = static_cast< unsigned char >( ( s ) & 0xff );
					d[1] = static_cast< unsigned char >( ( s >> 8 ) & 0xff );
					d[2] = static_cast< unsigned char >( ( s >> 16 ) & 0xff );
				}
				dst += width * 3;
				src += pitchInPixels;
			}
		}
		for ( int i = 0; i < 8; ++i ) dst[ i ] = 0;
		dst += 8;
		for ( int i = 0; i < footerLen; ++i ) dst[ i ] = footerString[ i ];
		dst[ footerLen ] = '\0';
		FileIO::Manager().write( filename, data, fileSize );
		SAFE_DELETE_ARRAY( data );
	}
	void writeTga( const char* filename ){
		// Readback from GPU to CPU staging
		ID3D11Texture2D* staging = createStagingReadback();
		if ( !staging ) return;
		D3D11_MAPPED_SUBRESOURCE ms = {};
		mContext->Map( staging, 0, D3D11_MAP_READ, 0, &ms );
		writeTga( filename, mWidth, mHeight, ms.RowPitch,
			static_cast< const unsigned* >( ms.pData ), true );
		mContext->Unmap( staging, 0 );
		staging->Release();
	}
	static void writeDds( const char* filename, int width, int height, int pitch, const unsigned* src, bool alpha ){
		int fileSize = width * height * 4 + 128;
		char* data = NEW char[ fileSize ];
		for ( int i = 0; i < 128; ++i ) data[ i ] = 0;
		unsigned char* dst = reinterpret_cast< unsigned char* >( data );
		dst[0]='D'; dst[1]='D'; dst[2]='S'; dst[3]=' ';
		dst[0x4]=0x7c; dst[0x8]=0x7; dst[0x9]=0x10;
		dst[0xc]=static_cast<char>(height&0xff); dst[0xd]=static_cast<char>((height>>8)&0xff);
		dst[0x10]=static_cast<char>(width&0xff); dst[0x11]=static_cast<char>((width>>8)&0xff);
		dst[0x4c]=0x20; dst[0x50]=0x41; dst[0x58]=0x20;
		dst[0x5e]=0xff; dst[0x61]=0xff; dst[0x64]=0xff; dst[0x6b]=0xff;
		dst[0x6c]=0x2; dst[0x6d]=0x10;
		dst += 128;
		int pitchInPixels = pitch / 4;
		for ( int y = 0; y < height; ++y ){
			for ( int x = 0; x < width; ++x ){
				unsigned char* d = dst + 4 * x;
				unsigned s = src[ x ];
				d[0]=static_cast<unsigned char>((s)&0xff);
				d[1]=static_cast<unsigned char>((s>>8)&0xff);
				d[2]=static_cast<unsigned char>((s>>16)&0xff);
				d[3]=alpha ? static_cast<unsigned char>((s>>24)&0xff) : 0xff;
			}
			dst += width * 4;
			src += pitchInPixels;
		}
		FileIO::Manager().write( filename, data, fileSize );
		SAFE_DELETE_ARRAY( data );
	}
	void writeDds( const char* filename ){
		ID3D11Texture2D* staging = createStagingReadback();
		if ( !staging ) return;
		D3D11_MAPPED_SUBRESOURCE ms = {};
		mContext->Map( staging, 0, D3D11_MAP_READ, 0, &ms );
		writeDds( filename, mWidth, mHeight, ms.RowPitch,
			static_cast< const unsigned* >( ms.pData ), true );
		mContext->Unmap( staging, 0 );
		staging->Release();
	}
	void write( const char* filename ){
		Extension ext = getExtension( filename );
		if ( ext == EXT_DDS ) writeDds( filename );
		else writeTga( filename );
	}
	static void write( const char* filename, int width, int height, int pitch, const unsigned* data ){
		Extension ext = getExtension( filename );
		if ( ext == EXT_DDS ) writeDds( filename, width, height, pitch, data, false );
		else writeTga( filename, width, height, pitch, data, false );
	}

	bool isError() const { return mIsError; }
	bool isReady(){
		if ( mFile ){
			if ( mFile.isFinished() ){
				if ( mFile.isError() ){
					mIsError = true;
				}else{
					// 画像を展開して GPU へ送るのは、ここ（最初に isReady を呼んだ描画の中）。
					// ゲームが始まった後に初めて使うテクスチャだと、ここがプレイ中のひっかかりになる（計画書 6-6 ③）。
					Profiling::addLoadWait( mProfileName.c_str(), mRequestedAt );
					LOAD_SCOPE( "texture", mProfileName.c_str() );
					if ( mExtension == EXT_DDS ) readDDS( mFile.data(), mFile.size() );
					else if ( mExtension == EXT_TGA ) readTGA( mFile.data(), mFile.size() );
					else ASSERT( 0 );
					mFile.release();
				}
				return true;
			}else{
				return false;
			}
		}else{
			return true;
		}
	}

	int width()          const { return mWidth; }
	int height()         const { return mHeight; }
	int originalWidth()  const { return mOriginalWidth; }
	int originalHeight() const { return mOriginalHeight; }
	const char* name()   const { return mName; }

	static int powerOfTwo( int a ){
		ASSERT( a < 0x40000000 );
		int r = 1;
		while ( r < a ) r *= 2;
		return r;
	}
	static int mipmapNumber( int w, int h ){
		int levels = 0;
		while ( w > 0 && h > 0 ){ w >>= 1; h >>= 1; ++levels; }
		return levels;
	}
	static unsigned getUnsigned( const char* data ){
		const unsigned char* up = reinterpret_cast< const unsigned char* >( data );
		return up[0] | (up[1]<<8) | (up[2]<<16) | (up[3]<<24);
	}
	static unsigned short getUnsignedShort( const char* data ){
		const unsigned char* up = reinterpret_cast< const unsigned char* >( data );
		return static_cast< unsigned short >( up[0] | (up[1]<<8) );
	}
	static unsigned char getUnsignedChar( const char* data ){
		return reinterpret_cast< const unsigned char* >( data )[0];
	}

	void readTGA( const char* data, int size ){
		int sw = getUnsignedShort( data + 12 );
		int sh = getUnsignedShort( data + 14 );
		mOriginalWidth  = sw;
		mOriginalHeight = sh;
		int pixelSize = getUnsignedChar( data + 16 ) / 8;
		int paletteNumber = 0, paletteDepth = 0;
		if ( ( data[2] & 7 ) == 1 ){
			paletteNumber = getUnsignedShort( data + 5 );
			paletteDepth  = getUnsignedChar( data + 7 ) / 8;
			ASSERT( paletteDepth == 3 || paletteDepth == 4 );
		}
		const unsigned char* src = reinterpret_cast< const unsigned char* >( data );
		const unsigned char* pSrc = src + 18;
		src += 18 + paletteNumber * paletteDepth;
		Array< unsigned char > uncompressed;
		if ( data[2] & 0x8 ){
			const unsigned char* s = src;
			const unsigned char* sEnd = reinterpret_cast< const unsigned char* >( data ) + size;
			uncompressed.setSize( pixelSize * sw * sh );
			int n = 0;
			while ( s < sEnd ){
				int l = ( *s & 0x7f ) + 1;
				bool compressed = ( ( *s & 0x80 ) != 0 );
				if ( n + l > sw * sh ){ mIsError = true; return; }
				int readSize = pixelSize * ( compressed ? 1 : l );
				if ( s + readSize > sEnd ){ mIsError = true; return; }
				++s;
				if ( compressed ){
					for ( int i = 0; i < l; ++i ) for ( int j = 0; j < pixelSize; ++j ) uncompressed[ n*pixelSize+j ] = s[j];
					n += l; s += pixelSize;
				}else{
					for ( int i = 0; i < l; ++i ) for ( int j = 0; j < pixelSize; ++j ) uncompressed[ n*pixelSize+j ] = *s++;
					n += l;
				}
				if ( n == sw * sh ) break;
			}
			src = &uncompressed[ 0 ];
		}else if ( size < ( sh * sw * pixelSize + 18 + paletteNumber + paletteDepth ) ){
			mIsError = true; return;
		}
		int yBegin, yEnd, dy, dSrc;
		if ( ( data[0x11] & 0x20 ) == 0 ){
			yBegin = sh - 1; yEnd = -1; dy = -1; dSrc = -sw * pixelSize;
			src += sw * pixelSize * ( sh - 1 );
		}else{
			yBegin = 0; yEnd = sh; dy = 1; dSrc = sw * pixelSize;
		}
		if ( ( data[2] & 0x7 ) == 0 ){ mIsError = true; return; }
		int dh = powerOfTwo( sh );
		int dw = powerOfTwo( sw );
		createStagingObject( dw, dh, true );

		unsigned* surface;
		int pitch;
		lock( &surface, &pitch, 0 );
		int pitchPx = pitch / 4;
		unsigned* dst = surface;

		if ( pixelSize == 4 ){
			for ( int y = yBegin; y != yEnd; y += dy ){
				for ( int x = 0; x < sw; ++x ){
					dst[x] = src[x*4+0] | (src[x*4+1]<<8) | (src[x*4+2]<<16) | (src[x*4+3]<<24);
				}
				for ( int x = sw; x < pitchPx; ++x ) dst[x] = 0;
				dst += pitchPx; src += dSrc;
			}
		}else if ( pixelSize == 3 ){
			for ( int y = yBegin; y != yEnd; y += dy ){
				for ( int x = 0; x < sw; ++x ){
					dst[x] = src[x*3+0] | (src[x*3+1]<<8) | (src[x*3+2]<<16) | 0xff000000u;
				}
				for ( int x = sw; x < pitchPx; ++x ) dst[x] = 0;
				dst += pitchPx; src += dSrc;
			}
		}else if ( pixelSize == 2 ){
			for ( int y = yBegin; y != yEnd; y += dy ){
				for ( int x = 0; x < sw; ++x ){
					unsigned t = src[x*2];
					dst[x] = t | (t<<8) | (t<<16) | (static_cast<unsigned>(src[x*2+1])<<24);
				}
				for ( int x = sw; x < pitchPx; ++x ) dst[x] = 0;
				dst += pitchPx; src += dSrc;
			}
		}else if ( pixelSize == 1 ){
			if ( paletteNumber > 0 ){
				Array< unsigned > palette( paletteNumber );
				if ( paletteDepth == 3 ){
					for ( int i = 0; i < paletteNumber; ++i )
						palette[i] = pSrc[i*3] | (pSrc[i*3+1]<<8) | (pSrc[i*3+2]<<16) | 0xff000000u;
				}else{
					for ( int i = 0; i < paletteNumber; ++i )
						palette[i] = pSrc[i*4] | (pSrc[i*4+1]<<8) | (pSrc[i*4+2]<<16) | (pSrc[i*4+3]<<24);
				}
				for ( int y = yBegin; y != yEnd; y += dy ){
					for ( int x = 0; x < sw; ++x ) dst[x] = palette[ src[x] ];
					for ( int x = sw; x < pitchPx; ++x ) dst[x] = 0;
					dst += pitchPx; src += dSrc;
				}
			}else{
				for ( int y = yBegin; y != yEnd; y += dy ){
					for ( int x = 0; x < sw; ++x ){ dst[x] = 0x00ffffff | (static_cast<unsigned>(src[x])<<24); }
					for ( int x = sw; x < pitchPx; ++x ) dst[x] = 0;
					dst += pitchPx; src += dSrc;
				}
			}
		}
		for ( int y = sh; y < dh; ++y ){
			for ( int x = 0; x < pitchPx; ++x ) dst[x] = 0;
			dst += pitchPx;
		}
		createMipmapChain( surface, pitchPx, dw, dh, 1 );
		unlock( 0 );
	}

	// tools/texture_to_dds.ps1 で作った BC 圧縮の DDS を、展開せずにブロックのまま持つ（ミップもファイルの物を使う）。
	// 読めない形式・色の指定との食い違いは、黙って変な絵を出すより止めて知らせる（LinearToneMap_Plan.txt 8-4）。
	void readCompressedDds( const char* data, int size ){
		std::string error;
		if ( !DdsReader::read( data, size, &mCompressed, &error ) ){
			GameLib::cout << "Texture: cannot read " << mProfileName.c_str() << " : " << error.c_str() << GameLib::endl;
			ASSERT( 0 && "Texture : unsupported DDS. See the output window." );
			mIsError = true;
			return;
		}
		bool srgbFile = DdsReader::isSrgb( mCompressed.format );
		bool srgbRequested = ( mColorSpace == TEXTURE_COLOR_SPACE_SRGB );
		if ( srgbFile != srgbRequested ){
			GameLib::cout << "Texture: " << mProfileName.c_str() << ( srgbFile ? " is sRGB but requested as non-color" : " is not sRGB but requested as color" ) << GameLib::endl;
			ASSERT( 0 && "Texture : the DDS format and the requested color space differ. See the output window." );
		}
		mWidth = mOriginalWidth = mCompressed.levels[ 0 ].width;
		mHeight = mOriginalHeight = mCompressed.levels[ 0 ].height;
		mNeedsUpload = true;
	}

	void readDDS( const char* data, int size ){
		if ( DdsReader::hasDx10Header( data, size ) ){
			readCompressedDds( data, size );
			return;
		}
		// ここから下は古い形の、圧縮していない DDS（Texture::write が書く物）。
		int sh = getUnsigned( data + 12 );
		int sw = getUnsigned( data + 16 );
		mOriginalWidth  = sw;
		mOriginalHeight = sh;
		int pixelSize = getUnsigned( data + 88 ) / 8;
		if ( size < ( sh * sw * pixelSize + 128 ) ){ mIsError = true; return; }
		int dh = powerOfTwo( sh );
		int dw = powerOfTwo( sw );
		createStagingObject( dw, dh, true );

		unsigned* surface;
		int pitch;
		lock( &surface, &pitch, 0 );
		int pitchPx = pitch / 4;
		unsigned* dst = surface;
		const unsigned char* src = reinterpret_cast< const unsigned char* >( data ) + 128;

		if ( pixelSize == 4 ){
			unsigned alphaMask = getUnsigned( data + 0x68 );
			unsigned alphaOr = ( alphaMask == 0 ) ? 0xff000000u : 0;
			for ( int y = 0; y < sh; ++y ){
				for ( int x = 0; x < sw; ++x ){
					dst[x] = (src[x*4+0]) | (src[x*4+1]<<8) | (src[x*4+2]<<16) | (src[x*4+3]<<24);
					dst[x] |= alphaOr;
				}
				for ( int x = sw; x < pitchPx; ++x ) dst[x] = 0;
				dst += pitchPx;
				src += ( sw * pixelSize + 3 ) & ~3;
			}
		}else if ( pixelSize == 3 ){
			for ( int y = 0; y < sh; ++y ){
				for ( int x = 0; x < sw; ++x )
					dst[x] = src[x*3] | (src[x*3+1]<<8) | (src[x*3+2]<<16) | 0xff000000u;
				for ( int x = sw; x < pitchPx; ++x ) dst[x] = 0;
				dst += pitchPx;
				src += ( sw * pixelSize + 3 ) & ~3;
			}
		}else if ( pixelSize == 1 ){
			for ( int y = 0; y < sh; ++y ){
				for ( int x = 0; x < sw; ++x ) dst[x] = 0x00ffffff | (static_cast<unsigned>(src[x])<<24);
				for ( int x = sw; x < pitchPx; ++x ) dst[x] = 0;
				dst += pitchPx;
				src += ( sw * pixelSize + 3 ) & ~3;
			}
		}
		for ( int y = sh; y < dh; ++y ){
			for ( int x = 0; x < pitchPx; ++x ) dst[x] = 0;
			dst += pitchPx;
		}
		createMipmapChain( surface, pitchPx, dw, dh, 1 );
		unlock( 0 );
	}

	// ミップを作るときの画素の並び（BGRA が 8bit ずつ。アルファは一番上の 8bit）と、縮める前の 4 画素。
	static const int kChannelBits = 8;
	static const int kChannels = 4;
	static const int kAlphaShift = 24;
	static const unsigned kChannelMask = 0xff;
	static const int kMipSourcePixels = 4;
	// リニアの数は 16bit（0〜65535）で持つ。8bit の色 256 段を 4 つ足しても溢れず、暗い所の差も潰れない。
	static const int kSrgbLevels = 256;
	static const int kLinearBits = 16;
	static constexpr float kLinearMax = static_cast< float >( ( 1 << kLinearBits ) - 1 );
	// リニア → 色の表の段数（16bit を下の 4bit 捨てて引く）。暗い所でも 1 段のずれが 8bit の色の 1 より小さくなる細かさ。
	static const int kLinearToSrgbBits = 12;
	static const int kLinearToSrgbShift = kLinearBits - kLinearToSrgbBits;

	// 色の画像のミップをリニアで平均するための表。pow を画素ごとに呼ぶと、4096² の読み込みが目に見えて遅くなるため。
	struct SrgbTables{
		unsigned short toLinear[ kSrgbLevels ];             // 8bit の色 → 16bit のリニア
		unsigned char  toSrgb[ 1 << kLinearToSrgbBits ];    // 16bit のリニアの上 12bit → 8bit の色
		SrgbTables(){
			const float colorMax = static_cast< float >( kSrgbLevels - 1 );
			for ( int i = 0; i < kSrgbLevels; ++i ){
				float linear = ColorSpace::srgbToLinear( i / colorMax );
				toLinear[ i ] = static_cast< unsigned short >( linear * kLinearMax + 0.5f );
			}
			const int steps = 1 << kLinearToSrgbBits;
			for ( int i = 0; i < steps; ++i ){
				// 段の真ん中の値で引く（切り捨てで引くと、全体が暗い側へ寄る）。
				float linear = ( i + 0.5f ) / steps;
				toSrgb[ i ] = static_cast< unsigned char >( ColorSpace::linearToSrgb( linear ) * colorMax + 0.5f );
			}
		}
	};
	static const SrgbTables& srgbTables(){
		static const SrgbTables sTables; // 初めて色の画像のミップを作るときに 1 回だけ作る
		return sTables;
	}
	// 4 画素の、shift の位置の 8bit を平均する。asSrgb なら sRGB の色としてリニアで平均する
	// （画像の数のまま平均すると、遠くの縮んだ絵が暗い側へ寄るため。LinearToneMap_Plan.txt 4 章）。
	static unsigned averageChannel( const unsigned* quad, int shift, bool asSrgb ){
		const unsigned half = kMipSourcePixels / 2; // 四捨五入
		unsigned sum = 0;
		if ( !asSrgb ){
			for ( int i = 0; i < kMipSourcePixels; ++i ) sum += ( quad[ i ] >> shift ) & kChannelMask;
			return ( sum + half ) / kMipSourcePixels;
		}
		const SrgbTables& tables = srgbTables();
		for ( int i = 0; i < kMipSourcePixels; ++i ) sum += tables.toLinear[ ( quad[ i ] >> shift ) & kChannelMask ];
		unsigned linear = ( sum + half ) / kMipSourcePixels;
		return tables.toSrgb[ linear >> kLinearToSrgbShift ];
	}

	void createMipmapChain( const unsigned* src, int srcPitch, int sw, int sh, int level ){
		int dw = sw >> 1;
		int dh = sh >> 1;
		if ( dw == 0 || dh == 0 ) return;
		unsigned* dstSurface;
		int dstPitch;
		lock( &dstSurface, &dstPitch, level );
		unsigned* dst = dstSurface;
		int dstPitchPx = dstPitch / 4;
		ASSERT( dstPitchPx >= dw );
		const bool srgb = ( mColorSpace == TEXTURE_COLOR_SPACE_SRGB );
		for ( int y = 0; y < dh; ++y ){
			for ( int x = 0; x < dw; ++x ){
				const unsigned quad[ kMipSourcePixels ] = {
					src[ 2*x ], src[ 2*x+1 ], src[ 2*x+srcPitch ], src[ 2*x+srcPitch+1 ] };
				unsigned averaged = 0;
				for ( int shift = 0; shift < kChannelBits * kChannels; shift += kChannelBits ){
					// アルファは色ではない（覆う割合）ので、色の画像でもそのまま平均する。
					bool linear = srgb && ( shift != kAlphaShift );
					averaged |= averageChannel( quad, shift, linear ) << shift;
				}
				dst[x] = averaged;
			}
			dst += dstPitchPx;
			src += srcPitch * 2;
		}
		unlock( level );
		createMipmapChain( dstSurface, dstPitchPx, dw, dh, level + 1 );
	}

	ID3D11Texture2D* createStagingReadback(){
		ensureGpuUploaded();
		if ( !mGpuTexture ) return nullptr;
		D3D11_TEXTURE2D_DESC desc;
		mGpuTexture->GetDesc( &desc );
		// 圧縮した絵は 1 画素 4 バイトとして読み戻せない（write はデバッグ用なので、TGA から読んだ絵だけ書ければよい）。
		if ( DdsReader::blockBytes( desc.Format ) > 0 ) return nullptr;
		D3D11_TEXTURE2D_DESC sd = desc;
		sd.Usage          = D3D11_USAGE_STAGING;
		sd.BindFlags      = 0;
		sd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		sd.MipLevels      = 1;
		sd.MiscFlags      = 0;
		ID3D11Texture2D* staging = nullptr;
		HRESULT hr = mDevice->CreateTexture2D( &sd, nullptr, &staging );
		if ( FAILED( hr ) ) return nullptr;
		mContext->CopySubresourceRegion( staging, 0, 0, 0, 0, mGpuTexture, 0, nullptr );
		return staging;
	}

	ID3D11Texture2D*          mGpuTexture;
	ID3D11ShaderResourceView* mSRV;
	ID3D11Device*             mDevice;
	ID3D11DeviceContext*      mContext;
	vector< MipLevel >        mMips;
	FileIO::InFile            mFile;
	bool                      mIsError;
	int                       mWidth;
	int                       mHeight;
	int                       mOriginalWidth;
	int                       mOriginalHeight;
	bool                      mNeedsUpload;
	Extension                 mExtension;
	char*                     mName;
	// ロードの計測用。読み込みを頼んだ時刻と、ファイル名（ファイルから読むときだけ）。
	long long                 mRequestedAt;
	string                    mProfileName;
	TextureColorSpace         mColorSpace; // 色の画像か（ファイル名から作るときだけ指定できる。ほかは LINEAR）
	// 圧縮した DDS の中身（GPU へ送るまで。送った後は空）。levels が空なら圧縮していない（mMips を使う）。
	DdsReader::Image          mCompressed;
};

} //namespace Graphics
} //namespace GameLib

#endif
