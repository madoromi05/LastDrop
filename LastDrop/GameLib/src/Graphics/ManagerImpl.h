#ifndef INCLUDED_GAMELIB_GRAPHICS_MANAGERIMPL_H
#define INCLUDED_GAMELIB_GRAPHICS_MANAGERIMPL_H

#include "Graphics/NoLightingVsHlsl.h"
#include "Graphics/NoLightingPsHlsl.h"
#include "Graphics/VertexLightingVsHlsl.h"
#include "Graphics/PixelLightingVsHlsl.h"
#include "Graphics/PixelLightingPsHlsl.h"

#include "GameLib/Math/Functions.h"
#include "GameLib/Math/Matrix44.h"
#include "GameLib/Math/Matrix34.h"
#include "GameLib/Graphics/Enum.h"
#include "GameLib/Graphics/Texture.h"
#include "GameLib/Graphics/VertexBuffer.h"
#include "GameLib/Graphics/IndexBuffer.h"
#include "GameLib/Threading/Manager.h"
#include "Graphics/TextureImpl.h"
#include "Graphics/VertexBufferImpl.h"
#include "Graphics/IndexBufferImpl.h"

#include <d3dcompiler.h>
#include <string>
#include <vector>

namespace GameLib{
using namespace Math;
namespace Graphics{

// GPU constant buffer layouts (must match HLSL cbuffers exactly)
struct VSCBufferData {
	float transform[4][4];    // c0-c3  (transform matrix)
	float worldMatrix[3][4];  // c4-c6  (world matrix)
	float itwMatrix[3][4];    // c7-c9  (inverse-transposed world matrix)
	float diffuseColor[4];    // c10
	float ambientColor[4];    // c11
	float lightPosX[4];       // c12
	float lightPosY[4];       // c13
	float lightPosZ[4];       // c14
	float lightColor[4][4];   // c15-c18
	// Total: 304 bytes
};
struct PSCBufferData {
	float ambient[4];         // c0
	float lightPosX[4];       // c1
	float lightPosY[4];       // c2
	float lightPosZ[4];       // c3
	float eyePosition[4];     // c4
	float specularColor[4];   // c5
	float lightColor[4][4];   // c6-c9
	float emissionColor[4];   // c10
	// Total: 176 bytes
};

class ManagerImpl{
public:
	ManagerImpl( HWND hwnd, int w, int h, bool fullScreen, bool vSync, bool antiAlias ) :
	mDevice( 0 ),
	mContext( 0 ),
	mSwapChain( 0 ),
	mRTV( 0 ),
	mDSV( 0 ),
	mBackBufferWidth( w ),
	mBackBufferHeight( h ),
	mNoLightingVS( 0 ),
	mVertexLightingVS( 0 ),
	mPixelLightingVS( 0 ),
	mNoLightingPS( 0 ),
	mPixelLightingPS( 0 ),
	mInputLayout( 0 ),
	mVSCB( 0 ),
	mPSCB( 0 ),
	mVSCBDirty( true ),
	mPSCBDirty( true ),
	mCurrentCullMode( 1 ),  // CULL_BACK
	mCurrentDepthTest( true ),
	mCurrentDepthWrite( true ),
	mCurrentBlendMode( 0 ), // BLEND_OPAQUE
	mCurrentSampler( 1 ),   // LINEAR
	mWidth( w ),
	mHeight( h ),
	mFullScreen( fullScreen ),
	mVSync( vSync ),
	mLightingMode( LIGHTING_NONE ),
	mFullScreenWindowWidth( w ),
	mFullScreenWindowHeight( h ),
	mFrameId( 0 ),
	mCanRender( true ),
	mWindowHandle( hwnd ),
	mFullScreenQuadVB( 0 ),
	mWhiteTexture( 0 ),
	mCurrentTexture( 0 ),
	mCurrentVB( 0 ),
	mCurrentIB( 0 ),
	mLightChanged( true ),
	mMatricesChanged( true ),
	mPixelShaderReady( true ),
	mCaptureRequest( false ){
		for ( int i = 0; i < 4; ++i ){
			mLightColors[ i ].set( 0.f, 0.f, 0.f );
			mLightIntensities[ i ] = 0.f;
		}
		mProjectionViewMatrix.setIdentity();
		mWorldMatrix.setIdentity();
		mEyePosition.set( 0.f, 0.f, 0.f, 0.f );
		mDiffuseColor.set( 1.f, 1.f, 1.f, 1.f );
		mSpecularColor.set( 1.f, 1.f, 1.f, 1.f );
		mEmissionColor.set( 0.f, 0.f, 0.f, 0.f );
		mAmbientColor.set( 1.f, 1.f, 1.f, 1.f );
		ZeroMemory( &mViewport, sizeof( mViewport ) );
		ZeroMemory( mRSStates,   sizeof( mRSStates ) );
		ZeroMemory( mDSSStates,  sizeof( mDSSStates ) );
		ZeroMemory( mBlendStates, sizeof( mBlendStates ) );
		ZeroMemory( mSamplerStates, sizeof( mSamplerStates ) );

		// --- Create device and swap chain ---
		DXGI_SWAP_CHAIN_DESC scDesc = {};
		scDesc.BufferCount                        = 1;
		scDesc.BufferDesc.Width                   = static_cast< UINT >( w );
		scDesc.BufferDesc.Height                  = static_cast< UINT >( h );
		scDesc.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
		scDesc.BufferDesc.RefreshRate.Numerator   = 60;
		scDesc.BufferDesc.RefreshRate.Denominator = 1;
		scDesc.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		scDesc.OutputWindow                       = hwnd;
		scDesc.SampleDesc.Count                   = 1;
		scDesc.SampleDesc.Quality                 = 0;
		scDesc.Windowed                           = TRUE;
		scDesc.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD;
		scDesc.Flags                              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0 };
		D3D_FEATURE_LEVEL actualLevel;
		UINT createFlags = 0;
#ifdef _DEBUG
		createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
		HRESULT hr = D3D11CreateDeviceAndSwapChain(
			nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
			createFlags, featureLevels, 3,
			D3D11_SDK_VERSION,
			&scDesc, &mSwapChain,
			&mDevice, &actualLevel, &mContext );
		STRONG_ASSERT( SUCCEEDED( hr ) && "D3D11CreateDeviceAndSwapChain failed" );

