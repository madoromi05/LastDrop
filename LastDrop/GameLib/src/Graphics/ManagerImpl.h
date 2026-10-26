#ifndef INCLUDED_GAMELIB_GRAPHICS_MANAGERIMPL_H
#define INCLUDED_GAMELIB_GRAPHICS_MANAGERIMPL_H

#include "Graphics/NoLightingVsHlsl.h"
#include "Graphics/NoLightingPsHlsl.h"
#include "Graphics/VertexLightingVsHlsl.h"
#include "Graphics/PixelLightingVsHlsl.h"
#include "Graphics/PixelLightingPsHlsl.h"
#include "Graphics/PixelLightingNormalMapPsHlsl.h"

#include "GameLib/Math/Functions.h"
#include "GameLib/Math/Matrix44.h"
#include "GameLib/Math/Matrix34.h"
#include "GameLib/Graphics/Enum.h"
#include "GameLib/Graphics/Manager.h"
#include "GameLib/Graphics/Texture.h"
#include "GameLib/Graphics/VertexBuffer.h"
#include "GameLib/Graphics/IndexBuffer.h"
#include "GameLib/Threading/Manager.h"
#include "GameLib/Profiling/Profiler.h"
#include "Graphics/TextureImpl.h"
#include "Graphics/ColorSpace.h"
#include "Graphics/VertexBufferImpl.h"
#include "Graphics/IndexBufferImpl.h"
#include "Graphics/RenderTargetImpl.h"
#include "Graphics/DepthTargetImpl.h"
#include "Graphics/ShaderImpl.h"
#include "Graphics/ConstantBufferImpl.h"

