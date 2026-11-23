#include "GameSrc/Map/TownBakeFile.h"

#include "GameLib/FileIO/Manager.h"
#include "GameLib/FileIO/ReadWhole.h"

#include <cstring>
#ifdef ZF_EDITOR
#include <filesystem>
#include <fstream>
#endif

using namespace GameLib;

namespace{
	const char kMagic[] = { 'T', 'W', 'N', 'B' };	// ファイルの始めの 4 文字（終わりの 0 は持たない）
	const std::uint32_t kFormatVersion = 1;			// 頭の並びを変えたら上げる
	const char* const kNavMeshPathHead = "Resource/Map/Bake/NavMesh_";
	const char* const kExtension = ".bin";
#ifdef ZF_EDITOR
	const char* const kTemporarySuffix = ".tmp";
#endif

	// ファイルの頭。Windows の x64 だけで読み書きするので、数はメモリの並びのまま（リトルエンディアン）書く
	struct Header{
		char magic[ sizeof( kMagic ) ];
		std::uint32_t version;
		std::uint64_t fingerprint;
		std::uint64_t payloadSize;	// 頭の後に続く中身のバイト数
	};
	static_assert( sizeof( Header ) == sizeof( kMagic ) + sizeof( std::uint32_t ) + sizeof( std::uint64_t ) * 2,
		"TownBakeFile : Header has padding." );	// 隙間があると、書いた頭に決まらないバイトが混ざる

#ifdef ZF_EDITOR
	bool writeFile( const std::string& path, const Header& header, const std::vector< unsigned char >& payload, std::string* error ) {	// path へ頭と payload をそのまま書く。開けない・書けなければ false と、その理由を error へ入れる
		std::ofstream ofs( path, std::ios::binary );
		if ( !ofs ) {
			*error = "Cannot open " + path;
			return false;
		}
		ofs.write( reinterpret_cast< const char* >( &header ), sizeof( header ) );
		ofs.write( reinterpret_cast< const char* >( payload.data() ), static_cast< std::streamsize >( payload.size() ) );
		if ( !ofs ) {
			*error = "Cannot write " + path;
			return false;
		}
		return true;
	}
#endif
}

namespace TownBakeFile{

const char* const kVisibilityPath = "Resource/Map/Bake/Visibility.bin";

std::string navMeshPath( int escapeCarSpot ) {
	return std::string( kNavMeshPathHead ) + std::to_string( escapeCarSpot ) + kExtension;
}

bool read( const char* path, std::uint64_t fingerprint, std::vector< unsigned char >* payload ) {
	payload->clear();
	if ( !FileIO::Manager().exists( path ) ) {	// FileIO は無いファイルを読もうとすると止まるので、先にあるかを見る。無いのは普通（まだ書き出していない）
		return false;
	}
	std::vector< char > data;
	if ( !FileIO::readWhole( path, &data ) || data.size() < sizeof( Header ) ) {
		return false;
	}
	Header header;
	std::memcpy( &header, data.data(), sizeof( Header ) );
	const bool matches =
		std::memcmp( header.magic, kMagic, sizeof( kMagic ) ) == 0 &&
		header.version == kFormatVersion &&
		header.fingerprint == fingerprint &&
		header.payloadSize == data.size() - sizeof( Header );	// 途中で切れた・後ろに余計な物が付いたファイルは読まない
	if ( !matches ) {
		return false;
	}
	payload->assign( data.begin() + sizeof( Header ), data.end() );
	return true;
}

#ifdef ZF_EDITOR
bool write( const char* path, std::uint64_t fingerprint, const std::vector< unsigned char >& payload, std::string* error ) {
	Header header;
	std::memcpy( header.magic, kMagic, sizeof( kMagic ) );
	header.version = kFormatVersion;
	header.fingerprint = fingerprint;
	header.payloadSize = payload.size();

	std::error_code code;
	std::filesystem::path target( path );
	if ( target.has_parent_path() ) {
		std::filesystem::create_directories( target.parent_path(), code );
	}
	const std::string temporary = std::string( path ) + kTemporarySuffix;
	if ( writeFile( temporary, header, payload, error ) ) {
		std::filesystem::rename( temporary, target, code );	// MSVC の rename は、置き換え先があっても上書きする
		if ( !code ) {
			return true;
		}
		*error = std::string( "Cannot replace " ) + path + " (" + code.message() + ")";
	}
	std::filesystem::remove( temporary, code );	// 書きかけの一時ファイルを残さない（make_pak が Resource/ の下を全部 .pak へ入れるため）。消せなくても、返すのは書けなかった理由
	return false;
}
#endif

} //namespace TownBakeFile
