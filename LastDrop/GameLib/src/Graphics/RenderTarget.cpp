#include <d3d11.h>
#undef max
#undef min

#include "GameLib/GameLib.h"
#include "GameLib/Graphics/RenderTarget.h"
#include "Graphics/RenderTargetImpl.h"
#include "Graphics/ManagerImpl.h"

namespace GameLib{
namespace Graphics{

RenderTarget RenderTarget::create( int width, int height, RenderTargetFormat format, int mipLevels ){
	ASSERT( width > 0 && height > 0 && "RenderTarget : size must be positive." );
	ASSERT( mipLevels >= FULL_MIP_CHAIN && "RenderTarget : mipLevels must not be negative." );
	RenderTarget r;
	r.mImpl = NEW Impl( width, height, format, RenderTargetDetail::kFixedSize, mipLevels, gManagerImpl->dxDevice() );
	return r;
}

RenderTarget RenderTarget::createScreenSized( float scale, RenderTargetFormat format, int mipLevels ){
	ASSERT( scale > 0.f && "RenderTarget : scale must be positive." );
	ASSERT( mipLevels >= FULL_MIP_CHAIN && "RenderTarget : mipLevels must not be negative." );
	RenderTarget r;
	r.mImpl = NEW Impl( gManagerImpl->mBackBufferWidth, gManagerImpl->mBackBufferHeight, format, scale, mipLevels, gManagerImpl->dxDevice() );
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

int RenderTarget::mipLevels() const {
	ASSERT( mImpl && "Graphics::RenderTarget : This is empty object." );
	return mImpl->mMipLevels;
}

void RenderTarget::writeTga( const char* filename ) const {
	ASSERT( mImpl && "Graphics::RenderTarget : This is empty object." );
	ASSERT( ( mImpl->mFormat == RENDER_TARGET_RGBA8 || mImpl->mFormat == RENDER_TARGET_RGBA8_SRGB ) &&
		"RenderTarget : writeTga needs an 8-bit RGBA target." );
	// 描画先は GPU だけが読み書きできるので、CPU が読める写し（staging）へ移してから読む（Manager::capture と同じ）。
	D3D11_TEXTURE2D_DESC desc;
	mImpl->mTexture->GetDesc( &desc );
	desc.Usage          = D3D11_USAGE_STAGING;
	desc.BindFlags      = 0;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	desc.MiscFlags      = 0;
	ID3D11Texture2D* staging = nullptr;
	if ( FAILED( gManagerImpl->dxDevice()->CreateTexture2D( &desc, nullptr, &staging ) ) ){
		return;
	}
	ID3D11DeviceContext* context = gManagerImpl->dxContext();
	context->CopyResource( staging, mImpl->mTexture );
	D3D11_MAPPED_SUBRESOURCE mapped = {};
	if ( SUCCEEDED( context->Map( staging, 0, D3D11_MAP_READ, 0, &mapped ) ) ){
		// 描画先は R が下位の並び（RGBA）、TGA の書き出しは B が下位の並び（BGRA）なので入れ替える。
		const int w = mImpl->mWidth;
		const int h = mImpl->mHeight;
		std::vector< unsigned > pixels( static_cast< size_t >( w ) * h );
		const unsigned* src = static_cast< const unsigned* >( mapped.pData );
		const int pitch = static_cast< int >( mapped.RowPitch / sizeof( unsigned ) );
		for ( int y = 0; y < h; ++y ){
			for ( int x = 0; x < w; ++x ){
				unsigned p = src[ y * pitch + x ];
				unsigned r = p & 0xff;
				unsigned b = ( p >> 16 ) & 0xff;
				pixels[ y * w + x ] = ( p & 0xff00ff00 ) | ( r << 16 ) | b;
			}
		}
		Texture::Impl::write( filename, w, h, w * static_cast< int >( sizeof( unsigned ) ), pixels.data() );
		context->Unmap( staging, 0 );
	}
	staging->Release();
}

//NULLポインタからの変換
RenderTarget::RenderTarget( const Impl* ) : mImpl( 0 ){
}

#define TYPE RenderTarget
#include "GameLib/Base/Impl/ReferenceTypeTemplate.h"

} //namespace Graphics
} //namespace GameLib
