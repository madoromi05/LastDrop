#include "GameSrc/Audio/AudioFileSystem.h"

#include "GameLib/FileIO/ReadWhole.h"

#include <algorithm>
#include <cstring>
#include <memory>
#include <vector>

namespace{
	// 開いた 1 つのファイル。中身は開くときに全部読んである
	struct OpenFile{
		std::vector< char > data;
		size_t cursor;
	};

	OpenFile* fileOf( ma_vfs_file file ){
		return static_cast< OpenFile* >( file );
	}

	ma_result onOpen( ma_vfs*, const char* path, ma_uint32 openMode, ma_vfs_file* out ){
		*out = nullptr;
		if ( openMode & MA_OPEN_MODE_WRITE ){
			return MA_NOT_IMPLEMENTED;	// 音を書くことはない
		}
		std::unique_ptr< OpenFile > file( new OpenFile() );
		file->cursor = 0;
		if ( !GameLib::FileIO::readWhole( path, &file->data ) ){
			return MA_DOES_NOT_EXIST;	// AudioEngine が「sound not found」を出して先へ進む
		}
		*out = file.release();			// onClose で消す
		return MA_SUCCESS;
	}

	ma_result onOpenW( ma_vfs*, const wchar_t*, ma_uint32, ma_vfs_file* out ){
		*out = nullptr;
		return MA_NOT_IMPLEMENTED;		// 音のパスは char で渡している
	}

	ma_result onClose( ma_vfs*, ma_vfs_file file ){
		std::unique_ptr< OpenFile > owned( fileOf( file ) );
		return MA_SUCCESS;
	}

	ma_result onRead( ma_vfs*, ma_vfs_file file, void* dst, size_t sizeInBytes, size_t* bytesRead ){
		OpenFile* f = fileOf( file );
		size_t count = std::min( sizeInBytes, f->data.size() - f->cursor );
		if ( count > 0 ){
			std::memcpy( dst, f->data.data() + f->cursor, count );
		}
		f->cursor += count;
		if ( bytesRead ){
			*bytesRead = count;
		}
		return ( count == 0 && sizeInBytes > 0 ) ? MA_AT_END : MA_SUCCESS;
	}

	ma_result onWrite( ma_vfs*, ma_vfs_file, const void*, size_t, size_t* bytesWritten ){
		if ( bytesWritten ){
			*bytesWritten = 0;
		}
		return MA_NOT_IMPLEMENTED;
	}

	ma_result onSeek( ma_vfs*, ma_vfs_file file, ma_int64 offset, ma_seek_origin origin ){
		OpenFile* f = fileOf( file );
		ma_int64 base = 0;
		if ( origin == ma_seek_origin_current ){
			base = static_cast< ma_int64 >( f->cursor );
		}else if ( origin == ma_seek_origin_end ){
			base = static_cast< ma_int64 >( f->data.size() );
		}
		ma_int64 target = base + offset;
		if ( target < 0 || target > static_cast< ma_int64 >( f->data.size() ) ){
			return MA_INVALID_ARGS;
		}
		f->cursor = static_cast< size_t >( target );
		return MA_SUCCESS;
	}

	ma_result onTell( ma_vfs*, ma_vfs_file file, ma_int64* cursor ){
		*cursor = static_cast< ma_int64 >( fileOf( file )->cursor );
		return MA_SUCCESS;
	}

	ma_result onInfo( ma_vfs*, ma_vfs_file file, ma_file_info* info ){
		info->sizeInBytes = static_cast< ma_uint64 >( fileOf( file )->data.size() );
		return MA_SUCCESS;
	}
}

AudioFileSystem::AudioFileSystem(){
	mCallbacks.onOpen = onOpen;
	mCallbacks.onOpenW = onOpenW;
	mCallbacks.onClose = onClose;
	mCallbacks.onRead = onRead;
	mCallbacks.onWrite = onWrite;
	mCallbacks.onSeek = onSeek;
	mCallbacks.onTell = onTell;
	mCallbacks.onInfo = onInfo;
}

ma_vfs* AudioFileSystem::vfs(){
	return &mCallbacks;
}
