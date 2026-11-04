#include "GameSrc/Scene/Game/ItemIcons.h"

#include "GameLib/GameLib.h"

using namespace GameLib;

ItemIcons::ItemIcons() {
	for ( int i = 0; i < ITEM_NUMBER; ++i ) {
		mTextures[ i ] = Graphics::Texture::create( itemSpec( static_cast< ItemId >( i ) ).icon, Graphics::TEXTURE_COLOR_SPACE_LINEAR );
	}
}

const Graphics::Texture* ItemIcons::icon( ItemId id ) {
	ASSERT( id >= 0 && id < ITEM_NUMBER && "ItemIcons::icon : bad ItemId." );
	Graphics::Texture& texture = mTextures[ id ];
	if ( !texture.isReady() || texture.isError() ) {
		return nullptr;
	}
	return &texture;
}
