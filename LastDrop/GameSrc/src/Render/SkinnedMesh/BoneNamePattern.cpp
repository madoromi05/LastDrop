#include "GameSrc/Render/SkinnedMesh/BoneNamePattern.h"
#include "GameSrc/Render/SkinnedMesh/SkinnedMeshData.h"

#include "GameLib/GameLib.h"

#include <string>

namespace{
	// 名前の末尾がこれなら、その前までで始まる骨を全部含める。
	const char kWildcard = '*';
}

void BoneNamePattern::append( const SkinnedMeshData* data, const char* prefix, const char* name, std::vector< int >* out ) {
	ASSERT( data && prefix && name && out && "BoneNamePattern : argument is null." );
	std::string pattern = std::string( prefix ) + name;
	if ( pattern.back() != kWildcard ) {
		int bone = data->findBone( pattern.c_str() );
		if ( bone >= 0 ) {
			out->push_back( bone );
		}
		return;
	}
	pattern.pop_back();
	for ( int bone = 0; bone < data->boneNumber(); ++bone ) {
		if ( std::string( data->boneName( bone ) ).compare( 0, pattern.size(), pattern ) == 0 ) {
			out->push_back( bone );
		}
	}
}

int BoneNamePattern::find( const SkinnedMeshData* data, const char* prefix, const char* name ) {
	ASSERT( data && prefix && name && "BoneNamePattern : argument is null." );
	return data->findBone( ( std::string( prefix ) + name ).c_str() );
}