		createRTVandDSV( w, h );

		// Query fullscreen resolution from DXGI output
		{
			IDXGIDevice* dxgiDevice = nullptr;
			IDXGIAdapter* adapter   = nullptr;
			IDXGIOutput* output     = nullptr;
			if ( SUCCEEDED( mDevice->QueryInterface( __uuidof( IDXGIDevice ), (void**)&dxgiDevice ) ) ){
				if ( SUCCEEDED( dxgiDevice->GetAdapter( &adapter ) ) ){
					if ( SUCCEEDED( adapter->EnumOutputs( 0, &output ) ) ){
						UINT numModes = 0;
						output->GetDisplayModeList( DXGI_FORMAT_R8G8B8A8_UNORM, 0, &numModes, nullptr );
						if ( numModes > 0 ){
							std::vector< DXGI_MODE_DESC > modes( numModes );
							output->GetDisplayModeList( DXGI_FORMAT_R8G8B8A8_UNORM, 0, &numModes, modes.data() );
							// Pick the closest mode that is >= requested size
							float nativeAspect = static_cast< float >( w ) / static_cast< float >( h );
							int   minScore     = 0x7fffffff;
							for ( UINT i = 0; i < numModes; ++i ){
								int tw = static_cast< int >( modes[ i ].Width );
								int th = static_cast< int >( modes[ i ].Height );
								if ( tw >= w && th >= h ){
									float asp = static_cast< float >( tw ) / th;
									if ( Math::abs( nativeAspect - asp ) < 0.01f ){
										int score = ( tw - w ) + ( th - h );
										if ( score < minScore ){
											minScore = score;
											mFullScreenWindowWidth  = tw;
											mFullScreenWindowHeight = th;
										}
									}
								}
							}
						}
						output->Release();
					}
					adapter->Release();
				}
				dxgiDevice->Release();
			}
		}

		// --- Compile and create shaders ---
		ID3DBlob* vsBlob = nullptr;
		vsBlob = compileShader( gNoLightingVsHlsl,    "main", "vs_4_0" );
		hr = mDevice->CreateVertexShader( vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &mNoLightingVS );
		STRONG_ASSERT( SUCCEEDED( hr ) );

		// Create input layout from the no-lighting VS bytecode
		D3D11_INPUT_ELEMENT_DESC layout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR",    0, DXGI_FORMAT_B8G8R8A8_UNORM,     0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		hr = mDevice->CreateInputLayout( layout, 4, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &mInputLayout );
		STRONG_ASSERT( SUCCEEDED( hr ) && "CreateInputLayout failed" );
		vsBlob->Release(); vsBlob = nullptr;

		vsBlob = compileShader( gVertexLightingVsHlsl, "main", "vs_4_0" );
		hr = mDevice->CreateVertexShader( vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &mVertexLightingVS );
		STRONG_ASSERT( SUCCEEDED( hr ) );
		vsBlob->Release(); vsBlob = nullptr;

		vsBlob = compileShader( gPixelLightingVsHlsl, "main", "vs_4_0" );
		hr = mDevice->CreateVertexShader( vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &mPixelLightingVS );
		STRONG_ASSERT( SUCCEEDED( hr ) );
		vsBlob->Release(); vsBlob = nullptr;

		ID3DBlob* psBlob = nullptr;
		psBlob = compileShader( gNoLightingPsHlsl,    "main", "ps_4_0" );
		hr = mDevice->CreatePixelShader( psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &mNoLightingPS );
		STRONG_ASSERT( SUCCEEDED( hr ) );
		psBlob->Release(); psBlob = nullptr;

		psBlob = compileShader( gPixelLightingPsHlsl, "main", "ps_4_0" );
		hr = mDevice->CreatePixelShader( psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &mPixelLightingPS );
		STRONG_ASSERT( SUCCEEDED( hr ) );
		psBlob->Release(); psBlob = nullptr;

		// --- Create constant buffers ---
		{
			D3D11_BUFFER_DESC cbd = {};
			cbd.ByteWidth      = sizeof( VSCBufferData );
			cbd.Usage          = D3D11_USAGE_DYNAMIC;
			cbd.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
			cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			hr = mDevice->CreateBuffer( &cbd, nullptr, &mVSCB );
			STRONG_ASSERT( SUCCEEDED( hr ) );
			cbd.ByteWidth = sizeof( PSCBufferData );
			hr = mDevice->CreateBuffer( &cbd, nullptr, &mPSCB );
			STRONG_ASSERT( SUCCEEDED( hr ) );
		}
		ZeroMemory( &mVSCBData, sizeof( mVSCBData ) );
		ZeroMemory( &mPSCBData, sizeof( mPSCBData ) );

