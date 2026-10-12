#include "GameLib/GameLib.h"
#include "GameLib/WindowCreator/WindowCreator.h"
#include "GameLib/Input/Manager.h"
#include "GameLib/FileIO/Manager.h"
#include "GameLib/Sound/Manager.h"
#include "GameLib/Graphics/Manager.h"
#include "GameLib/Math/Vector2.h"
#include "GameLib/Scene/StringRenderer.h"
#include "GameLib/Scene/Font.h"
#include "GameLib/Threading/Functions.h"
#include "GameLib/Threading/Manager.h"
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
		cout.begin();
		unsigned t = time();
		for ( int i = 0; i < TIME_HISTORY_SIZE; ++i ){
			mTimeHistory[ i ] = t;
		}
	}
	~Impl(){
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
		// 3D は異方性つきで読む。点サンプリングだと、浅い角度で見た路面の細い線が粗いミップでつぶれて太く見える。
		Graphics::Manager().setTextureFilter( Graphics::TEXTURE_FILTER_LINEAR );
	}
	void postUpdate(){
		// デバッグ文字・2D は画素をにじませない。
		Graphics::Manager().setTextureFilter( Graphics::TEXTURE_FILTER_POINT );
		mDebugStringRenderer.draw();
		Graphics::Manager().endDraw();
	}

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
