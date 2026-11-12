#ifndef INCLUDED_GAMELIB_GRAPHICS_RENDERTARGETIMPL_H
#define INCLUDED_GAMELIB_GRAPHICS_RENDERTARGETIMPL_H

#include "GameLib/Graphics/RenderTarget.h"
#include "GameLib/Base/Impl/ReferenceType.h"
#include <vector>

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

	// 画面の大きさ screen に倍率 scale を掛けた大きさ（四捨五入、下限 kMinSize）
	inline int scaledSize( int screen, float scale ){
		int r = static_cast< int >( static_cast< float >( screen ) * scale + 0.5f );
		return ( r < kMinSize ) ? kMinSize : r;
	}

	// w×h から 1×1 になるまでの段の数（D3D11 と同じく、段ごとに半分の切り捨て）
	inline int fullMipChain( int w, int h ){
		int larger = ( w > h ) ? w : h;
		int levels = 1;
		while ( larger > 1 ){
			larger /= 2;
			++levels;
		}
		return levels;
	}

	// 段 mip の一辺（半分の切り捨て、下限 1）
	inline int mipSize( int size, int mip ){
		int r = size >> mip;
		return ( r < 1 ) ? 1 : r;
	}
}

class RenderTarget::Impl : public ReferenceType{
public:
	// screenScale: RenderTargetDetail::kFixedSize なら width/height の固定サイズ。正なら画面の倍率（width/height は今の画面の大きさ）
	// mipLevels: 段の数。RenderTarget::FULL_MIP_CHAIN なら 1 画素まで全部（大きさが変わったら数え直す）
	Impl( int width, int height, RenderTargetFormat format, float screenScale, int mipLevels, ID3D11Device* device ) :
	mTexture( 0 ),
	mRTV( 0 ),
	mSRV( 0 ),
	mWidth( 0 ),
	mHeight( 0 ),
	mMipLevels( 0 ),
	mFormat( format ),
	mScreenScale( screenScale ),
	mRequestedMipLevels( mipLevels ),
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
	// 画面倍率で作った物を、今の画面の大きさに合わせる（同じ大きさなら何もしない）
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
	int mipWidth( int mip ) const { return RenderTargetDetail::mipSize( mWidth, mip ); }
	int mipHeight( int mip ) const { return RenderTargetDetail::mipSize( mHeight, mip ); }

	ID3D11Texture2D*          mTexture;
	ID3D11RenderTargetView*   mRTV;  // 段 0 に描く
	ID3D11ShaderResourceView* mSRV;  // 全部の段を読む
	// 段ごとの描く・読む view（段が 2 つ以上のときだけ。[ 0 ] は段 0）
	std::vector< ID3D11RenderTargetView* >   mMipRTVs;
	std::vector< ID3D11ShaderResourceView* > mMipSRVs;
	int mWidth;
	int mHeight;
	int mMipLevels;
	RenderTargetFormat mFormat;
	float mScreenScale;
private:
	void createDxObjects( int w, int h ){
		const int levels = ( mRequestedMipLevels == RenderTarget::FULL_MIP_CHAIN ) ?
			RenderTargetDetail::fullMipChain( w, h ) : mRequestedMipLevels;
		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width            = static_cast< UINT >( w );
		desc.Height           = static_cast< UINT >( h );
		desc.MipLevels        = static_cast< UINT >( levels );
		desc.ArraySize        = 1;
		desc.Format           = RenderTargetDetail::toDxgi( mFormat );
		desc.SampleDesc.Count = 1;
		desc.Usage            = D3D11_USAGE_DEFAULT;
		desc.BindFlags        = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		HRESULT hr = mDevice->CreateTexture2D( &desc, nullptr, &mTexture );
		STRONG_ASSERT( SUCCEEDED( hr ) && "RenderTarget : CreateTexture2D failed" );
		hr = mDevice->CreateRenderTargetView( mTexture, nullptr, &mRTV ); // 何も指定しなければ段 0
		STRONG_ASSERT( SUCCEEDED( hr ) && "RenderTarget : CreateRenderTargetView failed" );
		hr = mDevice->CreateShaderResourceView( mTexture, nullptr, &mSRV ); // 何も指定しなければ全部の段
		STRONG_ASSERT( SUCCEEDED( hr ) && "RenderTarget : CreateShaderResourceView failed" );
		if ( levels > 1 ){
			createMipViews( desc.Format, levels );
		}
		mWidth = w;
		mHeight = h;
		mMipLevels = levels;
	}
	// 段ごとの view。縮小のパスは「段 i に描きながら段 i−1 を読む」（同じ画像の別の段なので、D3D11 は両方を同時に付けられる）
	void createMipViews( DXGI_FORMAT format, int levels ){
		mMipRTVs.assign( levels, 0 );
		mMipSRVs.assign( levels, 0 );
		for ( int mip = 0; mip < levels; ++mip ){
			D3D11_RENDER_TARGET_VIEW_DESC rtv = {};
			rtv.Format             = format;
			rtv.ViewDimension      = D3D11_RTV_DIMENSION_TEXTURE2D;
			rtv.Texture2D.MipSlice = static_cast< UINT >( mip );
			HRESULT hr = mDevice->CreateRenderTargetView( mTexture, &rtv, &mMipRTVs[ mip ] );
			STRONG_ASSERT( SUCCEEDED( hr ) && "RenderTarget : CreateRenderTargetView (mip) failed" );
			D3D11_SHADER_RESOURCE_VIEW_DESC srv = {};
			srv.Format                    = format;
			srv.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
			srv.Texture2D.MostDetailedMip = static_cast< UINT >( mip );
			srv.Texture2D.MipLevels       = 1;
			hr = mDevice->CreateShaderResourceView( mTexture, &srv, &mMipSRVs[ mip ] );
			STRONG_ASSERT( SUCCEEDED( hr ) && "RenderTarget : CreateShaderResourceView (mip) failed" );
		}
	}
	void releaseDxObjects(){
		for ( ID3D11RenderTargetView* view : mMipRTVs ){ if ( view ){ view->Release(); } }
		for ( ID3D11ShaderResourceView* view : mMipSRVs ){ if ( view ){ view->Release(); } }
		mMipRTVs.clear();
		mMipSRVs.clear();
		if ( mSRV ){ mSRV->Release(); mSRV = 0; }
		if ( mRTV ){ mRTV->Release(); mRTV = 0; }
		if ( mTexture ){ mTexture->Release(); mTexture = 0; }
	}
	int mRequestedMipLevels; // 作るときに頼まれた段の数（FULL_MIP_CHAIN のままにして、大きさが変わったら数え直す）
	ID3D11Device* mDevice;
};

} //namespace Graphics
} //namespace GameLib

#endif
