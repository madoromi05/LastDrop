#include "GameSrc/Render/StaticMesh/StaticMeshLods.h"
#include "GameSrc/Render/StaticMesh/StaticMeshData.h"
#include "GameSrc/Render/MeshCache.h"

#include "GameLib/GameLib.h"

#include <string>

namespace{
	// 粗い形のファイル名の付け方（tools/convert/fbx_to_gamelib.py の lod_path と同じ）。
	const char* kLodSuffix = "_lod";
}

StaticMeshLods::StaticMeshLods( const StaticMeshData* full ) :
	mLevels{ full, nullptr, nullptr } {
	ASSERT( full && "StaticMeshLods : full is null." );
}

StaticMeshLods StaticMeshLods::request( MeshCache* meshes, const char* filename ) {
	ASSERT( meshes && filename && "StaticMeshLods : an argument is null." );
	StaticMeshLods lods( meshes->staticMesh( filename ) );
	const std::string name( filename );
	const std::string::size_type dot = name.rfind( '.' );
	const std::string stem = name.substr( 0, dot );
	const std::string extension = ( dot == std::string::npos ) ? std::string() : name.substr( dot );
	for ( int i = 1; i < kLevelNumber; ++i ) {
		const std::string lodName = stem + kLodSuffix + std::to_string( i ) + extension;
		lods.mLevels[ i ] = meshes->staticMesh( lodName.c_str() );
	}
	return lods;
}

const StaticMeshData* StaticMeshLods::full() const {
	return mLevels[ 0 ];
}

const StaticMeshData* StaticMeshLods::level( int level ) const {
	ASSERT( level >= 0 && level < kLevelNumber && "StaticMeshLods : bad level." );
	for ( int i = level; i > 0; --i ) {
		if ( mLevels[ i ] && mLevels[ i ]->isReady() ) {
			return mLevels[ i ];
		}
	}
	return mLevels[ 0 ];
}
