#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601 // Windows 7+ (required for DX11/DXGI 1.1)
#endif

#include <tchar.h>
#include <windows.h>
#include <mmsystem.h>
#include "WindowCreator/resource.h"
#include <string>
#undef min
#undef max

#include "GameLib/GameLib.h"
#include "GameLib/WindowCreator/WindowCreator.h"
#include "GameLib/Threading/Functions.h"
#include "GameLib/DebugUi/DebugUi.h"

using namespace std;
using namespace GameLib;

//WindowCreatorのメンバ関数
namespace GameLib{

namespace{

//ウィンドウ表示のときのスタイル。WS_THICKFRAME で端をドラッグして大きさを変えられ、最大化ボタンで画面いっぱいになる。
const LONG kWindowedStyle = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME;
//フルスクリーン（ボーダーレス）のときのスタイル。枠なしのウィンドウをモニター全体に広げる。
//画面モードを切り替える排他フルスクリーンにしないのは、切り替えの暗転が無く、Alt+Tab でも描画が止まらないため。
const LONG kFullScreenStyle = WS_POPUP;
//WM_SYSKEYDOWN の lParam で「前から押されていた（キーリピート）」を表すビット。
const LPARAM kKeyWasDownBit = 1 << 30;

class Impl{
public:
	Impl() :
	mWidth( 640 ), 
	mHeight( 480 ), 
	mFullScreen( false ),
	mMinimized( false ),
	mActive( false ),
	mTitle( "A GameLib Application" ),
	mCommandLine( "" ),
	mEndRequested( false ),
	mEnded( false ),
	mStarted( false ),
	mFullScreenForbidden( false ),
	mDragAndDropEnabled( false ),
	mMouseWheel( 0 ),
	mMouseButtonPresses( 0 ),
	mTimeBias( 0 ),
	mThreadId( 0 ),
	mWindowHandle( NULL ),
	mWindowPlacement(){
		mWindowPlacement.length = sizeof( mWindowPlacement );
		//getTime()用バイアス値計算
		unsigned time = timeGetTime();
		mTimeBias = 0xffffffff - time;
		mTimeBias -= 60000; //60秒の余裕を持たせる。

		//スレッドID取得
		mThreadId = GetCurrentThreadId();
	}
	~Impl(){
	}
	void enableFullScreen( bool f ){
		if ( mFullScreen != f ){
			if ( mStarted ){
				if ( f ){
					//戻すときのために、位置・大きさ・最大化していたかをまとめて覚えておく
					GetWindowPlacement( mWindowHandle, &mWindowPlacement );
					//ウィンドウが今いるモニターの全体に広げる
					MONITORINFO monitor = {};
					monitor.cbSize = sizeof( monitor );
					GetMonitorInfo( MonitorFromWindow( mWindowHandle, MONITOR_DEFAULTTONEAREST ), &monitor );
					const RECT& area = monitor.rcMonitor;
					SetWindowLong( mWindowHandle, GWL_STYLE, kFullScreenStyle | WS_VISIBLE );
					SetWindowPos( mWindowHandle, HWND_TOP,
						area.left, area.top,
						area.right - area.left,
						area.bottom - area.top,
						SWP_FRAMECHANGED | SWP_SHOWWINDOW );
				}else{
					SetWindowLong( mWindowHandle, GWL_STYLE, kWindowedStyle | WS_VISIBLE );
					//覚えておいた位置へ戻す（最大化していたなら最大化へ戻る）
					SetWindowPlacement( mWindowHandle, &mWindowPlacement );
					//スタイルの変更（枠の有無）を反映させる
					SetWindowPos( mWindowHandle, NULL, 0, 0, 0, 0,
						SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED );
				}
			}
			mFullScreen = f;
		}
	}
	//フルスクリーンで起動したときの、ウィンドウへ戻す先を作る（左上に、描画サイズの大きさで置く）
	void createDefaultWindowPos(){
		RECT rect = { 0, 0, mWidth, mHeight };
		AdjustWindowRect( &rect, kWindowedStyle, FALSE );
		OffsetRect( &rect, -rect.left, -rect.top );
		mWindowPlacement.length = sizeof( mWindowPlacement );
		mWindowPlacement.showCmd = SW_SHOWNORMAL;
		mWindowPlacement.rcNormalPosition = rect;
	}
	int mWidth;
	int mHeight;
	bool mFullScreen;
	bool mMinimized;
	bool mActive;
	string mTitle;
	string mCommandLine;
	bool mEndRequested;
	bool mEnded;
	bool mStarted;
	bool mFullScreenForbidden;
	bool mDragAndDropEnabled;
	int mMouseWheel;
	unsigned mMouseButtonPresses; //前に読んでから押されたマウスのボタン（ビット 0 左・1 右・2 中。Input::Manager が読んで 0 に戻す）
	unsigned mTimeBias; //timeGetTime()にはこれを加算
	unsigned mThreadId;
	Array< string > mDroppedItems;
	HWND mWindowHandle;
	WINDOWPLACEMENT mWindowPlacement; //フルスクリーンからウィンドウへ戻すときの位置・大きさ・最大化の有無
};

Impl* gImpl = 0; //唯一のインスタンス

} //namespace{}

WindowCreator::WindowCreator(){
	//別スレッドからの呼び出しは許されない
	ASSERT( isMainThread() && "you must call from MAIN thread" );
}

const char* WindowCreator::title() const {
	return gImpl->mTitle.c_str();
}

const char* WindowCreator::commandLineString() const {
	return gImpl->mCommandLine.c_str();
}

int WindowCreator::titleLength() const {
	return static_cast< int >( gImpl->mTitle.size() );
}

int WindowCreator::height() const {
	return gImpl->mHeight;
}

int WindowCreator::width() const {
	return gImpl->mWidth;
}

bool WindowCreator::isFullScreen() const {
	return gImpl->mFullScreen;
}

WindowCreator WindowCreator::instance(){
	return WindowCreator(); //中身はない関数だけのクラス
}

bool WindowCreator::isEndRequested() const {
	return gImpl->mEndRequested;
}

void WindowCreator::requestEnd(){
	gImpl->mEndRequested = true;
}

void WindowCreator::end(){
	gImpl->mEnded = true;
}

unsigned WindowCreator::time() const {
	return timeGetTime() + gImpl->mTimeBias;
}

bool WindowCreator::isMinimized() const {
	return gImpl->mMinimized;
}

bool WindowCreator::isActive() const {
	return gImpl->mActive;
}

bool WindowCreator::isMainThread(){
	return ( GetCurrentThreadId() == gImpl->mThreadId );
}

void WindowCreator::enableFullScreen( bool f ){
	gImpl->enableFullScreen( f );
}

bool WindowCreator::isDragAndDropEnabled() const {
	return gImpl->mDragAndDropEnabled;
}

int WindowCreator::droppedItemNumber() const {
	return gImpl->mDroppedItems.size();
}

const char* WindowCreator::droppedItem( int i ) const {
	return gImpl->mDroppedItems[ i ].c_str();
}

void WindowCreator::clearDroppedItem(){
	gImpl->mDroppedItems.clear();
}

int WindowCreator::getAndResetMouseWheel(){
	int t = gImpl->mMouseWheel;
	gImpl->mMouseWheel = 0;
	return t;
}

unsigned WindowCreator::getAndResetMouseButtonPresses(){
	unsigned t = gImpl->mMouseButtonPresses;
	gImpl->mMouseButtonPresses = 0;
	return t;
}

//Configuration
void WindowCreator::Configuration::setWidth( int w ){
	gImpl->mWidth = w;
}

void WindowCreator::Configuration::setHeight( int h ){
	gImpl->mHeight = h;
}

void WindowCreator::Configuration::setTitle( const char* title ){
	gImpl->mTitle = title;
}

void WindowCreator::Configuration::enableFullScreen( bool f ){
	gImpl->enableFullScreen( f );
}

void WindowCreator::Configuration::forbidFullScreen( bool f ){
	gImpl->mFullScreenForbidden = f;
}

void WindowCreator::Configuration::enableDragAndDrop( bool f ){
	gImpl->mDragAndDropEnabled = f;
}

} //namespace GameLib

