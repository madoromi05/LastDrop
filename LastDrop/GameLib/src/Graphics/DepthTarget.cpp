#include <d3d11.h>
#undef max
#undef min

#include "GameLib/GameLib.h"
#include "GameLib/Graphics/DepthTarget.h"
#include "Graphics/DepthTargetImpl.h"
#include "Graphics/ManagerImpl.h"

namespace GameLib{
namespace Graphics{

DepthTarget DepthTarget::create( int width, int height ){
	ASSERT( width > 0 && height > 0 && "DepthTarget : size must be positive." );
	DepthTarget r;
	r.mImpl = NEW Impl( width, height, RenderTargetDetail::kFixedSize, gManagerImpl->dxDevice() );
	return r;
}

DepthTarget DepthTarget::createScreenSized( float scale ){
	ASSERT( scale > 0.f && "DepthTarget : scale must be positive." );
	DepthTarget r;
	r.mImpl = NEW Impl( gManagerImpl->mBackBufferWidth, gManagerImpl->mBackBufferHeight, scale, gManagerImpl->dxDevice() );
	return r;
}

int DepthTarget::width() const {
	ASSERT( mImpl && "Graphics::DepthTarget : This is empty object." );
	return mImpl->mWidth;
}

int DepthTarget::height() const {
	ASSERT( mImpl && "Graphics::DepthTarget : This is empty object." );
	return mImpl->mHeight;
}

//NULLポインタからの変換
DepthTarget::DepthTarget( const Impl* ) : mImpl( 0 ){
}

#define TYPE DepthTarget
#include "GameLib/Base/Impl/ReferenceTypeTemplate.h"

} //namespace Graphics
} //namespace GameLib
