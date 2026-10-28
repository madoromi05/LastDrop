#ifndef INCLUDED_GAMELIB_GRAPHICS_RENDERTARGETIMPL_H
#define INCLUDED_GAMELIB_GRAPHICS_RENDERTARGETIMPL_H

#include "GameLib/Graphics/RenderTarget.h"
#include "GameLib/Base/Impl/ReferenceType.h"

namespace GameLib{
namespace Graphics{

namespace RenderTargetDetail{
	const float kFixedSize = 0.f; // mScreenScale がこの値なら、画面に合わせて作り直さない
	const int kMinSize = 1;       // 画面倍率で求めた大きさの下限（0 だと作れない）

	inline DXGI_FORMAT toDxgi( RenderTargetFormat f ){
		switch ( f ){
			case RENDER_TARGET_RGBA8:   return DXGI_FORMAT_R8G8B8A8_UNORM;
			case RENDER_TARGET_RGBA16F: return DXGI_FORMAT_R16G16B16A16_FLOAT;
			case RENDER_TARGET_R32F:    return DXGI_FORMAT_R32_FLOAT;
			case RENDER_TARGET_RG16F:   return DXGI_FORMAT_R16G16_FLOAT;
			case RENDER_TARGET_RGBA8_SRGB: return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		}
		ASSERT( false && "RenderTarget : unknown format." );
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	}

	// 画面の大きさ screen に倍率 scale を掛けた大きさ（四捨五入、下限 kMinSize）。
	inline int scaledSize( int screen, float scale ){
		int r = static_cast< int >( static_cast< float >( screen ) * scale + 0.5f );
		return ( r < kMinSize ) ? kMinSize : r;
	}
}

class RenderTarget::Impl : public ReferenceType{
public:
	// screenScale: RenderTargetDetail::kFixedSize なら width/height の固定サイズ。正なら画面の倍率（width/height は今の画面の大きさ）。
	Impl( int width, int height, RenderTargetFormat format, float screenScale, ID3D11Device* device ) :
	mTexture( 0 ),
	mRTV( 0 ),
	mSRV( 0 ),
	mWidth( 0 ),
	mHeight( 0 ),
	mFormat( format ),
	mScreenScale( screenScale ),
	mDevice( device ){
		if ( mScreenScale == RenderTargetDetail::kFixedSize ){
			createDxObjects( width, height );
		}else{
			createDxObjects(
				RenderTargetDetail::scaledSize( width, mScreenScale ),
				RenderTargetDetail::scaledSize( height, mScreenScale ) );
		}
	}
	~Impl(){
		releaseDxObjects();
	}
	// 画面倍率で作った物を、今の画面の大きさに合わせる（同じ大きさなら何もしない）。
	void fitToScreen( int screenWidth, int screenHeight ){
		if ( mScreenScale == RenderTargetDetail::kFixedSize ){
			return;
		}
		int w = RenderTargetDetail::scaledSize( screenWidth, mScreenScale );
		int h = RenderTargetDetail::scaledSize( screenHeight, mScreenScale );
		if ( w == mWidth && h == mHeight ){
			return;
		}
		releaseDxObjects();
		createDxObjects( w, h );
	}

	ID3D11Texture2D*          mTexture;
	ID3D11RenderTargetView*   mRTV;
	ID3D11ShaderResourceView* mSRV;
	int mWidth;
	int mHeight;
	RenderTargetFormat mFormat;
	float mScreenScale;
private:
	void createDxObjects( int w, int h ){
		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width            = static_cast< UINT >( w );
		desc.Height           = static_cast< UINT >( h );
		desc.MipLevels        = 1;
		desc.ArraySize        = 1;
		desc.Format           = RenderTargetDetail::toDxgi( mFormat );
		desc.SampleDesc.Count = 1;
		desc.Usage            = D3D11_USAGE_DEFAULT;
		desc.BindFlags        = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		HRESULT hr = mDevice->CreateTexture2D( &desc, nullptr, &mTexture );
		STRONG_ASSERT( SUCCEEDED( hr ) && "RenderTarget : CreateTexture2D failed" );
		hr = mDevice->CreateRenderTargetView( mTexture, nullptr, &mRTV );
		STRONG_ASSERT( SUCCEEDED( hr ) && "RenderTarget : CreateRenderTargetView failed" );
		hr = mDevice->CreateShaderResourceView( mTexture, nullptr, &mSRV );
		STRONG_ASSERT( SUCCEEDED( hr ) && "RenderTarget : CreateShaderResourceView failed" );
		mWidth = w;
		mHeight = h;
	}
	void releaseDxObjects(){
		if ( mSRV ){ mSRV->Release(); mSRV = 0; }
		if ( mRTV ){ mRTV->Release(); mRTV = 0; }
		if ( mTexture ){ mTexture->Release(); mTexture = 0; }
	}
	ID3D11Device* mDevice;
};

} //namespace Graphics
} //namespace GameLib

#endif