#define MAX_LOADSTRING 100

// グローバル変数:
HINSTANCE hInst;								// 現在のインターフェイス
TCHAR szTitle[MAX_LOADSTRING];					// タイトル バーのテキスト
TCHAR szWindowClass[MAX_LOADSTRING];			// メイン ウィンドウ クラス名

// このコード モジュールに含まれる関数の宣言を転送します:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);


int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	HRESULT hrCoInit = CoInitializeEx( NULL, COINIT_MULTITHREADED );
	if ( FAILED( hrCoInit ) ){
		MessageBoxA( NULL, "COMの初期化に失敗しました。", "致命的なエラー", MB_OK | MB_ICONERROR );
		return 0;
	}
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: ここにコードを挿入してください。
	MSG msg;
	HACCEL hAccelTable;

	//ウィンドウ状態保持クラス生成
	gImpl = NEW Impl();
	gImpl->mCommandLine = lpCmdLine;
	WindowCreator wc;
	//ユーザ設定関数呼び出し
	WindowCreator::Configuration config;
	wc.configure( &config );

	memcpy(
		szTitle, 
		wc.title(), 
		wc.titleLength() ); 
	memcpy( szWindowClass, "GameLibWindowCreator", sizeof("GameLibWindowCreator"));

	MyRegisterClass( hInstance );

	// アプリケーションの初期化を実行します:
	if ( !InitInstance ( hInstance, nCmdShow ) ){
		return FALSE;
	}

	hAccelTable = LoadAccelerators( hInstance, MAKEINTRESOURCE( IDC_WINDOWCREATOR ) );

	timeBeginPeriod( 1 );
	HWND windowHandle = gImpl->mWindowHandle;
	///起動時の例外は継続不可能なので、メッセージボックスを出して終了させる
	try{
		wc.start( windowHandle );
	}
	catch ( ... ){ //起動時の例外は無視できない。
		MessageBoxA( 
			windowHandle, 
			"致命的な問題が発生したため、起動できません。ご迷惑をおかけしますが終了させていただきます",
			"致命的なエラー",
			MB_OK | MB_ICONERROR );
		wc.requestEnd(); //終了。
		wc.update(); //ここで終了処理
		SAFE_DELETE( gImpl );
		timeEndPeriod( 1 );
		return 0;
	}

	//フルスクリーンなら戻すウィンドウ位置を生成
	if ( gImpl->mFullScreen ){
		gImpl->createDefaultWindowPos();
	}
	//ドラッグアンドドロップ有効化
	if ( gImpl->mDragAndDropEnabled ){
		DragAcceptFiles( windowHandle, TRUE );
	}
	gImpl->mStarted = true; //ループ開始合図
	bool quitRequested = false;
	while ( true ){
		//溜まっているメッセージは、更新の前に全部処理する。
		//1フレームに1件だけだと、キーの自動リピート(WM_KEYDOWN/WM_CHAR)やマウス移動のメッセージが溜まり続け、
		//後から押したキー(押している間だけ走る Shift など)の反映が大きく遅れる。
		while ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) ){
			if ( msg.message == WM_QUIT ){
				quitRequested = true; //WM_QUITが来ました。ループを抜けます。
				break;
			}
			if ( !TranslateAccelerator( msg.hwnd, hAccelTable, &msg ) ){
				TranslateMessage( &msg );
				DispatchMessage( &msg );
			}
		}
		if ( quitRequested ){
			break;
		}
		//終了していたらwindow破棄
		if ( gImpl->mEnded ){ //終了していなければ更新ループ
			if ( windowHandle ){ //ここに2回来るかもしれないので念のため
				DestroyWindow( windowHandle );
				windowHandle = 0;
			}
		}else{	//実行本体
			try{
				wc.update();
			}
			catch ( Exception e ){
				if ( e == EXCEPTION_EXIT ){ //終了要求の例外の場合のみ終了
					MessageBoxA( 
						windowHandle, 
						"致命的な問題が発生したため、続行できません。ご迷惑をおかけしますが終了させていただきます",
						"致命的なエラー",
						MB_OK | MB_ICONERROR );
				}
			}
		}
	}
	SAFE_DELETE( gImpl );
	timeEndPeriod( 1 );

	CoUninitialize();
	return (int) msg.wParam;
}



