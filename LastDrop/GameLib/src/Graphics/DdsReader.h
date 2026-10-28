#ifndef INCLUDED_GAMELIB_GRAPHICS_DDSREADER_H
#define INCLUDED_GAMELIB_GRAPHICS_DDSREADER_H

#include <dxgiformat.h>
#include <cstring>
#include <string>
#include <vector>

// ブロック圧縮（BC1 / BC5 / BC7）の DDS を、GPU へそのまま送れる形に読み分ける（ファイルを読むだけ。D3D は触らない）。
// tools/texture_to_dds.ps1（texconv -dx10）が書く「DX10 の追加の頭」付きの DDS だけを読む。
// 仕様は SummaryPlan/個別仕様/LinearToneMap_Plan.txt の 8-4。
// 古い形の DDS（追加の頭が無い・圧縮していない。Texture::write が書く物）は読まない（TextureImpl の readDDS が今までどおり読む）。
//
// ファイルの並び（https://learn.microsoft.com/ja-jp/windows/win32/direct3ddds/dx-graphics-dds-pguide）
//   "DDS "（4）＋ DDS_HEADER（124）＋ DDS_HEADER_DXT10（20）＋ ミップ 0 → 1 → … の中身
namespace GameLib{
namespace Graphics{
namespace DdsReader{

// ミップ 1 段ぶん。bytes は 4×4 画素のブロックを左上から行ごとに並べた物。
struct Level{
	int width;
	int height;
	int rowPitch; // 1 行（ブロック 1 行＝4 画素ぶんの高さ）のバイト数。UpdateSubresource に渡す
	std::vector< char > bytes;
};

struct Image{
	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
	std::vector< Level > levels; // [0] が一番大きい
};

// 頭の中の位置（バイト）と、決まった値。
const int kMagicSize = 4;
const int kHeaderSize = 124;
const int kDx10HeaderSize = 20;
const int kDataOffset = kMagicSize + kHeaderSize + kDx10HeaderSize;
const int kHeightOffset = 12;
const int kWidthOffset = 16;
const int kMipCountOffset = 28;
const int kFlagsOffset = 8;
const int kFourCcOffset = 84;
const int kDxgiFormatOffset = kMagicSize + kHeaderSize; // DX10 の頭の最初の 4 バイト
const unsigned kMipCountFlag = 0x20000; // DDSD_MIPMAPCOUNT。無ければミップは 1 段
const int kBlockPixels = 4;             // ブロックの 1 辺（画素）

inline unsigned readUnsigned( const char* p ){
	const unsigned char* u = reinterpret_cast< const unsigned char* >( p );
	return u[ 0 ] | ( u[ 1 ] << 8 ) | ( u[ 2 ] << 16 ) | ( static_cast< unsigned >( u[ 3 ] ) << 24 );
}

// DX10 の追加の頭が付いた DDS か（読み分けの入口。"DDS " と FourCC "DX10" を見るだけ）。
inline bool hasDx10Header( const char* data, int size ){
	return size >= kDataOffset && std::memcmp( data, "DDS ", kMagicSize ) == 0 &&
		std::memcmp( data + kFourCcOffset, "DX10", 4 ) == 0;
}

// format の 1 ブロック（4×4 画素）のバイト数。対応していない形式は 0。
inline int blockBytes( DXGI_FORMAT format ){
	switch ( format ){
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC1_UNORM_SRGB:
			return 8;
		case DXGI_FORMAT_BC5_UNORM:
		case DXGI_FORMAT_BC7_UNORM:
		case DXGI_FORMAT_BC7_UNORM_SRGB:
			return 16;
		default:
			return 0;
	}
}

// GPU が読むときにリニアへ直す（_SRGB の）形式か。
inline bool isSrgb( DXGI_FORMAT format ){
	return format == DXGI_FORMAT_BC1_UNORM_SRGB || format == DXGI_FORMAT_BC7_UNORM_SRGB;
}

// data を読んで image に入れる。読めなかったら false と、その理由を error に入れる（止めるかは呼んだ側が決める）。
inline bool read( const char* data, int size, Image* image, std::string* error ){
	if ( !hasDx10Header( data, size ) ){
		*error = "not a DDS with the DX10 header (convert with tools/texture_to_dds.ps1)";
		return false;
	}
	image->format = static_cast< DXGI_FORMAT >( readUnsigned( data + kDxgiFormatOffset ) );
	const int bytesPerBlock = blockBytes( image->format );
	if ( bytesPerBlock == 0 ){
		*error = "unsupported DXGI format " + std::to_string( static_cast< int >( image->format ) ) + " (BC1 / BC5 / BC7 only)";
		return false;
	}
	int width = static_cast< int >( readUnsigned( data + kWidthOffset ) );
	int height = static_cast< int >( readUnsigned( data + kHeightOffset ) );
	bool hasMipCount = ( readUnsigned( data + kFlagsOffset ) & kMipCountFlag ) != 0;
	int levelCount = hasMipCount ? static_cast< int >( readUnsigned( data + kMipCountOffset ) ) : 1;
	if ( width <= 0 || height <= 0 || levelCount <= 0 ){
		*error = "broken header";
		return false;
	}
	image->levels.clear();
	image->levels.resize( levelCount );
	const char* p = data + kDataOffset;
	const char* end = data + size;
	for ( int i = 0; i < levelCount; ++i ){
		Level& level = image->levels[ i ];
		level.width = width;
		level.height = height;
		// 4 画素に満たない段（2×2・1×1）も、ブロック 1 つぶんを持つ。
		int blocksWide = ( width + kBlockPixels - 1 ) / kBlockPixels;
		int blocksHigh = ( height + kBlockPixels - 1 ) / kBlockPixels;
		level.rowPitch = blocksWide * bytesPerBlock;
		size_t levelSize = static_cast< size_t >( level.rowPitch ) * blocksHigh;
		if ( static_cast< size_t >( end - p ) < levelSize ){
			*error = "file is shorter than its mip levels";
			return false;
		}
		level.bytes.assign( p, p + levelSize );
		p += levelSize;
		width = ( width > 1 ) ? width / 2 : 1;
		height = ( height > 1 ) ? height / 2 : 1;
	}
	return true;
}

} //namespace DdsReader
} //namespace Graphics
} //namespace GameLib

#endif