		// --- Create rasterizer states ---
		// [0]=CULL_NONE, [1]=CULL_BACK, [2]=CULL_FRONT
		D3D11_RASTERIZER_DESC rsDesc = {};
		rsDesc.FillMode              = D3D11_FILL_SOLID;
		rsDesc.FrontCounterClockwise = TRUE;  // match DX9 CCW = front face
		rsDesc.DepthClipEnable       = TRUE;
		rsDesc.CullMode = D3D11_CULL_NONE;  mDevice->CreateRasterizerState( &rsDesc, &mRSStates[0] );
		rsDesc.CullMode = D3D11_CULL_BACK;  mDevice->CreateRasterizerState( &rsDesc, &mRSStates[1] );
		rsDesc.CullMode = D3D11_CULL_FRONT; mDevice->CreateRasterizerState( &rsDesc, &mRSStates[2] );

		// --- Create depth-stencil states ---
		// index = (depthTest ? 0 : 2) | (depthWrite ? 0 : 1)
		D3D11_DEPTH_STENCIL_DESC dssDesc = {};
		dssDesc.StencilEnable    = FALSE;
		for ( int i = 0; i < 4; ++i ){
			bool test  = ( i & 2 ) == 0;
			bool write = ( i & 1 ) == 0;
			dssDesc.DepthEnable    = test  ? TRUE : FALSE;
			dssDesc.DepthWriteMask = write ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
			dssDesc.DepthFunc      = test  ? D3D11_COMPARISON_LESS : D3D11_COMPARISON_ALWAYS;
			mDevice->CreateDepthStencilState( &dssDesc, &mDSSStates[ i ] );
		}

		// --- Create blend states ---
		// [0]=Opaque, [1]=Linear, [2]=Add
		D3D11_BLEND_DESC blendDesc = {};
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		// Opaque (no blending)
		blendDesc.RenderTarget[0].BlendEnable = FALSE;
		mDevice->CreateBlendState( &blendDesc, &mBlendStates[0] );
		// Linear
		blendDesc.RenderTarget[0].BlendEnable    = TRUE;
		blendDesc.RenderTarget[0].SrcBlend       = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend      = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp        = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha  = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		blendDesc.RenderTarget[0].BlendOpAlpha   = D3D11_BLEND_OP_ADD;
		mDevice->CreateBlendState( &blendDesc, &mBlendStates[1] );
		// Additive
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
		mDevice->CreateBlendState( &blendDesc, &mBlendStates[2] );

		// --- Create sampler states ---
		D3D11_SAMPLER_DESC sampDesc = {};
		sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
		// Point
		sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		mDevice->CreateSamplerState( &sampDesc, &mSamplerStates[0] );
		// Linear / Anisotropic
		sampDesc.Filter    = D3D11_FILTER_ANISOTROPIC;
		sampDesc.MaxAnisotropy = 16;
		mDevice->CreateSamplerState( &sampDesc, &mSamplerStates[1] );

		// --- Full-screen quad VB (3 verts that cover the screen) ---
		mFullScreenQuadVB = NEW VertexBuffer::Impl( 3, mDevice, mContext );
		Vertex* v = mFullScreenQuadVB->lock();
		for ( int i = 0; i < 3; ++i ){
			v[ i ].mColor = 0xffffffff;
			v[ i ].mPosition.set( -1.f, 1.f, 0.f, 1.f );
			v[ i ].mNormal.set( 0.f, 0.f, 1.f );
			v[ i ].mUv.set( 0.f, 0.f );
		}
		v[2].mPosition.x += 4.f;
		v[1].mPosition.y -= 4.f;
		v[2].mUv.x = 2.f;
		v[1].mUv.y = 2.f;
		mFullScreenQuadVB->unlock();

		// --- White 1x1 texture ---
		mWhiteTexture = NEW Texture::Impl( 1, 1, false, mDevice, mContext );
		unsigned* texData;
		int texPitch;
		mWhiteTexture->lock( &texData, &texPitch, 0 );
		*texData = 0xffffffff;
		mWhiteTexture->unlock( 0 );

		// Initialize logical viewport to full game resolution
		mViewport.TopLeftX = 0.f;
		mViewport.TopLeftY = 0.f;
		mViewport.Width    = static_cast< float >( w );
		mViewport.Height   = static_cast< float >( h );
		mViewport.MinDepth = 0.f;
		mViewport.MaxDepth = 1.f;

		setInitialStates();

		// --- Go fullscreen if requested ---
		if ( fullScreen ){
			mCanRender = false; // will be handled in restore()
		}
	}

	~ManagerImpl(){
		setTexture( 0 );
		setVertexBuffer( 0 );
		setIndexBuffer( 0 );

		// Return to windowed before releasing
		if ( mSwapChain ){
			mSwapChain->SetFullscreenState( FALSE, nullptr );
		}

		mWhiteTexture->release();
		SAFE_DELETE( mWhiteTexture );
		mFullScreenQuadVB->release();
		SAFE_DELETE( mFullScreenQuadVB );

		for ( int i = 0; i < 3; ++i ) if ( mRSStates[i] )    { mRSStates[i]->Release();    mRSStates[i] = 0; }
		for ( int i = 0; i < 4; ++i ) if ( mDSSStates[i] )   { mDSSStates[i]->Release();   mDSSStates[i] = 0; }
		for ( int i = 0; i < 3; ++i ) if ( mBlendStates[i] ) { mBlendStates[i]->Release(); mBlendStates[i] = 0; }
		for ( int i = 0; i < 2; ++i ) if ( mSamplerStates[i]){ mSamplerStates[i]->Release(); mSamplerStates[i] = 0; }

		if ( mInputLayout )       { mInputLayout->Release();       mInputLayout = 0; }
		if ( mNoLightingVS )      { mNoLightingVS->Release();      mNoLightingVS = 0; }
		if ( mVertexLightingVS )  { mVertexLightingVS->Release();  mVertexLightingVS = 0; }
		if ( mPixelLightingVS )   { mPixelLightingVS->Release();   mPixelLightingVS = 0; }
		if ( mNoLightingPS )      { mNoLightingPS->Release();      mNoLightingPS = 0; }
		if ( mPixelLightingPS )   { mPixelLightingPS->Release();   mPixelLightingPS = 0; }
		if ( mVSCB )              { mVSCB->Release();              mVSCB = 0; }
		if ( mPSCB )              { mPSCB->Release();              mPSCB = 0; }

		destroyRTVandDSV();
		if ( mSwapChain ) { mSwapChain->Release(); mSwapChain = 0; }
		if ( mContext )   { mContext->Release();   mContext = 0; }
		if ( mDevice )    { mDevice->Release();    mDevice = 0; }
	}