//
//  関数: MyRegisterClass()
//
//  目的: ウィンドウ クラスを登録します。
//
//  コメント:
//
//    この関数およびその使用方法は、'RegisterClassEx' 関数が追加された
//    Windows 95 より前の Win32 システムと互換性を保つ場合にのみ必要です。
//    アプリケーションに、関連付けられた
//    正しい形式のウィンドウ クラスとアイコンを取得できるようにするには、
//    この関数を呼び出してください。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WINDOWCREATOR));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_WINDOWCREATOR);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   関数: InitInstance(HINSTANCE, int)
//
//   目的: インスタンス ハンドルを保存して、メイン ウィンドウを作成します。
//
//   コメント:
//
//        この関数で、グローバル変数にインスタンス ハンドルを保存し、
//        メイン プログラム ウィンドウを作成および表示します。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;

	hInst = hInstance; // グローバル変数にインスタンスを格納します。
//----
	DWORD style = gImpl->mFullScreen ? kFullScreenStyle : kWindowedStyle;
	RECT rect;
	rect.left = 0;
	rect.top = 0;
	rect.right = gImpl->mWidth;
	rect.bottom = gImpl->mHeight;
	AdjustWindowRect(&rect, style, FALSE);
	rect.right -= rect.left;
	rect.bottom -= rect.top; //高さ計算
	rect.left = 0;
	rect.top = 0;
