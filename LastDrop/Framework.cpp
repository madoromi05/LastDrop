#include "GameLib/GameLib.h"
#include "GameLib/WindowCreator/WindowCreator.h"
#include "GameLib/Input/Manager.h"
#include "GameLib/FileIO/Manager.h"
#include "GameLib/Sound/Manager.h"
#include "GameLib/Graphics/Manager.h"
#include "GameLib/Graphics/Texture.h"
#include "GameLib/Math/Vector2.h"
#include "GameLib/Math/Random.h"
#include "GameLib/Scene/StringRenderer.h"
#include "GameLib/Scene/Font.h"
#include "GameLib/Threading/Functions.h"
#include "GameLib/Threading/Manager.h"
#include "GameLib/Scene/PrimitiveRenderer.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Vector4.h"
#include "GameLib/Base/Array.h"

#include "Framework.h"

#include <cstdio>
#include <string>
#include <vector>
using namespace std;

namespace GameLib{
namespace {

class Impl{
public:
	Impl() :
	mWidth( 640 ),
	mHeight( 480 ),
	mFullScreen( false ),
	mFullScreenForbidden( true ),
	mVSync( true ),
	mAntiAlias( false ),
	mTitle( "DirectX11 Turn Command Battle" ),
	mArchiveNames( 0 ),
	mArchiveNumber( 0 ),
	mLoadMode( FileIO::Manager::MODE_DIRECT_FIRST ),
	mPreviousFrameInterval( 0 ),
	mPreviousFrameTime( 0 ),
	mFrameRate( 0 ),
	mIdealFrameInterval( 0 ),
	mEndRequested( false ),
	mStarted( false ){
		mRandom = Random::create();
		cout.begin();
		unsigned t = time();
		for ( int i = 0; i < TIME_HISTORY_SIZE; ++i ){
			mTimeHistory[ i ] = t;
		}
	}
	~Impl(){
		mPrimitiveRenderer.release();
		for ( int i = 0; i < TEXTURE_NUMBER; ++i ){
			mTextures[ i ].release();
		}
		if ( mArchiveNames ){
			SAFE_DELETE_ARRAY( mArchiveNames );
		}
		mDebugStringRenderer.release();
		mDebugFont.release();
		Graphics::Manager::destroy();
		Input::Manager::destroy();
		Sound::Manager::destroy();
		FileIO::Manager::destroy();
		Threading::Manager::destroy();
		cout.end();
	}

