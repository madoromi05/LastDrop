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

using namespace std;
using namespace GameLib;

//WindowCreator�����o�֐�
namespace GameLib{

namespace{ //����

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
	mTimeBias( 0 ){
		//getTime()�p�o�C�A�X�l�v�Z
		unsigned time = timeGetTime();
		mTimeBias = 0xffffffff - time;
		mTimeBias -= 60000; //60�b�����B

		//�X���b�hID�擾
		mThreadId = GetCurrentThreadId();
	}
	~Impl(){
	}
	void enableFullScreen( bool f ){
		if ( mFullScreen != f ){
			if ( mStarted ){
				LONG style;
				if ( f ){
					style = WS_POPUP | WS_VISIBLE;
					//�ő剻����Ƃ��ɂ͍��̏ꏊ���o���Ă���
					GetWindowRect( mWindowHandle, &mWindowRect );
					SetWindowLong( mWindowHandle, GWL_STYLE, style );
					//�ʒu��0,0
					SetWindowPos( mWindowHandle, NULL, 
						0, 0,
						mWidth, mHeight,
						SWP_SHOWWINDOW );
				}else{
					style = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE;
					SetWindowLong( mWindowHandle, GWL_STYLE, style );
					//���ɖ߂����ɂ͈ʒu��߂�
					SetWindowPos( mWindowHandle, NULL, 
						mWindowRect.left, mWindowRect.top,
						mWindowRect.right - mWindowRect.left,
						mWindowRect.bottom - mWindowRect.top,
						SWP_SHOWWINDOW );
				}
			}
			mFullScreen = f;
		}
	}
	void createDefaultWindowPos(){
		mWindowRect.left = 0;
		mWindowRect.top = 0;
		mWindowRect.right = mWidth;
		mWindowRect.bottom = mHeight;
		LONG style;
		style = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE;
		int oldLeft = mWindowRect.left;
		int oldTop = mWindowRect.top;
		AdjustWindowRect( &mWindowRect, style, FALSE );
		int diffX = mWindowRect.left - oldLeft;
		int diffY = mWindowRect.top - oldTop;
		mWindowRect.left -= diffX;
		mWindowRect.right -= diffX;
		mWindowRect.top -= diffY;
		mWindowRect.bottom -= diffY;
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
	unsigned mTimeBias; //timeGetTime�ɂ͂�����Q�^
	unsigned mThreadId;
	Array< string > mDroppedItems;
	HWND mWindowHandle;
	RECT mWindowRect;
};

Impl* gImpl = 0; //�B��C���X�^���X

} //namespace{}

WindowCreator::WindowCreator(){
	//�ʃX���b�h����̌Ăяo���͋����Ȃ�
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
	return WindowCreator(); //���͂��̊֐�����Ȃ�
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

// �O���[�o���ϐ�:
HINSTANCE hInst;								// ���݂̃C���^�[�t�F�C�X
TCHAR szTitle[MAX_LOADSTRING];					// �^�C�g�� �o�[�̃e�L�X�g
TCHAR szWindowClass[MAX_LOADSTRING];			// ���C�� �E�B���h�E �N���X��

// ���̃R�[�h ���W���[���Ɋ܂܂��֐��̐錾��]�����܂�:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);


int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	CoInitializeEx( NULL, COINIT_MULTITHREADED );
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: �����ɃR�[�h��}�����Ă��������B
	MSG msg;
	HACCEL hAccelTable;

	//�E�B���h�E��ԕێ��N���X����
	gImpl = NEW Impl();
	gImpl->mCommandLine = lpCmdLine;
	WindowCreator wc;
	//���[�U�ݒ�֐��Ăяo��
	WindowCreator::Configuration config;
	wc.configure( &config );

	memcpy(
		szTitle, 
		wc.title(), 
		wc.titleLength() ); 
	memcpy( szWindowClass, "GameLibWindowCreator", sizeof("GameLibWindowCreator"));

	MyRegisterClass( hInstance );

	// �A�v���P�[�V�����̏����������s���܂�:
	if ( !InitInstance ( hInstance, nCmdShow ) ){
		return FALSE;
	}

	hAccelTable = LoadAccelerators( hInstance, MAKEINTRESOURCE( IDC_WINDOWCREATOR ) );

	timeBeginPeriod( 1 );
	HWND windowHandle = gImpl->mWindowHandle;
	///�N���O�̗�O�͌p���s�\�Ȃ̂ŁA���b�Z�[�W�{�b�N�X���o���ďI��点��
	try{
		wc.start( windowHandle );
	}
	catch ( ... ){ //�N�����̗�O�͖����ł��Ȃ��B
		MessageBoxA( 
			windowHandle, 
			"�v���I�Ȗ�肪����A�N���ł��܂���B�\���󂠂�܂��񂪏I���������܂�", 
			"�v���I�G���[", 
			MB_OK | MB_ICONERROR );
		wc.requestEnd(); //�I���B
		wc.update(); //���ŏI������
		SAFE_DELETE( gImpl );
		timeEndPeriod( 1 );
		return 0;
	}

	//�t���X�N���[���Ȃ�߂��E�B���h�E�ʒu�𐶐�
	if ( gImpl->mFullScreen ){
		gImpl->createDefaultWindowPos();
	}
	//�h���b�O�A���h�h���b�v�L����
	if ( gImpl->mDragAndDropEnabled ){
		DragAcceptFiles( windowHandle, TRUE );
	}
	gImpl->mStarted = true; //���[�v�J�n�M��
	while ( true ){
		//���b�Z�[�W������Ȃ珈��
		if ( PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) ){
			if ( GetMessage( &msg, NULL, 0, 0 ) ){
				if ( !TranslateAccelerator( msg.hwnd, hAccelTable, &msg ) ){
					TranslateMessage( &msg );
					DispatchMessage( &msg );
				}
			}else{
				break; //WM_QUIT���܂����B���[�v�����܂��B
			}
		}
		//�I����Ă��window�j��
		if ( gImpl->mEnded ){ //�I����ĂȂ���΍X�V���[�v
			if ( windowHandle ){ //�����������2�񗈂邩������Ȃ��̂ŔO�̂���
				DestroyWindow( windowHandle );
				windowHandle = 0;
			}
		}else{	//���s�{��
			try{
				wc.update();
			}
			catch ( Exception e ){
				if ( e == EXCEPTION_EXIT ){ //�I���v����O�̏ꍇ�̂ݏI��
					MessageBoxA( 
						windowHandle, 
						"�v���I�Ȗ�肪����A�p���ł��܂���B�\���󂠂�܂��񂪏I���������܂�", 
						"�v���I�G���[", 
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
//  �֐�: MyRegisterClass()
//
//  �ړI: �E�B���h�E �N���X��o�^���܂��B
//
//  �R�����g:
//
//    ���̊֐�����юg�����́A'RegisterClassEx' �֐����ǉ����ꂽ
//    Windows 95 ���O�� Win32 �V�X�e���ƌ݊�������ꍇ�ɂ̂ݕK�v�ł��B
//    �A�v���P�[�V�������A�֘A�t����ꂽ
//    �������`���̏������A�C�R�����擾�ł���悤�ɂ���ɂ́A
//    ���̊֐����Ăяo���Ă��������B
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
//   �֐�: InitInstance(HINSTANCE, int)
//
//   �ړI: �C���X�^���X �n���h����ۑ����āA���C�� �E�B���h�E���쐬���܂��B
//
//   �R�����g:
//
//        ���̊֐��ŁA�O���[�o���ϐ��ŃC���X�^���X �n���h����ۑ����A
//        ���C�� �v���O���� �E�B���h�E���쐬����ѕ\�����܂��B
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;

	hInst = hInstance; // �O���[�o���ϐ��ɃC���X�^���X�������i�[���܂��B
//----
	DWORD style = 0;
	if ( gImpl->mFullScreen ){
		style = WS_POPUP;
	}else{
		style = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
	}
	RECT rect;
	rect.left = 0;
	rect.top = 0;
	rect.right = gImpl->mWidth;
	rect.bottom = gImpl->mHeight;
	AdjustWindowRect(&rect, style, FALSE);
	rect.right -= rect.left;
	rect.bottom -= rect.top; //�����v�Z
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
//  �֐�: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  �ړI:  ���C�� �E�B���h�E�̃��b�Z�[�W���������܂��B
//
//  WM_COMMAND	- �A�v���P�[�V���� ���j���[�̏���
//  WM_PAINT	- ���C�� �E�B���h�E�̕`��
//  WM_DESTROY	- ���~���b�Z�[�W��\�����Ė߂�
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		break;
	case WM_CLOSE: //�I���v��
		gImpl->mEndRequested = true;
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_SYSKEYDOWN: //alt+enter�F��
		if ( !gImpl->mFullScreenForbidden ){
			if ( wParam == VK_RETURN ){
				if ( gImpl->mFullScreen ){
					WindowCreator().enableFullScreen( false );
				}else{
					WindowCreator().enableFullScreen( true );
				}
			}
		}
		DefWindowProc(hWnd, message, wParam, lParam);
		break;
	case WM_ACTIVATE:
		if ( wParam & 0xffff0000 ){ //�ŏ�������Ă����false
			gImpl->mActive = false;
		}else{
			gImpl->mActive = ( ( wParam & 0xffff ) != 0 );
		}
		break;
	case WM_SYSCOMMAND:
		if ( wParam == SC_MAXIMIZE ){
			if ( !gImpl->mFullScreenForbidden ){
				WindowCreator().enableFullScreen( true ); //�ő剻�̎���DefWindowProc�͌Ă΂Ȃ�
			}
		}else if ( wParam == SC_MINIMIZE ){
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