	// ---- Shader compilation helper ----
	static ID3DBlob* compileShader( const char* hlsl, const char* entry, const char* target ){
		ID3DBlob* blob  = nullptr;
		ID3DBlob* error = nullptr;
		HRESULT hr = D3DCompile(
			hlsl, strlen( hlsl ), nullptr, nullptr, nullptr,
			entry, target,
			D3DCOMPILE_OPTIMIZATION_LEVEL3, 0,
			&blob, &error );
		if ( FAILED( hr ) ){
			if ( error ){
				HALT( static_cast< const char* >( error->GetBufferPointer() ) );
				error->Release();
			}else{
				HALT( "Shader compile failed (unknown error)" );
			}
		}
		if ( error ) error->Release();
		return blob;
	}

	// ---- RTV / DSV creation ----
	void createRTVandDSV( int w, int h ){
		ID3D11Texture2D* backBuffer = nullptr;
		HRESULT hr = mSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), reinterpret_cast< void** >( &backBuffer ) );
		STRONG_ASSERT( SUCCEEDED( hr ) );
		hr = mDevice->CreateRenderTargetView( backBuffer, nullptr, &mRTV );
		STRONG_ASSERT( SUCCEEDED( hr ) );
		backBuffer->Release();

		D3D11_TEXTURE2D_DESC dsDesc = {};
		dsDesc.Width            = static_cast< UINT >( w );
		dsDesc.Height           = static_cast< UINT >( h );
		dsDesc.MipLevels        = 1;
		dsDesc.ArraySize        = 1;
		dsDesc.Format           = DXGI_FORMAT_D24_UNORM_S8_UINT;
		dsDesc.SampleDesc.Count = 1;
		dsDesc.Usage            = D3D11_USAGE_DEFAULT;
		dsDesc.BindFlags        = D3D11_BIND_DEPTH_STENCIL;
		ID3D11Texture2D* dsTex = nullptr;
		hr = mDevice->CreateTexture2D( &dsDesc, nullptr, &dsTex );
		STRONG_ASSERT( SUCCEEDED( hr ) );
		hr = mDevice->CreateDepthStencilView( dsTex, nullptr, &mDSV );
		STRONG_ASSERT( SUCCEEDED( hr ) );
		dsTex->Release();

