#include <d3d11.h>
#undef max
#undef min

#include "GameLib/GameLib.h"
#include "GameLib/Graphics/RenderTarget.h"
#include "Graphics/RenderTargetImpl.h"
#include "Graphics/ManagerImpl.h"

namespace GameLib{
namespace Graphics{

RenderTarget RenderTarget::create( int width, int height, RenderTargetFormat format ){
	ASSERT( width > 0 && height > 0 && "RenderTarget : size must be positive." );
	RenderTarget r;
	r.mImpl = NEW Impl( width, height, format, RenderTargetDetail::kFixedSize, gManagerImpl->dxDevice() );
	return r;
}

RenderTarget RenderTarget::createScreenSized( float scale, RenderTargetFormat format ){
	ASSERT( scale > 0.f && "RenderTarget : scale must be positive." );
	RenderTarget r;
	r.mImpl = NEW Impl( gManagerImpl->mBackBufferWidth, gManagerImpl->mBackBufferHeight, format, scale, gManagerImpl->dxDevice() );
	return r;
}

int RenderTarget::width() const {
	ASSERT( mImpl && "Graphics::RenderTarget : This is empty object." );
	return mImpl->mWidth;
}

int RenderTarget::height() const {
	ASSERT( mImpl && "Graphics::RenderTarget : This is empty object." );
	return mImpl->mHeight;
}

RenderTargetFormat RenderTarget::format() const {
	ASSERT( mImpl && "Graphics::RenderTarget : This is empty object." );
	return mImpl->mFormat;
}

//NULLポインタからの変換
RenderTarget::RenderTarget( const Impl* ) : mImpl( 0 ){
}

#define TYPE RenderTarget
#include "GameLib/Base/Impl/ReferenceTypeTemplate.h"

} //namespace Graphics
} //namespace GameLib