#include <d3dcompiler.h>
#include <cstdio>
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
	float colorParams[4];     // c19 (x: 1 なら頂点色をリニアへ直す。enableLinearColor  yzw: 未使用)
	// Total: 320 bytes
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
	float materialParams[4];  // c11 (x: 法線マップの強さ yzw: 未使用)
	// Total: 192 bytes
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
	mNormalMapPS( 0 ),
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
	mCurrentNormalMap( 0 ),
	mCurrentVB( 0 ),
	mCurrentIB( 0 ),
	mCurrentShader( 0 ),
	mTargetWidth( w ),
	mTargetHeight( h ),
	mClampSampler( 0 ),
	mShadowSampler( 0 ),
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
		mNormalMapStrength = 1.f; // 標準の凹凸。モデルごとの値は MeshMaterial 側から渡す
		mLinearColor = false;     // 今までどおり、選んだ色のまま計算する
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
		logAdapter( actualLevel );

		createRTVandDSV( w, h );

		// DXGI が alt+enter を横取りして排他フルスクリーンへ切り替えないようにする。
		// フルスクリーンは WindowCreator がボーダーレス（枠なしウィンドウをモニター全体へ）で行う。
		{
			IDXGIFactory* factory = nullptr;
			if ( SUCCEEDED( mSwapChain->GetParent( __uuidof( IDXGIFactory ), reinterpret_cast< void** >( &factory ) ) ) ){
				factory->MakeWindowAssociation( hwnd, DXGI_MWA_NO_ALT_ENTER );
				factory->Release();
			}
		}

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

		psBlob = compileShader( gPixelLightingNormalMapPsHlsl, "main", "ps_4_0" );
		hr = mDevice->CreatePixelShader( psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &mNormalMapPS );
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
		// ステートの作成に失敗すると null が残り、D3D11 の既定値（ブレンドなし等）で黙って描かれてしまうので、すべて結果を確かめる。
		D3D11_RASTERIZER_DESC rsDesc = {};
		rsDesc.FillMode              = D3D11_FILL_SOLID;
		rsDesc.FrontCounterClockwise = TRUE;  // match DX9 CCW = front face
		rsDesc.DepthClipEnable       = TRUE;
		rsDesc.CullMode = D3D11_CULL_NONE;  checkCreated( mDevice->CreateRasterizerState( &rsDesc, &mRSStates[0] ), "RasterizerState(CULL_NONE)" );
		rsDesc.CullMode = D3D11_CULL_BACK;  checkCreated( mDevice->CreateRasterizerState( &rsDesc, &mRSStates[1] ), "RasterizerState(CULL_BACK)" );
		rsDesc.CullMode = D3D11_CULL_FRONT; checkCreated( mDevice->CreateRasterizerState( &rsDesc, &mRSStates[2] ), "RasterizerState(CULL_FRONT)" );

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
			checkCreated( mDevice->CreateDepthStencilState( &dssDesc, &mDSSStates[ i ] ), "DepthStencilState" );
		}

		// --- Create blend states ---
		// [0]=Opaque, [1]=Linear, [2]=Add
		// 0 番の設定を 8 枚分すべてに入れる。IndependentBlendEnable が FALSE なら 1〜7 番は使われないはずだが、
		// 0 埋めのままだと SrcBlend などが不正な値（0）になるので、ドライバーに不正な設定を渡さないようにしておく。
		D3D11_RENDER_TARGET_BLEND_DESC rtBlend = {};
		rtBlend.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		rtBlend.SrcBlend       = D3D11_BLEND_ONE;
		rtBlend.DestBlend      = D3D11_BLEND_ZERO;
		rtBlend.BlendOp        = D3D11_BLEND_OP_ADD;
		rtBlend.SrcBlendAlpha  = D3D11_BLEND_ONE;
		rtBlend.DestBlendAlpha = D3D11_BLEND_ZERO;
		rtBlend.BlendOpAlpha   = D3D11_BLEND_OP_ADD;
		// Opaque (no blending)
		rtBlend.BlendEnable = FALSE;
		createBlendState( rtBlend, &mBlendStates[0], "BlendState(OPAQUE)" );
		// Linear
		rtBlend.BlendEnable = TRUE;
		rtBlend.SrcBlend    = D3D11_BLEND_SRC_ALPHA;
		rtBlend.DestBlend   = D3D11_BLEND_INV_SRC_ALPHA;
		createBlendState( rtBlend, &mBlendStates[1], "BlendState(LINEAR)" );
		// Additive
		rtBlend.DestBlend = D3D11_BLEND_ONE;
		createBlendState( rtBlend, &mBlendStates[2], "BlendState(ADDITIVE)" );

		// --- Create sampler states ---
		D3D11_SAMPLER_DESC sampDesc = {};
		sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
		// Point
		sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		checkCreated( mDevice->CreateSamplerState( &sampDesc, &mSamplerStates[0] ), "SamplerState(POINT)" );
		// Linear / Anisotropic
		sampDesc.Filter    = D3D11_FILTER_ANISOTROPIC;
		sampDesc.MaxAnisotropy = 16;
		checkCreated( mDevice->CreateSamplerState( &sampDesc, &mSamplerStates[1] ), "SamplerState(ANISOTROPIC)" );

		// 自作シェーダ用（s1: 端で止める線形。画面の写しを読むとき、端で反対側の色が混ざらないように）
		D3D11_SAMPLER_DESC clampDesc = {};
		clampDesc.Filter         = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		clampDesc.AddressU       = D3D11_TEXTURE_ADDRESS_CLAMP;
		clampDesc.AddressV       = D3D11_TEXTURE_ADDRESS_CLAMP;
		clampDesc.AddressW       = D3D11_TEXTURE_ADDRESS_CLAMP;
		clampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		clampDesc.MaxLOD         = D3D11_FLOAT32_MAX;
		hr = mDevice->CreateSamplerState( &clampDesc, &mClampSampler );
		STRONG_ASSERT( SUCCEEDED( hr ) );
		// 自作シェーダ用（s2: 影の比較。SampleCmp で「比べる深度 <= マップの深度」の割合を4画素の線形補間で返す）
		// マップの外は深度1（一番奥）として扱い、影にならないようにする。
		D3D11_SAMPLER_DESC shadowDesc = {};
		shadowDesc.Filter         = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
		shadowDesc.AddressU       = D3D11_TEXTURE_ADDRESS_BORDER;
		shadowDesc.AddressV       = D3D11_TEXTURE_ADDRESS_BORDER;
		shadowDesc.AddressW       = D3D11_TEXTURE_ADDRESS_BORDER;
		shadowDesc.BorderColor[0] = 1.f;
		shadowDesc.BorderColor[1] = 1.f;
		shadowDesc.BorderColor[2] = 1.f;
		shadowDesc.BorderColor[3] = 1.f;
		shadowDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
		shadowDesc.MaxLOD         = D3D11_FLOAT32_MAX;
		hr = mDevice->CreateSamplerState( &shadowDesc, &mShadowSampler );
		STRONG_ASSERT( SUCCEEDED( hr ) );

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
		setShader( 0 );
		if ( mClampSampler )  { mClampSampler->Release();  mClampSampler = 0; }
		if ( mShadowSampler ) { mShadowSampler->Release(); mShadowSampler = 0; }

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
		if ( mNormalMapPS )       { mNormalMapPS->Release();       mNormalMapPS = 0; }
		if ( mVSCB )              { mVSCB->Release();              mVSCB = 0; }
		if ( mPSCB )              { mPSCB->Release();              mPSCB = 0; }

		destroyRTVandDSV();
		if ( mSwapChain ) { mSwapChain->Release(); mSwapChain = 0; }
		if ( mContext )   { mContext->Release();   mContext = 0; }
		if ( mDevice )    { mDevice->Release();    mDevice = 0; }
	}

	// ---- State creation helpers ----
	// 作成に失敗したら、何が失敗したかを出力ウィンドウへ出してから止める（Release でも止まる）。
	static void checkCreated( HRESULT hr, const char* what ){
		if ( FAILED( hr ) ){
			char code[ 16 ];
			sprintf_s( code, "0x%08X", static_cast< unsigned >( hr ) ); // cout の hex は元に戻せないので文字列にして出す
			cout << "Graphics::Manager : failed to create " << what << " hr=" << code << endl;
		}
		STRONG_ASSERT( SUCCEEDED( hr ) && "Graphics::Manager : failed to create a pipeline state. See the output window." );
	}
	// 1 枚分のブレンド設定を 8 枚すべてに入れて作る。
	void createBlendState( const D3D11_RENDER_TARGET_BLEND_DESC& rtBlend, ID3D11BlendState** state, const char* what ){
		D3D11_BLEND_DESC desc = {};
		desc.AlphaToCoverageEnable  = FALSE;
		desc.IndependentBlendEnable = FALSE;
		for ( int i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i ){
			desc.RenderTarget[ i ] = rtBlend;
		}
		checkCreated( mDevice->CreateBlendState( &desc, state ), what );
	}
	// 実際に使われている GPU と機能レベルを出す（GPU が2つあるノートPCでは内蔵 GPU が選ばれることがある）。
	void logAdapter( D3D_FEATURE_LEVEL level ){
		IDXGIDevice* dxgiDevice = nullptr;
		IDXGIAdapter* adapter = nullptr;
		if ( SUCCEEDED( mDevice->QueryInterface( __uuidof( IDXGIDevice ), reinterpret_cast< void** >( &dxgiDevice ) ) ) ){
			if ( SUCCEEDED( dxgiDevice->GetAdapter( &adapter ) ) ){
				DXGI_ADAPTER_DESC desc = {};
				if ( SUCCEEDED( adapter->GetDesc( &desc ) ) ){
					char name[ 256 ] = {};
					WideCharToMultiByte( CP_ACP, 0, desc.Description, -1, name, sizeof( name ) - 1, nullptr, nullptr );
					cout << "Graphics::Manager : adapter = " << name << endl;
				}
				adapter->Release();
			}
			dxgiDevice->Release();
		}
		char levelText[ 16 ];
		sprintf_s( levelText, "0x%X", static_cast< unsigned >( level ) ); // 0xb000 = 11_0, 0xa100 = 10_1, 0xa000 = 10_0
		cout << "Graphics::Manager : feature level = " << levelText << endl;
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
		mContext->PSSetSamplers( Manager::CLAMP_SAMPLER_SLOT, 1, &mClampSampler );
		mContext->PSSetSamplers( Manager::SHADOW_SAMPLER_SLOT, 1, &mShadowSampler );

		bindBackBuffer();

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
		// 自作シェーダが付いていればそれを使う（ライティングの種類・法線マップの有無より優先）。
		if ( mCurrentShader && mCurrentShader->isValid() ){
			mContext->IASetInputLayout( mCurrentShader->mLayout );
			mContext->VSSetShader( mCurrentShader->mVS, nullptr, 0 );
			mContext->PSSetShader( mCurrentShader->mPS, nullptr, 0 );
			return;
		}
		mContext->IASetInputLayout( mInputLayout );
		if ( mLightingMode == LIGHTING_NONE ){
			mContext->VSSetShader( mNoLightingVS, nullptr, 0 );
			mContext->PSSetShader( mNoLightingPS, nullptr, 0 );
		}else if ( mLightingMode == LIGHTING_PER_VERTEX ){
			mContext->VSSetShader( mVertexLightingVS, nullptr, 0 );
			mContext->PSSetShader( mNoLightingPS, nullptr, 0 );
		}else{
			mContext->VSSetShader( mPixelLightingVS, nullptr, 0 );
			// 法線マップが設定されていればノーマルマップ用PSを使う。
			mContext->PSSetShader( mCurrentNormalMap ? mNormalMapPS : mPixelLightingPS, nullptr, 0 );
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

	// バックバッファをウィンドウの描画領域と同じ大きさに作り直す（最大化・端のドラッグ・フルスクリーンの後）。
	// ゲームの座標は高さ（mHeight）を固定し、幅（mWidth）だけをウィンドウの縦横比に合わせて変える。
	// 高さを固定するのは、2D（文字など）の大きさが画面の高さに対して一定になり、横長の画面では左右に広く見えるだけにするため。
	void fitBackBufferToWindow(){
		RECT client;
		GetClientRect( mWindowHandle, &client );
		int w = client.right - client.left;
		int h = client.bottom - client.top;
		if ( w <= 0 || h <= 0 ){
			return; // 最小化中は大きさが0になる。作り直さず、戻ったときの大きさで作る
		}
		if ( w == mBackBufferWidth && h == mBackBufferHeight ){
			return;
		}
		// ResizeBuffers の前に、バックバッファを指しているビューを全部外す
		mContext->OMSetRenderTargets( 0, nullptr, nullptr );
		destroyRTVandDSV();
		HRESULT hr = mSwapChain->ResizeBuffers( 1, static_cast< UINT >( w ), static_cast< UINT >( h ),
			DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH );
		if ( FAILED( hr ) ){
			cout << "Graphics::fitBackBufferToWindow() : ResizeBuffers failed" << endl;
			// 失敗しても今の大きさのまま作り直して描き続ける
			DXGI_SWAP_CHAIN_DESC desc;
			mSwapChain->GetDesc( &desc );
			w = static_cast< int >( desc.BufferDesc.Width );
			h = static_cast< int >( desc.BufferDesc.Height );
		}
		createRTVandDSV( w, h );
		setInitialStates();
		fitLogicalWidthToBackBuffer();
	}

	// ゲームの座標の幅を、バックバッファと同じ縦横比になるよう求め直す（左右に黒帯が出ないようにする）。
	// 画面全体の描画範囲（mViewport）も新しい幅に広げる。部分的な描画範囲を設定していても、大きさが変わったら全体に戻る。
	void fitLogicalWidthToBackBuffer(){
		float aspect = static_cast< float >( mBackBufferWidth ) / static_cast< float >( mBackBufferHeight );
		mWidth = static_cast< int >( static_cast< float >( mHeight ) * aspect + 0.5f );
		mViewport.TopLeftX = 0.f;
		mViewport.TopLeftY = 0.f;
		mViewport.Width    = static_cast< float >( mWidth );
		mViewport.Height   = static_cast< float >( mHeight );
	}

	void beginDraw(){
		fitBackBufferToWindow();
		float clearColor[4] = { 0.f, 0.f, 0.f, 1.f };
		bindBackBuffer();
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
		// Present の呼び出しだけを測る（capture も GPU を待つが、頼んだときだけなので混ぜない。計画書 2 章）。
		Profiling::beginPresent();
		HRESULT hr = mSwapChain->Present( mVSync ? 1 : 0, 0 );
		Profiling::endPresent();
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

	// 法線マップを解除して通常PSへ戻す（設定されていなければ何もしない）。
	void clearNormalMap(){
		if ( !mCurrentNormalMap ) return;
		mCurrentNormalMap->release();
		if ( mCurrentNormalMap->referenceCount() == 0 ) SAFE_DELETE( mCurrentNormalMap );
		mCurrentNormalMap = 0;
		ID3D11ShaderResourceView* nullSRV = nullptr;
		mContext->PSSetShaderResources( 1, 1, &nullSRV );
		applyShader();
	}
	// スロット t1 に法線マップをバインドし、ノーマルマップ用PSへ切り替える。
	void setNormalMap( Texture::Impl* o ){
		if ( mCurrentNormalMap == o ) return;
		if ( mCurrentNormalMap ){
			mCurrentNormalMap->release();
			if ( mCurrentNormalMap->referenceCount() == 0 ) SAFE_DELETE( mCurrentNormalMap );
		}
		mCurrentNormalMap = o;
		if ( o ){
			o->refer();
			o->ensureGpuUploaded();
			mContext->PSSetShaderResources( 1, 1, &o->mSRV );
		}else{
			ID3D11ShaderResourceView* nullSRV = nullptr;
			mContext->PSSetShaderResources( 1, 1, &nullSRV );
		}
		applyShader();
	}

	void setTexture( Texture::Impl* o ){
		clearNormalMap(); // 既定で法線マップを解除。必要なら直後に setNormalMap する
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
			UINT stride = static_cast< UINT >( o->mStride ); // 頂点の形はバッファごとに違う（VertexFormat）
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

	// enableLinearColor の間は、選んだ色（画面の色）をリニアへ直して渡す。アルファ・鋭さ（w）は色ではないのでそのまま。
	Vector4 toSceneColor( const Vector4& c ) const {
		if ( !mLinearColor ) return c;
		Vector3 linear = ColorSpace::srgbToLinear( Vector3( c.x, c.y, c.z ) );
		return Vector4( linear.x, linear.y, linear.z, c.w );
	}
	Vector3 toSceneColor( const Vector3& c ) const {
		return mLinearColor ? ColorSpace::srgbToLinear( c ) : c;
	}

	void sendLightingParameters(){
		// 色だけを直してから強さを掛ける（強さ込みで直すと、強さの意味が変わるため）。
		Vector4 lc[4];
		for ( int i = 0; i < 4; ++i ){
			Vector3 color = toSceneColor( mLightColors[ i ] );
			lc[ i ].x = color.x * mLightIntensities[ i ];
			lc[ i ].y = color.y * mLightIntensities[ i ];
			lc[ i ].z = color.z * mLightIntensities[ i ];
			lc[ i ].w = 0.f;
		}
		const Vector4 diffuse  = toSceneColor( mDiffuseColor );
		const Vector4 ambient  = toSceneColor( mAmbientColor );
		const Vector4 specular = toSceneColor( mSpecularColor );
		const Vector4 emission = toSceneColor( mEmissionColor );

		// --- VS cbuffer ---
		memcpy( mVSCBData.diffuseColor, &diffuse.x, 16 );
		memcpy( mVSCBData.ambientColor, &ambient.x, 16 );
		memcpy( mVSCBData.lightPosX,    &mLightPositionXs.x, 16 );
		memcpy( mVSCBData.lightPosY,    &mLightPositionYs.x, 16 );
		memcpy( mVSCBData.lightPosZ,    &mLightPositionZs.x, 16 );
		for ( int i = 0; i < 4; ++i ) memcpy( mVSCBData.lightColor[ i ], &lc[ i ].x, 16 );
		mVSCBData.colorParams[ 0 ] = mLinearColor ? 1.f : 0.f;
		mVSCBDirty = true;

		// --- PS cbuffer (for pixel lighting) ---
		memcpy( mPSCBData.ambient,      &ambient.x,          16 );
		memcpy( mPSCBData.lightPosX,    &mLightPositionXs.x, 16 );
		memcpy( mPSCBData.lightPosY,    &mLightPositionYs.x, 16 );
		memcpy( mPSCBData.lightPosZ,    &mLightPositionZs.x, 16 );
		memcpy( mPSCBData.eyePosition,  &mEyePosition.x,     16 );
		{
			const float rcpPi2 = 1.f / ( 3.14159265f * 2.f );
			float f = ( 2.f + specular.w ) * rcpPi2;
			Vector4 sc;
			sc.x = specular.x * f;
			sc.y = specular.y * f;
			sc.z = specular.z * f;
			sc.w = specular.w;
			memcpy( mPSCBData.specularColor, &sc.x, 16 );
		}
		for ( int i = 0; i < 4; ++i ) memcpy( mPSCBData.lightColor[ i ], &lc[ i ].x, 16 );
		memcpy( mPSCBData.emissionColor, &emission.x, 16 );
		mPSCBData.materialParams[ 0 ] = mNormalMapStrength;
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

		// Scale viewport to the current target (letterbox / pillarbox)
		// 今の描画先（普段はバックバッファ、setRenderTargets 中はその描画先）の大きさへ写す。
		float dw = static_cast< float >( mTargetWidth );
		float dh = static_cast< float >( mTargetHeight );
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
	void setNormalMapStrength( float s ){
		mNormalMapStrength = s;
		mLightChanged = true; // PS定数バッファは sendLightingParameters でまとめて詰める
	}
	void enableLinearColor( bool f ){
		if ( f != mLinearColor ){
			mLinearColor  = f;
			mLightChanged = true; // 色の定数を直し直す（sendLightingParameters）
		}
	}
	void setLightingMode( LightingMode lm ){
		if ( lm != mLightingMode ){
			mLightingMode  = lm;
			mLightChanged  = true;
			applyShader();
		}
	}

	// ---- 描画先・自作シェーダ（A-1 で追加）----

	// バックバッファへ描く状態に戻す（描画範囲の写し先もバックバッファの大きさにする）。
	void bindBackBuffer(){
		mContext->OMSetRenderTargets( 1, &mRTV, mDSV );
		mTargetWidth  = mBackBufferWidth;
		mTargetHeight = mBackBufferHeight;
	}

	// 自作のテクスチャ番号（t2〜）を全部外す。描画先を切り替える前に呼ぶ。
	// 外さずに、読み込み中の物を描画先にすると、GPU が読み込み側を勝手に外して警告を出すため。
	void unbindUserShaderResources(){
		ID3D11ShaderResourceView* nulls[ Manager::MAX_TEXTURE_SLOT ] = {};
		int n = Manager::MAX_TEXTURE_SLOT - Manager::FIRST_USER_TEXTURE_SLOT;
		mContext->PSSetShaderResources( Manager::FIRST_USER_TEXTURE_SLOT, static_cast< UINT >( n ), nulls );
	}

	// colors（count 枚、0枚可）と depth（nullptr 可）へ描く。描画範囲は描画先の全体にする。
	void setRenderTargets( RenderTarget::Impl* const* colors, int count, DepthTarget::Impl* depth ){
		ASSERT( count >= 0 && count <= Manager::MAX_RENDER_TARGET && "Graphics::Manager : too many render targets." );
		ASSERT( ( count > 0 || depth ) && "Graphics::Manager : no render target." );
		unbindUserShaderResources();
		ID3D11RenderTargetView* rtvs[ Manager::MAX_RENDER_TARGET ] = {};
		int w = 0;
		int h = 0;
		for ( int i = 0; i < count; ++i ){
			ASSERT( colors[ i ] && "Graphics::Manager : empty RenderTarget." );
			colors[ i ]->fitToScreen( mBackBufferWidth, mBackBufferHeight );
			rtvs[ i ] = colors[ i ]->mRTV;
			if ( i == 0 ){
				w = colors[ i ]->mWidth;
				h = colors[ i ]->mHeight;
			}
			ASSERT( colors[ i ]->mWidth == w && colors[ i ]->mHeight == h && "Graphics::Manager : render targets differ in size." );
		}
		ID3D11DepthStencilView* dsv = 0;
		if ( depth ){
			depth->fitToScreen( mBackBufferWidth, mBackBufferHeight );
			dsv = depth->mDSV;
			if ( count == 0 ){
				w = depth->mWidth;
				h = depth->mHeight;
			}
			ASSERT( depth->mWidth == w && depth->mHeight == h && "Graphics::Manager : depth target differs in size." );
		}
		mContext->OMSetRenderTargets( static_cast< UINT >( count ), rtvs, dsv );
		mTargetWidth  = w;
		mTargetHeight = h;
		D3D11_VIEWPORT vp = {};
		vp.Width    = static_cast< float >( w );
		vp.Height   = static_cast< float >( h );
		vp.MaxDepth = 1.f;
		mContext->RSSetViewports( 1, &vp );
	}

	// バックバッファへ戻し、描画範囲を setViewport で決めた範囲（論理座標）に戻す。
	void resetRenderTargets(){
		bindBackBuffer();
		setViewport(
			static_cast< int >( mViewport.TopLeftX ),
			static_cast< int >( mViewport.TopLeftY ),
			static_cast< int >( mViewport.Width ),
			static_cast< int >( mViewport.Height ) );
	}

	void clear( RenderTarget::Impl* t, const Vector4& color ){
		ASSERT( t && "Graphics::Manager : empty RenderTarget." );
		t->fitToScreen( mBackBufferWidth, mBackBufferHeight );
		const float c[ 4 ] = { color.x, color.y, color.z, color.w };
		mContext->ClearRenderTargetView( t->mRTV, c );
	}
	void clear( DepthTarget::Impl* t ){
		ASSERT( t && "Graphics::Manager : empty DepthTarget." );
		t->fitToScreen( mBackBufferWidth, mBackBufferHeight );
		const float kFarthestDepth = 1.f;
		mContext->ClearDepthStencilView( t->mDSV, D3D11_CLEAR_DEPTH, kFarthestDepth, 0 );
	}

	void setShaderResource( int slot, ID3D11ShaderResourceView* srv ){
		ASSERT( slot >= Manager::FIRST_USER_TEXTURE_SLOT && slot < Manager::MAX_TEXTURE_SLOT && "Graphics::Manager : texture slot out of range." );
		mContext->PSSetShaderResources( static_cast< UINT >( slot ), 1, &srv );
	}
	void setShaderResource( int slot, RenderTarget::Impl* t ){
		if ( t ){
			t->fitToScreen( mBackBufferWidth, mBackBufferHeight );
		}
		setShaderResource( slot, t ? t->mSRV : 0 );
	}
	void setShaderResource( int slot, DepthTarget::Impl* t ){
		if ( t ){
			t->fitToScreen( mBackBufferWidth, mBackBufferHeight );
		}
		setShaderResource( slot, t ? t->mSRV : 0 );
	}
	void setShaderResource( int slot, Texture::Impl* t ){
		if ( t ){
			t->ensureGpuUploaded();
		}
		setShaderResource( slot, t ? t->mSRV : 0 );
	}

	void setConstantBuffer( int slot, ConstantBuffer::Impl* b ){
		ASSERT( slot >= Manager::FIRST_USER_CONSTANT_SLOT && slot < Manager::MAX_CONSTANT_SLOT && "Graphics::Manager : constant buffer slot out of range." );
		ID3D11Buffer* buffer = b ? b->mBuffer : 0;
		mContext->VSSetConstantBuffers( static_cast< UINT >( slot ), 1, &buffer );
		mContext->PSSetConstantBuffers( static_cast< UINT >( slot ), 1, &buffer );
	}

	// 自作シェーダを付ける（0 で固定パイプラインへ戻る）。Debug ビルドでは、ここでファイルの変更を見て作り直す。
	void setShader( Shader::Impl* o ){
#ifdef _DEBUG
		if ( o ){
			reloadIfSourceChanged( o );
		}
#endif
		if ( mCurrentShader != o ){
			if ( mCurrentShader ){
				mCurrentShader->release();
				if ( mCurrentShader->referenceCount() == 0 ) SAFE_DELETE( mCurrentShader );
			}
			mCurrentShader = o;
			if ( o ){
				o->refer();
			}
		}
		applyShader();
	}
	// Shader::reload の後に呼ばれる。今付いているシェーダなら、作り直した物を付け直す。
	void onShaderRebuilt( Shader::Impl* o ){
		if ( mCurrentShader == o ){
			applyShader();
		}
	}
	void reloadIfSourceChanged( Shader::Impl* o ){
		// ファイルの時刻を見るのは数フレームに1回にする（毎回 OS に問い合わせると、描画1回に何度も呼ぶとき重い）。
		const unsigned kCheckIntervalFrames = 30;
		if ( mFrameId - o->mLastCheckFrame < kCheckIntervalFrames ){
			return;
		}
		o->mLastCheckFrame = mFrameId;
		if ( o->isSourceChanged() ){
			o->compile();
		}
	}

	// 頂点バッファを使わず、画面（今の描画範囲）全体を三角形1枚で描く。今付いている自作シェーダで描く。
	// VSMain は SV_VertexID（0,1,2）から位置を作ること。深度テスト・書き込みと裏面カリングはこの描画の間だけ切る。
	void drawFullScreen(){
		ASSERT( mCurrentShader && "Graphics::Manager : drawFullScreen needs a Shader." );
		if ( !mCurrentShader || !mCurrentShader->isValid() ){
			return; // 一度もコンパイルに成功していない。固定パイプラインでは全画面を描けないので描かない
		}
		if ( mLightChanged )    sendLightingParameters();
		if ( mMatricesChanged ) sendMatrices();
		flushConstantBuffers();
		const int kFullScreenVertexNumber = 3;
		const int kNoDepthIndex = 3; // mDSSStates の「テストしない・書かない」
		const int kNoCullIndex = 0;  // mRSStates の CULL_NONE
		mContext->IASetInputLayout( nullptr );
		mContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
		mContext->OMSetDepthStencilState( mDSSStates[ kNoDepthIndex ], 0 );
		mContext->RSSetState( mRSStates[ kNoCullIndex ] );
		mContext->Draw( kFullScreenVertexNumber, 0 );
		applyDepthStencilState();
		applyRasterizerState();
		applyShader(); // InputLayout を戻す
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
	ID3D11PixelShader*  mNormalMapPS;
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
	Texture::Impl*      mCurrentNormalMap;
	VertexBuffer::Impl* mCurrentVB;
	IndexBuffer::Impl*  mCurrentIB;
	Shader::Impl*       mCurrentShader; // 自作シェーダ。0 なら固定パイプライン
	int mTargetWidth;  // 今の描画先の大きさ（画素）。setViewport の写し先
	int mTargetHeight;
	ID3D11SamplerState* mClampSampler;  // s1
	ID3D11SamplerState* mShadowSampler; // s2

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
	float    mNormalMapStrength; // 法線マップの凹凸の強さ(0で無効、1で標準)
	bool     mLinearColor;       // 色をリニアへ直して計算するか（enableLinearColor）。HUD など画面へ直接描く間は false
};

extern ManagerImpl* gManagerImpl;

} //namespace Graphics
} //namespace GameLib

#endif
