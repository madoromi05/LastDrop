#include "GameSrc/Render/Flipbook.h"

#include "GameLib/GameLib.h"

#include <algorithm>

Flipbook::Flipbook( int columns, int rows, int frameNumber ) :
	mColumns( columns ),
	mRows( rows ),
	mFrameNumber( frameNumber ) {
	ASSERT( columns > 0 && rows > 0 && "Flipbook : grid must not be empty." );
	ASSERT( frameNumber > 0 && frameNumber <= columns * rows && "Flipbook : frameNumber is out of the grid." );
}

int Flipbook::frameNumber() const {
	return mFrameNumber;
}

int Flipbook::frameAt( float progress ) const {
	// 最後のコマも同じ長さだけ見せる（progress × コマの数 を切り捨て、1 のときだけ最後のコマへ寄せる）
	int frame = static_cast< int >( progress * static_cast< float >( mFrameNumber ) );
	return std::min( std::max( frame, 0 ), mFrameNumber - 1 );
}

QuadUv Flipbook::frameUv( int frame ) const {
	frame = std::min( std::max( frame, 0 ), mFrameNumber - 1 );
	float width = 1.f / static_cast< float >( mColumns );
	float height = 1.f / static_cast< float >( mRows );
	QuadUv uv;
	uv.left = static_cast< float >( frame % mColumns ) * width;
	uv.top = static_cast< float >( frame / mColumns ) * height;
	uv.right = uv.left + width;
	uv.bottom = uv.top + height;
	return uv;
}
