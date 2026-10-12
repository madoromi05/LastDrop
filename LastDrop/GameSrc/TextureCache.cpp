#include "TextureCache.h"

#include "GameLib/GameLib.h"
#include "GameLib/Base/DebugStream.h"

using namespace GameLib;

TextureCache::TextureCache() {
}

Graphics::Texture TextureCache::load( const char* filename ) {
	ASSERT( filename && filename[ 0 ] != '\0' && "TextureCache : filename is empty." );
	auto found = mTextures.find( filename );
	if ( found != mTextures.end() ) {
		return found->second;
	}
	// ここに来るのはファイルごとに1回だけ。使い回せているかはこの出力の回数で確かめられる。
	GameLib::cout << "TextureCache: load " << filename << GameLib::endl;
	Graphics::Texture texture = Graphics::Texture::create( filename );
	mTextures[ filename ] = texture;
	return texture;
}
