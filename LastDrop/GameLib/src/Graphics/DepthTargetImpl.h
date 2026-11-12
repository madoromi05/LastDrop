#ifndef INCLUDED_GAMELIB_GRAPHICS_DEPTHTARGETIMPL_H
#define INCLUDED_GAMELIB_GRAPHICS_DEPTHTARGETIMPL_H

#include "GameLib/Graphics/DepthTarget.h"
#include "GameLib/Base/Impl/ReferenceType.h"
#include "Graphics/RenderTargetImpl.h" // RenderTargetDetail（大きさの決まりは RenderTarget と同じ）

namespace GameLib{
namespace Graphics{

class DepthTarget::Impl : public ReferenceType{
public:
	// screenScale の意味は RenderTarget::Impl と同じ
	Impl( int width, int height, float screenScale, ID3D11Device* device ) :
	mTexture( 0 ),
	mDSV( 0 ),
	mReadOnlyDSV( 0 ),
	mSRV( 0 ),
	mWidth( 0 ),
	mHeight( 0 ),
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
	ID3D11DepthStencilView*   mDSV;
	ID3D11DepthStencilView*   mReadOnlyDSV; // 奥行きの判定だけに使い、書き込まない（付けている間も mSRV から読める）
	ID3D11ShaderResourceView* mSRV;
	int mWidth;
	int mHeight;
	float mScreenScale;
private:
	void createDxObjects( int w, int h ){
		// 深度として書き、float として読むため、テクスチャは型を決めずに作り、ビューごとに読み方を決める
		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width            = static_cast< UINT >( w );
		desc.Height           = static_cast< UINT >( h );
		desc.MipLevels        = 1;
		desc.ArraySize        = 1;
		desc.Format           = DXGI_FORMAT_R32_TYPELESS;
		desc.SampleDesc.Count = 1;
		desc.Usage            = D3D11_USAGE_DEFAULT;
		desc.BindFlags        = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
		HRESULT hr = mDevice->CreateTexture2D( &desc, nullptr, &mTexture );
		STRONG_ASSERT( SUCCEEDED( hr ) && "DepthTarget : CreateTexture2D failed" );

		D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format        = DXGI_FORMAT_D32_FLOAT;
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		hr = mDevice->CreateDepthStencilView( mTexture, &dsvDesc, &mDSV );
		STRONG_ASSERT( SUCCEEDED( hr ) && "DepthTarget : CreateDepthStencilView failed" );
		// 読み取り専用の付け方（半透明の板が、奥行きで隠れながら同じ深度を読む）
		dsvDesc.Flags = D3D11_DSV_READ_ONLY_DEPTH;
		hr = mDevice->CreateDepthStencilView( mTexture, &dsvDesc, &mReadOnlyDSV );
		STRONG_ASSERT( SUCCEEDED( hr ) && "DepthTarget : CreateDepthStencilView (read only) failed" );

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format              = DXGI_FORMAT_R32_FLOAT;
		srvDesc.ViewDimension       = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		hr = mDevice->CreateShaderResourceView( mTexture, &srvDesc, &mSRV );
		STRONG_ASSERT( SUCCEEDED( hr ) && "DepthTarget : CreateShaderResourceView failed" );
		mWidth = w;
		mHeight = h;
	}
	void releaseDxObjects(){
		if ( mSRV ){ mSRV->Release(); mSRV = 0; }
		if ( mReadOnlyDSV ){ mReadOnlyDSV->Release(); mReadOnlyDSV = 0; }
		if ( mDSV ){ mDSV->Release(); mDSV = 0; }
		if ( mTexture ){ mTexture->Release(); mTexture = 0; }
	}
	ID3D11Device* mDevice;
};

} //namespace Graphics
} //namespace GameLib

#endif