	void start( void* windowHandle ){
		Threading::Manager::create( 0 );
		if ( mArchiveNumber > 0 ){
			Array< const char* > names( mArchiveNumber );
			for ( int i = 0; i < mArchiveNumber; ++i ){
				names[ i ] = mArchiveNames[ i ].c_str();
			}
			FileIO::Manager::create( &names[ 0 ], mArchiveNumber, mLoadMode );
			SAFE_DELETE_ARRAY( mArchiveNames );
			mArchiveNumber = 0;
		}else{
			FileIO::Manager::create();
		}
		Sound::Manager::create( windowHandle );
		Input::Manager::create( windowHandle );
		Graphics::Manager::create( windowHandle, mWidth, mHeight, mFullScreen, mVSync, mAntiAlias );

		Scene::Font::BitmapChar chars[158];
		int pos = 0;
		for ( int i = 0x20; i <= 0x7E; ++i ){
			Scene::Font::BitmapChar& c = chars[ pos ];
			c.mCode        = static_cast< unsigned short >( i );
			c.mX           = static_cast< unsigned short >( ( i % 16 ) * 8 );
			c.mY           = static_cast< unsigned short >( ( i / 16 ) * 16 );
			c.mTexturePage = 0;
			c.mWidth       = 8;
			c.mHeight      = 16;
			++pos;
		}
		for ( int i = 0xA1; i <= 0xDF; ++i ){
			Scene::Font::BitmapChar& c = chars[ pos ];
			c.mCode        = static_cast< unsigned short >( i );
			c.mX           = static_cast< unsigned short >( ( i % 16 ) * 8 );
			c.mY           = static_cast< unsigned short >( ( i / 16 ) * 16 );
			c.mTexturePage = 0;
			c.mWidth       = 8;
			c.mHeight      = 16;
			++pos;
		}
		ASSERT( pos == 158 );

		{
			FILE* fp = nullptr;
			fopen_s( &fp, "Resource/font.tga", "rb" );
			ASSERT( fp && "Resource/font.tga not found." );
			if ( fp ){
				fseek( fp, 0, SEEK_END );
				int textureSize = static_cast< int >( ftell( fp ) );
				rewind( fp );
				std::vector< char > fontData( textureSize );
				fread( fontData.data(), 1, textureSize, fp );
				fclose( fp );
				const char* fontDataArray[ 1 ] = { fontData.data() };
				mDebugFont = Scene::Font::create( fontDataArray, &textureSize, 1, chars, 158 );
			}
		}
		mDebugStringRenderer = Scene::StringRenderer::create( 2048, 128 );
		mDebugStringRenderer.setFont( mDebugFont );
		Graphics::Manager().setTextureFilter( Graphics::TEXTURE_FILTER_POINT );
		mPrimitiveRenderer = Scene::PrimitiveRenderer::create( 100000, 10000 );

		mStarted = true;
	}
	unsigned time() const {
		return WindowCreator().time();
	}
	void preUpdate(){
		unsigned currentTime = time();
		while ( ( currentTime - mPreviousFrameTime ) < mIdealFrameInterval ){
			Threading::sleep( 1 );
			currentTime = time();
		}
		mPreviousFrameTime = currentTime;

		mPreviousFrameInterval = currentTime - mTimeHistory[ TIME_HISTORY_SIZE - 1 ];
		unsigned frameIntervalSum = currentTime - mTimeHistory[ 0 ];
		mFrameRate = TIME_HISTORY_SIZE * 1000 / frameIntervalSum;
		for ( int i = 0; i < TIME_HISTORY_SIZE - 1; ++i ){
			mTimeHistory[ i ] = mTimeHistory[ i + 1 ];
		}
		mTimeHistory[ TIME_HISTORY_SIZE - 1 ] = currentTime;

		Graphics::Manager().beginDraw();
		float pointerScale;
		Vector2 pointerOffset;
		Graphics::Manager().getPointerModifier( &pointerScale, &pointerOffset );
		Input::Manager().update( pointerScale, pointerOffset );
		mPrimitiveRenderer.enableDepthTest( false );
		mPrimitiveRenderer.enableDepthWrite( true );
		mPrimitiveRenderer.setCullMode( Graphics::CULL_NONE );
		mPrimitiveRenderer.setBlendMode( Graphics::BLEND_OPAQUE );
	}
	void postUpdate(){
		mPrimitiveRenderer.draw();
		mDebugStringRenderer.draw();
		Graphics::Manager().endDraw();
	}
	void setBlendMode( Framework::BlendMode a ){
		Graphics::BlendMode b = Graphics::BLEND_OPAQUE;
		switch ( a ){
			case Framework::BLEND_LINEAR:   b = Graphics::BLEND_LINEAR;   break;
			case Framework::BLEND_ADDITIVE: b = Graphics::BLEND_ADDITIVE; break;
			case Framework::BLEND_OPAQUE:   b = Graphics::BLEND_OPAQUE;   break;
			default: ASSERT( false ); break;
		}
		mPrimitiveRenderer.setBlendMode( b );
	}
	void drawTriangle3D(
	const double* p0, const double* p1, const double* p2,
	const double* t0, const double* t1, const double* t2,
	unsigned c0, unsigned c1, unsigned c2 ){
		Vector3 p[ 3 ];
		Vector2 t[ 3 ];
		for ( int i = 0; i < 3; ++i ){
			p[ 0 ][ i ] = static_cast< float >( p0[ i ] );
			p[ 1 ][ i ] = static_cast< float >( p1[ i ] );
			p[ 2 ][ i ] = static_cast< float >( p2[ i ] );
		}
		if ( t0 ){ t[ 0 ].x = static_cast< float >( t0[ 0 ] ); t[ 0 ].y = static_cast< float >( t0[ 1 ] ); }
		if ( t1 ){ t[ 1 ].x = static_cast< float >( t1[ 0 ] ); t[ 1 ].y = static_cast< float >( t1[ 1 ] ); }
		if ( t2 ){ t[ 2 ].x = static_cast< float >( t2[ 0 ] ); t[ 2 ].y = static_cast< float >( t2[ 1 ] ); }
		//幅はウィンドウの縦横比で変わるので、描画側（Graphics）の今の大きさを使う
		Graphics::Manager gm;
		float sx = 2.f / static_cast< float >( gm.width() );
		float sy = -2.f / static_cast< float >( gm.height() );
		for ( int i = 0; i < 3; ++i ){
			p[ i ].x = p[ i ].x * sx - 1.f;
			p[ i ].y = p[ i ].y * sy + 1.f;
		}
		mPrimitiveRenderer.addTriangle( p[ 0 ], p[ 1 ], p[ 2 ], t[ 0 ], t[ 1 ], t[ 2 ], c0, c1, c2 );
	}
	void drawTriangle3DH(
	const double* p0, const double* p1, const double* p2,
	const double* t0, const double* t1, const double* t2,
	unsigned c0, unsigned c1, unsigned c2 ){
		Vector4 p[ 3 ];
		Vector2 t[ 3 ];
		for ( int i = 0; i < 4; ++i ){
			p[ 0 ][ i ] = static_cast< float >( p0[ i ] );
			p[ 1 ][ i ] = static_cast< float >( p1[ i ] );
			p[ 2 ][ i ] = static_cast< float >( p2[ i ] );
		}
		if ( t0 ){ t[ 0 ].x = static_cast< float >( t0[ 0 ] ); t[ 0 ].y = static_cast< float >( t0[ 1 ] ); }
		if ( t1 ){ t[ 1 ].x = static_cast< float >( t1[ 0 ] ); t[ 1 ].y = static_cast< float >( t1[ 1 ] ); }
		if ( t2 ){ t[ 2 ].x = static_cast< float >( t2[ 0 ] ); t[ 2 ].y = static_cast< float >( t2[ 1 ] ); }
		mPrimitiveRenderer.addTransformedTriangle( p[ 0 ], p[ 1 ], p[ 2 ], t[ 0 ], t[ 1 ], t[ 2 ], c0, c1, c2 );
	}
	Graphics::Texture createTexture(
	int dw, int dh, const unsigned* src, int sw, int sh ){
		int newPos = -1;
		for ( int i = 0; i < TEXTURE_NUMBER; ++i ){
			if ( !mTextures[ i ] ){ newPos = i; break; }
		}
		ASSERT( newPos >= 0 && "Texture Full!" );
		mTextures[ newPos ] = Graphics::Texture::create( dw, dh, false );
		unsigned* dst;
		int pitch;
		mTextures[ newPos ].lock( &dst, &pitch, 0 );
		int yEnd = ( dh < sh ) ? dh : sh;
		int xEnd = ( dw < sw ) ? dw : sw;
		for ( int y = 0; y < yEnd; ++y ){
			for ( int x = 0; x < xEnd; ++x ) dst[ x ] = src[ x ];
			for ( int x = xEnd; x < pitch / 4; ++x ) dst[ x ] = 0;
			dst += pitch / 4;
			src += sw;
		}
		for ( int y = yEnd; y < dh; ++y ){
			for ( int x = 0; x < pitch / 4; ++x ) dst[ x ] = 0;
			dst += pitch / 4;
		}
		mTextures[ newPos ].unlock( &dst );
		return mTextures[ newPos ];
	}
	void destroyTexture( Graphics::Texture& t ){
		for ( int i = 0; i < TEXTURE_NUMBER; ++i ){
			if ( mTextures[ i ] == t ){ mTextures[ i ].release(); break; }
		}
	}