		mBackBufferWidth  = w;
		mBackBufferHeight = h;
	}
	void destroyRTVandDSV(){
		if ( mDSV ) { mDSV->Release(); mDSV = 0; }
		if ( mRTV ) { mRTV->Release(); mRTV = 0; }
	}

	// ---- Set initial D3D11 pipeline state ----
	void setInitialStates(){
		mContext->IASetInputLayout( mInputLayout );
		mContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

		applyShader();
		applyRasterizerState();
		applyDepthStencilState();
		applyBlendState();
		applySamplerState();

		mContext->OMSetRenderTargets( 1, &mRTV, mDSV );

		// Set both constant buffers
		mContext->VSSetConstantBuffers( 0, 1, &mVSCB );
		mContext->PSSetConstantBuffers( 0, 1, &mPSCB );

		// Default viewport
		D3D11_VIEWPORT vp = {};
		vp.Width    = static_cast< float >( mBackBufferWidth );
		vp.Height   = static_cast< float >( mBackBufferHeight );
		vp.MaxDepth = 1.f;
		mContext->RSSetViewports( 1, &vp );

		mLightChanged    = true;
		mMatricesChanged = true;
	}

	void applyShader(){
		if ( mLightingMode == LIGHTING_NONE ){
			mContext->VSSetShader( mNoLightingVS, nullptr, 0 );
			mContext->PSSetShader( mNoLightingPS, nullptr, 0 );
		}else if ( mLightingMode == LIGHTING_PER_VERTEX ){
			mContext->VSSetShader( mVertexLightingVS, nullptr, 0 );
			mContext->PSSetShader( mNoLightingPS, nullptr, 0 );
		}else{
			mContext->VSSetShader( mPixelLightingVS, nullptr, 0 );
			mContext->PSSetShader( mPixelLightingPS, nullptr, 0 );
		}
	}
	void applyRasterizerState(){
		mContext->RSSetState( mRSStates[ mCurrentCullMode ] );
	}
	void applyDepthStencilState(){
		int idx = ( mCurrentDepthTest ? 0 : 2 ) | ( mCurrentDepthWrite ? 0 : 1 );
		mContext->OMSetDepthStencilState( mDSSStates[ idx ], 0 );
	}
	void applyBlendState(){
		float factor[4] = { 1.f, 1.f, 1.f, 1.f };
		mContext->OMSetBlendState( mBlendStates[ mCurrentBlendMode ], factor, 0xffffffff );
	}
	void applySamplerState(){
		mContext->PSSetSamplers( 0, 1, &mSamplerStates[ mCurrentSampler ] );
	}

	void beginDraw(){
		float clearColor[4] = { 0.f, 0.f, 0.f, 1.f };
		mContext->OMSetRenderTargets( 1, &mRTV, mDSV );
		mContext->ClearRenderTargetView( mRTV, clearColor );
		mContext->ClearDepthStencilView( mDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0 );
		setViewport(
			static_cast< int >( mViewport.TopLeftX ),
			static_cast< int >( mViewport.TopLeftY ),
			static_cast< int >( mViewport.Width ),
			static_cast< int >( mViewport.Height ) );
	}

	void endDraw(){
		if ( mCaptureRequest && !mCaptureFilename.empty() ){
			capture();
		}
		HRESULT hr = mSwapChain->Present( mVSync ? 1 : 0, 0 );
		if ( hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET ){
			mCanRender = false;
		}
		++mFrameId;
	}

	void capture(){
		ID3D11Texture2D* backBuffer = nullptr;
		if ( FAILED( mSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), reinterpret_cast< void** >( &backBuffer ) ) ) ) return;

		D3D11_TEXTURE2D_DESC desc;
		backBuffer->GetDesc( &desc );

		D3D11_TEXTURE2D_DESC sd = desc;
		sd.Usage          = D3D11_USAGE_STAGING;
		sd.BindFlags      = 0;
		sd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		sd.MiscFlags      = 0;
		ID3D11Texture2D* staging = nullptr;
		if ( SUCCEEDED( mDevice->CreateTexture2D( &sd, nullptr, &staging ) ) ){
			mContext->CopyResource( staging, backBuffer );
			D3D11_MAPPED_SUBRESOURCE ms = {};
			if ( SUCCEEDED( mContext->Map( staging, 0, D3D11_MAP_READ, 0, &ms ) ) ){
				int bw = static_cast< int >( desc.Width );
				int bh = static_cast< int >( desc.Height );
				// RGBA -> BGRA swap for the write helper
				std::vector< unsigned > buf( bw * bh );
				const unsigned* src = static_cast< const unsigned* >( ms.pData );
				int pitch = ms.RowPitch / 4;
				for ( int y = 0; y < bh; ++y ){
					for ( int x = 0; x < bw; ++x ){
						unsigned p = src[ y * pitch + x ];
						unsigned r = ( p )       & 0xff;
						unsigned g = ( p >> 8 )  & 0xff;
						unsigned b = ( p >> 16 ) & 0xff;
						unsigned a = ( p >> 24 ) & 0xff;
						buf[ y * bw + x ] = b | ( g << 8 ) | ( r << 16 ) | ( a << 24 );
					}
				}
				Texture::Impl::write( mCaptureFilename.c_str(), bw, bh, bw * 4, buf.data() );
				mContext->Unmap( staging, 0 );
			}
			staging->Release();
		}
		backBuffer->Release();
		mCaptureRequest = false;
	}

	void restore(){
		if ( mCanRender ) return;

		// Release references before ResizeBuffers
		mContext->OMSetRenderTargets( 0, nullptr, nullptr );
		destroyRTVandDSV();

		BOOL isFullScreen = FALSE;
		mSwapChain->GetFullscreenState( &isFullScreen, nullptr );

		if ( mFullScreen && !isFullScreen ){
			mSwapChain->SetFullscreenState( TRUE, nullptr );
		}else if ( !mFullScreen && isFullScreen ){
			mSwapChain->SetFullscreenState( FALSE, nullptr );
		}

		// Resize buffers (0,0,0 = keep current fullscreen size)
		UINT newW = mFullScreen ? 0 : static_cast< UINT >( mWidth );
		UINT newH = mFullScreen ? 0 : static_cast< UINT >( mHeight );
		HRESULT hr = mSwapChain->ResizeBuffers( 1, newW, newH, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH );
		if ( FAILED( hr ) ){
			cout << "Graphics::restore() : ResizeBuffers failed" << endl;
			return;
		}

		// Query actual size after resize
		DXGI_SWAP_CHAIN_DESC scDesc;
		mSwapChain->GetDesc( &scDesc );
		int actualW = static_cast< int >( scDesc.BufferDesc.Width );
		int actualH = static_cast< int >( scDesc.BufferDesc.Height );
		if ( mFullScreen ){
			mFullScreenWindowWidth  = actualW;
			mFullScreenWindowHeight = actualH;
		}

		createRTVandDSV( actualW, actualH );
		setInitialStates();
		mCanRender = true;
		cout << "Graphics::restore() : succeeded" << endl;
	}

	void setTexture( Texture::Impl* o ){
		if ( mCurrentTexture == o ) return;
		if ( mCurrentTexture ){
			mCurrentTexture->release();
			if ( mCurrentTexture->referenceCount() == 0 ) SAFE_DELETE( mCurrentTexture );
		}
		mCurrentTexture = o;
		if ( o ){
			o->refer();
			o->ensureGpuUploaded();
			mContext->PSSetShaderResources( 0, 1, &o->mSRV );
		}else{
			ID3D11ShaderResourceView* nullSRV = nullptr;
			mContext->PSSetShaderResources( 0, 1, &nullSRV );
		}
	}

	void setVertexBuffer( VertexBuffer::Impl* o ){
		if ( mCurrentVB == o ) return;
		if ( mCurrentVB ){
			mCurrentVB->release();
			if ( mCurrentVB->referenceCount() == 0 ) SAFE_DELETE( mCurrentVB );
		}
		mCurrentVB = o;
		if ( o ){
			o->refer();
			UINT stride = sizeof( Vertex );
			UINT offset = 0;
			mContext->IASetVertexBuffers( 0, 1, &o->mDxObject, &stride, &offset );
		}else{
			ID3D11Buffer* nullBuf = nullptr;
			UINT zero = 0;
			mContext->IASetVertexBuffers( 0, 1, &nullBuf, &zero, &zero );
		}
	}

	void setIndexBuffer( IndexBuffer::Impl* o ){
		if ( mCurrentIB == o ) return;
		if ( mCurrentIB ){
			mCurrentIB->release();
			if ( mCurrentIB->referenceCount() == 0 ) SAFE_DELETE( mCurrentIB );
		}
		mCurrentIB = o;
		if ( o ){
			o->refer();
			mContext->IASetIndexBuffer( o->mDxObject, DXGI_FORMAT_R16_UINT, 0 );
		}else{
			mContext->IASetIndexBuffer( nullptr, DXGI_FORMAT_R16_UINT, 0 );
		}
	}

	void enableDepthTest( bool f ){
		if ( mCurrentDepthTest == f ) return;
		mCurrentDepthTest = f;
		applyDepthStencilState();
	}
	void enableDepthWrite( bool f ){
		if ( mCurrentDepthWrite == f ) return;
		mCurrentDepthWrite = f;
		applyDepthStencilState();
	}

	void setBlendMode( BlendMode b ){
		int idx = 0;
		if ( b == BLEND_OPAQUE )        idx = 0;
		else if ( b == BLEND_LINEAR )   idx = 1;
		else                            idx = 2;
		if ( mCurrentBlendMode == idx ) return;
		mCurrentBlendMode = idx;
		applyBlendState();
	}

	void setTextureFilter( TextureFilter f ){
		int idx = ( f == TEXTURE_FILTER_POINT ) ? 0 : 1;
		if ( mCurrentSampler == idx ) return;
		mCurrentSampler = idx;
		applySamplerState();
	}

	void setCullMode( CullMode c ){
		int idx = 0;
		switch ( c ){
			case CULL_NONE:  idx = 0; break;
			case CULL_BACK:  idx = 1; break;
			case CULL_FRONT: idx = 2; break;
		}
		if ( mCurrentCullMode == idx ) return;
		mCurrentCullMode = idx;
		applyRasterizerState();
	}

	void setProjectionViewMatrix( const Matrix44& m ){
		if ( mProjectionViewMatrix != m ){
			mProjectionViewMatrix = m;
			mMatricesChanged = true;
		}
	}
	void setWorldMatrix( const Matrix34& m ){
		if ( mWorldMatrix != m ){
			mWorldMatrix = m;
			mMatricesChanged = true;
		}
	}

	void sendMatrices(){
		Matrix44 pvwm;
		pvwm.setMul( mProjectionViewMatrix, mWorldMatrix );
		// DX11: no half-pixel correction needed (pixel centers at 0.5,0.5 by spec)

		Matrix34 itwm;
		itwm.setInverse( mWorldMatrix );
		itwm.transpose33();

		memcpy( mVSCBData.transform,   &pvwm.m00,         sizeof( float ) * 16 );
		memcpy( mVSCBData.worldMatrix, &mWorldMatrix.m00, sizeof( float ) * 12 );
		memcpy( mVSCBData.itwMatrix,   &itwm.m00,         sizeof( float ) * 12 );

		mVSCBDirty = true;
		mMatricesChanged = false;
	}

	void sendLightingParameters(){
		// Compute light color * intensity
		Vector4 lc[4];
		for ( int i = 0; i < 4; ++i ){
			lc[ i ].x = mLightColors[ i ].x * mLightIntensities[ i ];
			lc[ i ].y = mLightColors[ i ].y * mLightIntensities[ i ];
			lc[ i ].z = mLightColors[ i ].z * mLightIntensities[ i ];
			lc[ i ].w = 0.f;
		}

		// --- VS cbuffer ---
		memcpy( mVSCBData.diffuseColor, &mDiffuseColor.x, 16 );
		memcpy( mVSCBData.ambientColor, &mAmbientColor.x, 16 );
		memcpy( mVSCBData.lightPosX,    &mLightPositionXs.x, 16 );
		memcpy( mVSCBData.lightPosY,    &mLightPositionYs.x, 16 );
		memcpy( mVSCBData.lightPosZ,    &mLightPositionZs.x, 16 );
		for ( int i = 0; i < 4; ++i ) memcpy( mVSCBData.lightColor[ i ], &lc[ i ].x, 16 );
		mVSCBDirty = true;

		// --- PS cbuffer (for pixel lighting) ---
		memcpy( mPSCBData.ambient,      &mAmbientColor.x,    16 );
		memcpy( mPSCBData.lightPosX,    &mLightPositionXs.x, 16 );
		memcpy( mPSCBData.lightPosY,    &mLightPositionYs.x, 16 );
		memcpy( mPSCBData.lightPosZ,    &mLightPositionZs.x, 16 );
		memcpy( mPSCBData.eyePosition,  &mEyePosition.x,     16 );
		{
			const float rcpPi2 = 1.f / ( 3.14159265f * 2.f );
			float f = ( 2.f + mSpecularColor.w ) * rcpPi2;
			Vector4 sc;
			sc.x = mSpecularColor.x * f;
			sc.y = mSpecularColor.y * f;
			sc.z = mSpecularColor.z * f;
			sc.w = mSpecularColor.w;
			memcpy( mPSCBData.specularColor, &sc.x, 16 );
		}
		for ( int i = 0; i < 4; ++i ) memcpy( mPSCBData.lightColor[ i ], &lc[ i ].x, 16 );
		memcpy( mPSCBData.emissionColor, &mEmissionColor.x, 16 );
		mPSCBDirty = true;

		mLightChanged = false;
	}

	void flushConstantBuffers(){
		if ( mVSCBDirty ){
			D3D11_MAPPED_SUBRESOURCE ms = {};
			mContext->Map( mVSCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms );
			memcpy( ms.pData, &mVSCBData, sizeof( mVSCBData ) );
			mContext->Unmap( mVSCB, 0 );
			mVSCBDirty = false;
		}
		if ( mPSCBDirty ){
			D3D11_MAPPED_SUBRESOURCE ms = {};
			mContext->Map( mPSCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms );
			memcpy( ms.pData, &mPSCBData, sizeof( mPSCBData ) );
			mContext->Unmap( mPSCB, 0 );
			mPSCBDirty = false;
		}
	}

	void draw( int offset, int primitiveNumber, PrimitiveType prim = PRIMITIVE_TRIANGLE ){
		if ( mLightChanged )    sendLightingParameters();
		if ( mMatricesChanged ) sendMatrices();
		STRONG_ASSERT( mCurrentVB && "VertexBuffer is not set." );
		if ( !mCurrentTexture ) setTexture( mWhiteTexture );
		D3D11_PRIMITIVE_TOPOLOGY topo = topologyOf( prim );
		mContext->IASetPrimitiveTopology( topo );
		int vpp = vertsPerPrim( prim );
		flushConstantBuffers();
		mContext->Draw( primitiveNumber * vpp, offset * vpp );
	}

	void drawIndexed( int offset, int primitiveNumber, PrimitiveType prim = PRIMITIVE_TRIANGLE ){
		if ( mLightChanged )    sendLightingParameters();
		if ( mMatricesChanged ) sendMatrices();
		STRONG_ASSERT( mCurrentIB && "IndexBuffer is not set." );
		STRONG_ASSERT( mCurrentVB && "VertexBuffer is not set." );
		if ( !mCurrentTexture ) setTexture( mWhiteTexture );
		D3D11_PRIMITIVE_TOPOLOGY topo = topologyOf( prim );
		mContext->IASetPrimitiveTopology( topo );
		int vpp = vertsPerPrim( prim );
		flushConstantBuffers();
		mContext->DrawIndexed( primitiveNumber * vpp, offset * vpp, 0 );
	}

	static D3D11_PRIMITIVE_TOPOLOGY topologyOf( PrimitiveType prim ){
		switch ( prim ){
			case PRIMITIVE_TRIANGLE: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			case PRIMITIVE_LINE:     return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
			case PRIMITIVE_POINT:    return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
			default: STRONG_ASSERT( 0 ); return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		}
	}
	static int vertsPerPrim( PrimitiveType prim ){
		switch ( prim ){
			case PRIMITIVE_TRIANGLE: return 3;
			case PRIMITIVE_LINE:     return 2;
			case PRIMITIVE_POINT:    return 1;
			default: return 3;
		}
	}

	void setViewport( int x, int y, int w, int h ){
		mViewport.TopLeftX = static_cast< float >( x );
		mViewport.TopLeftY = static_cast< float >( y );
		mViewport.Width    = static_cast< float >( w );
		mViewport.Height   = static_cast< float >( h );
		mViewport.MinDepth = 0.f;
		mViewport.MaxDepth = 1.f;

		// Scale viewport to back buffer (letterbox / pillarbox)
		float dw = static_cast< float >( mBackBufferWidth );
		float dh = static_cast< float >( mBackBufferHeight );
		float rw = static_cast< float >( mWidth );
		float rh = static_cast< float >( mHeight );
		float wRatio = dw / rw;
		float hRatio = dh / rh;
		float newX, newY, newW, newH;
		if ( wRatio > hRatio ){
			newX = ( dw - hRatio * rw ) * 0.5f + static_cast< float >( x ) * hRatio;
			newY = 0.f;
			newW = static_cast< float >( w ) * hRatio;
			newH = dh;
		}else{
			newX = 0.f;
			newY = ( dh - wRatio * rh ) * 0.5f + static_cast< float >( y ) * wRatio;
			newW = dw;
			newH = static_cast< float >( h ) * wRatio;
		}
		D3D11_VIEWPORT vp = {};
		vp.TopLeftX = newX;
		vp.TopLeftY = newY;
		vp.Width    = newW;
		vp.Height   = newH;
		vp.MinDepth = 0.f;
		vp.MaxDepth = 1.f;
		mContext->RSSetViewports( 1, &vp );
	}

	void getViewport( int* x, int* y, int* w, int* h ){
		if ( x ) *x = static_cast< int >( mViewport.TopLeftX );
		if ( y ) *y = static_cast< int >( mViewport.TopLeftY );
		if ( w ) *w = static_cast< int >( mViewport.Width );
		if ( h ) *h = static_cast< int >( mViewport.Height );
	}

	void blendToScreen( Texture::Impl* t ){
		setTexture( t );
		setBlendMode( BLEND_LINEAR );
		enableDepthTest( false );
		enableDepthWrite( false );
		Matrix44 pvm; pvm.setIdentity();
		setProjectionViewMatrix( pvm );
		Matrix34 wm; wm.setIdentity();
		setWorldMatrix( wm );
		setVertexBuffer( mFullScreenQuadVB );
		draw( 0, 1 );
		setTexture( 0 );
	}

	void enableFullScreen( bool f ){
		if ( f != mFullScreen ){
			mFullScreen = f;
			mCanRender  = false;
		}
	}

	void getPointerModifier( float* scale, Vector2* offset ){
		float dw = static_cast< float >( mBackBufferWidth );
		float dh = static_cast< float >( mBackBufferHeight );
		float rw = static_cast< float >( mWidth );
		float rh = static_cast< float >( mHeight );
		float wRatio = rw / dw;
		float hRatio = rh / dh;
		if ( wRatio > hRatio ){
			*scale     = wRatio;
			offset->x  = 0.f;
			offset->y  = ( dh - ( rh / wRatio ) ) * -0.5f;
		}else{
			*scale     = hRatio;
			offset->x  = ( dw - ( rw / hRatio ) ) * -0.5f;
			offset->y  = 0.f;
		}
	}

	void setLightPosition( int index, const Vector3& position ){
		mLightPositionXs[ index ] = position.x;
		mLightPositionYs[ index ] = position.y;
		mLightPositionZs[ index ] = position.z;
		mLightChanged = true;
	}
	void setLightIntensity( int index, float intensity ){
		mLightIntensities[ index ] = intensity;
		mLightChanged = true;
	}
	void setLightColor( int index, const Vector3& color ){
		mLightColors[ index ] = color;
		mLightChanged = true;
	}
	void setEyePosition( const Vector3& position ){
		mEyePosition.x = position.x;
		mEyePosition.y = position.y;
		mEyePosition.z = position.z;
		mLightChanged = true;
	}
	void setDiffuseColor( const Vector3& c ){
		mDiffuseColor.x = c.x; mDiffuseColor.y = c.y; mDiffuseColor.z = c.z;
		mLightChanged = true;
	}
	void setSpecularColor( const Vector3& c ){
		mSpecularColor.x = c.x; mSpecularColor.y = c.y; mSpecularColor.z = c.z;
		mLightChanged = true;
	}
	void setEmissionColor( const Vector3& c ){
		mEmissionColor.x = c.x; mEmissionColor.y = c.y; mEmissionColor.z = c.z;
		mLightChanged = true;
	}
	void setAmbientColor( const Vector3& c ){
		mAmbientColor.x = c.x; mAmbientColor.y = c.y; mAmbientColor.z = c.z;
		mLightChanged = true;
	}
	void setTransparency( float t ){
		mDiffuseColor.w = t;
		mLightChanged = true;
	}
	void setSpecularSharpness( float e ){
		mSpecularColor.w = e;
		mLightChanged = true;
	}
	void setLightingMode( LightingMode lm ){
		if ( lm != mLightingMode ){
			mLightingMode  = lm;
			mLightChanged  = true;
			applyShader();
		}
	}

	// Device / context accessors (used by Vertex/Index/Texture Impl constructors)
	ID3D11Device*        dxDevice()  { return mDevice; }
	ID3D11DeviceContext* dxContext() { return mContext; }

	// ---- Member variables ----
	ID3D11Device*           mDevice;
	ID3D11DeviceContext*    mContext;
	IDXGISwapChain*         mSwapChain;
	ID3D11RenderTargetView* mRTV;
	ID3D11DepthStencilView* mDSV;
	int mBackBufferWidth;
	int mBackBufferHeight;

	ID3D11VertexShader* mNoLightingVS;
	ID3D11VertexShader* mVertexLightingVS;
	ID3D11VertexShader* mPixelLightingVS;
	ID3D11PixelShader*  mNoLightingPS;
	ID3D11PixelShader*  mPixelLightingPS;
	ID3D11InputLayout*  mInputLayout;

	ID3D11RasterizerState*   mRSStates[3];
	ID3D11DepthStencilState* mDSSStates[4];
	ID3D11BlendState*        mBlendStates[3];
	ID3D11SamplerState*      mSamplerStates[2];

	ID3D11Buffer* mVSCB;
	ID3D11Buffer* mPSCB;
	VSCBufferData mVSCBData;
	PSCBufferData mPSCBData;
	bool mVSCBDirty;
	bool mPSCBDirty;

	int  mCurrentCullMode;
	bool mCurrentDepthTest;
	bool mCurrentDepthWrite;
	int  mCurrentBlendMode;
	int  mCurrentSampler;

	int          mWidth;
	int          mHeight;
	bool         mFullScreen;
	bool         mVSync;
	LightingMode mLightingMode;
	int          mFullScreenWindowWidth;
	int          mFullScreenWindowHeight;
	unsigned     mFrameId;
	bool         mCanRender;
	HWND         mWindowHandle;

	VertexBuffer::Impl* mFullScreenQuadVB;
	Texture::Impl*      mWhiteTexture;
	Texture::Impl*      mCurrentTexture;
	VertexBuffer::Impl* mCurrentVB;
	IndexBuffer::Impl*  mCurrentIB;

	D3D11_VIEWPORT mViewport;

	bool mLightChanged;
	bool mMatricesChanged;
	bool mPixelShaderReady;
	bool mCaptureRequest;
	std::string mCaptureFilename;

	Matrix44 mProjectionViewMatrix;
	Matrix34 mWorldMatrix;
	Vector4  mLightPositionXs;
	Vector4  mLightPositionYs;
	Vector4  mLightPositionZs;
	Vector3  mLightColors[4];
	float    mLightIntensities[4];
	Vector4  mEyePosition;
	Vector4  mDiffuseColor;
	Vector4  mSpecularColor;
	Vector4  mEmissionColor;
	Vector4  mAmbientColor;
};

extern ManagerImpl* gManagerImpl;

} //namespace Graphics
} //namespace GameLib

#endif