//----

	hWnd = CreateWindow(szWindowClass, szTitle, 
		style,
		CW_USEDEFAULT, CW_USEDEFAULT,
		rect.right, rect.bottom, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   gImpl->mWindowHandle = hWnd;

   return TRUE;
}

//
//  関数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的:  メイン ウィンドウのメッセージを処理します。
//
//  WM_COMMAND	- アプリケーション メニューの処理
//  WM_PAINT	- メイン ウィンドウの描画
//  WM_DESTROY	- 終了メッセージを表示して戻る
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	// 開発用のエディタ（ImGui）に、マウス・キー・文字の知らせを先に渡す（Release では何もしない）。
	if ( DebugUi::handleWindowMessage( hWnd, message, wParam, lParam ) ){
		return 1;
	}
	switch (message)
	{
		break;
	case WM_CLOSE: //終了要求
		gImpl->mEndRequested = true;
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_SYSKEYDOWN: //alt+enterでフルスクリーン⇔ウィンドウ
		if ( !gImpl->mFullScreenForbidden && wParam == VK_RETURN ){
			//押しっぱなしのキーリピートでは切り替えない（押すたびに1回だけ）
			if ( !( lParam & kKeyWasDownBit ) ){
				WindowCreator().enableFullScreen( !gImpl->mFullScreen );
			}
			break; //DefWindowProcへ渡すとシステムメニューのビープが鳴る
		}
		DefWindowProc(hWnd, message, wParam, lParam);
		break;
	case WM_ACTIVATE:
		if ( wParam & 0xffff0000 ){ //最小化されていればfalse
			gImpl->mActive = false;
		}else{
			gImpl->mActive = ( ( wParam & 0xffff ) != 0 );
		}
		break;
	case WM_SYSCOMMAND:
		//最大化（SC_MAXIMIZE）は通常のウィンドウの最大化にするので DefWindowProc に任せる。
		//フルスクリーンは alt+enter で切り替える。
		//Alt だけを押して離すと（lParam が 0）、メニューを選ぶ状態に入り、次のキーでビープが鳴ってゲームに届かない。
		//エディタ（F6）は Alt を押している間だけ触るので、ここで止める（Alt+Space のシステムメニューは残す）。
		if ( ( wParam & 0xfff0 ) == SC_KEYMENU && lParam == 0 ){
			break;
		}
		if ( wParam == SC_MINIMIZE ){
			gImpl->mMinimized = true;
			gImpl->mActive = false;
			DefWindowProc(hWnd, message, wParam, lParam);
		}else if ( wParam == SC_RESTORE ){
			gImpl->mMinimized = false;
			gImpl->mActive = true;
			DefWindowProc(hWnd, message, wParam, lParam);
		}else{
			DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_MOUSEWHEEL:
		gImpl->mMouseWheel = ( static_cast< short >( HIWORD( wParam ) ) > 0 ) ? 1 : -1;
		break;
	//押した知らせを覚えておく（1 フレームの間に押して離した短いクリックを、Input::Manager が落とさないように。
	//GetAsyncKeyState は読んだ瞬間に押しているかしか分からないため。FuelEscapeLoading_Plan.txt Q10）。
	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
		gImpl->mMouseButtonPresses |= WindowCreator::MOUSE_PRESS_LEFT;
		return DefWindowProc( hWnd, message, wParam, lParam );
	case WM_RBUTTONDOWN:
	case WM_RBUTTONDBLCLK:
		gImpl->mMouseButtonPresses |= WindowCreator::MOUSE_PRESS_RIGHT;
		return DefWindowProc( hWnd, message, wParam, lParam );
	case WM_MBUTTONDOWN:
	case WM_MBUTTONDBLCLK:
		gImpl->mMouseButtonPresses |= WindowCreator::MOUSE_PRESS_MIDDLE;
		return DefWindowProc( hWnd, message, wParam, lParam );
	case WM_DROPFILES:
		if ( gImpl->mDragAndDropEnabled ){
			HDROP hDrop = ( HDROP )wParam;
			gImpl->mDroppedItems.clear();
			unsigned n = DragQueryFileA( hDrop, 0xffffffff, NULL, 0 );
			gImpl->mDroppedItems.setSize( n );
			char item[ MAX_PATH ];
			for ( unsigned i = 0; i < n; ++i ){
				DragQueryFileA( hDrop, i, item, MAX_PATH );
				gImpl->mDroppedItems[ i ] = item;
			}
		}
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