	Scene::PrimitiveRenderer mPrimitiveRenderer;
	static const int TEXTURE_NUMBER = 1024;
	Graphics::Texture mTextures[ TEXTURE_NUMBER ];

	int mWidth;
	int mHeight;
	bool mFullScreen;
	bool mFullScreenForbidden;
	bool mVSync;
	bool mAntiAlias;
	string mTitle;
	string* mArchiveNames;
	int mArchiveNumber;
	FileIO::Manager::AccessMode mLoadMode;
	static const int TIME_HISTORY_SIZE = 60;
	unsigned mTimeHistory[ TIME_HISTORY_SIZE ];
	int mPreviousFrameInterval;
	unsigned mPreviousFrameTime;
	int mFrameRate;
	unsigned mIdealFrameInterval;
	bool mEndRequested;
	bool mStarted;
	Scene::StringRenderer mDebugStringRenderer;
	Scene::Font mDebugFont;
	Random mRandom;
};

Impl* gImpl = 0;

}

Framework::Framework(){
	ASSERT( WindowCreator::isMainThread() && "you must call from MAIN thread" );
}

void Framework::create(){
	ASSERT( !gImpl );
	gImpl = NEW Impl();
}

void Framework::destroy(){
	SAFE_DELETE( gImpl );
}

Framework Framework::instance(){
	return Framework();
}

void Framework::start( void* windowHandle ){
	gImpl->start( windowHandle );
}

void Framework::preUpdate(){
	gImpl->preUpdate();
}

void Framework::postUpdate(){
	gImpl->postUpdate();
}

//起動後は、ウィンドウの縦横比に合わせて変わる描画側（Graphics）の大きさを返す。
//起動前（WindowCreator::configure でウィンドウの大きさを決めるとき）は設定値を返す。
int Framework::width() const {
	return gImpl->mStarted ? Graphics::Manager().width() : gImpl->mWidth;
}

int Framework::height() const {
	return gImpl->mStarted ? Graphics::Manager().height() : gImpl->mHeight;
}

void Framework::requestEnd(){
	gImpl->mEndRequested = true;
}

bool Framework::isEndRequested() const {
	return gImpl->mEndRequested;
}

unsigned Framework::time() const {
	return gImpl->time();
}

int Framework::previousFrameInterval() const {
	return gImpl->mPreviousFrameInterval;
}

int Framework::frameRate() const {
	return gImpl->mFrameRate;
}

void Framework::setFrameRate( int fr ){
	gImpl->mIdealFrameInterval = 1000 / fr;
}

void Framework::drawDebugString( int c, int r, const char* s, unsigned col ){
	gImpl->mDebugStringRenderer.add( c * 8, r * 16, s, col );
}

void Framework::sleep( int ms ) const {
	Threading::sleep( ms );
}

int Framework::getRandom( int m ){
	if ( m <= 0 ){
		return gImpl->mRandom.getInt();
	}else{
		return gImpl->mRandom.getInt( 0, m );
	}
}

void Framework::drawTriangle3D(
const double* p0, const double* p1, const double* p2,
const double* t0, const double* t1, const double* t2,
unsigned c0, unsigned c1, unsigned c2 ){
	gImpl->drawTriangle3D( p0, p1, p2, t0, t1, t2, c0, c1, c2 );
}

void Framework::drawTriangle3DH(
const double* p0, const double* p1, const double* p2,
const double* t0, const double* t1, const double* t2,
unsigned c0, unsigned c1, unsigned c2 ){
	gImpl->drawTriangle3DH( p0, p1, p2, t0, t1, t2, c0, c1, c2 );
}

class Texture{
public:
	Graphics::Texture mTexture;
};

void Framework::createTexture( Texture** tex, const char* filename ){
	ASSERT( !( *tex ) && "Non Null Pointer! already created?" );
	*tex = NEW Texture();
	( *tex )->mTexture = Graphics::Texture::create( filename );
	while ( !( *tex )->mTexture.isReady() ){
		;
	}
}

void Framework::getTextureSizes( const Texture* tex, int* width, int* height, int* originalWidth, int* originalHeight ){
	if ( width )          *width          = tex->mTexture.width();
	if ( height )         *height         = tex->mTexture.height();
	if ( originalWidth )  *originalWidth  = tex->mTexture.originalWidth();
	if ( originalHeight ) *originalHeight = tex->mTexture.originalHeight();
}

void Framework::destroyTexture( Texture** tex ){
	ASSERT( tex && "NULL Pointer!" );
	if ( *tex ){
		gImpl->destroyTexture( ( *tex )->mTexture );
	}
	SAFE_DELETE( *tex );
}

void Framework::setTexture( const Texture* tex ){
	if ( tex ){
		gImpl->mPrimitiveRenderer.setTexture( tex->mTexture );
	}else{
		gImpl->mPrimitiveRenderer.setTexture( 0 );
	}
}

void Framework::setBlendMode( BlendMode a ){
	gImpl->setBlendMode( a );
}

void Framework::enableDepthTest( bool f ){
	gImpl->mPrimitiveRenderer.enableDepthTest( f );
}

void Framework::enableDepthWrite( bool f ){
	gImpl->mPrimitiveRenderer.enableDepthWrite( f );
}

// WindowCreator callbacks
void WindowCreator::configure( Configuration* config ){
	Framework::create();
	Framework f;
	config->setWidth( f.width() );
	config->setHeight( f.height() );
	config->setTitle( "DirectX11 Turn Command Battle" );
	config->enableFullScreen( false );
	config->forbidFullScreen( false ); //alt+enterでボーダーレスのフルスクリーンへ切り替えられる
}

void WindowCreator::update(){
	WindowCreator wc;
	Framework f;
	bool sleepFlag = wc.isMinimized();
	if ( !sleepFlag ){
		f.preUpdate();
	}
	if ( isEndRequested() ){
		f.requestEnd();
	}
	if ( sleepFlag ){
		Threading::sleep( 50 );
	}else{
		f.update();
		f.postUpdate();
	}
	if ( f.isEndRequested() ){
		Framework::destroy();
		end();
	}
}

void WindowCreator::start( void* windowHandle ){
	Framework().start( windowHandle );
}
}
