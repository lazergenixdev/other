#include "Window.h"
#include <cassert>
#pragma comment( lib,"User32.lib" )

Window::Window(UINT width, UINT height, std::wstring title)
	: ScreenWidth( width ), ScreenHeight( height ), title( title ),
	hInst( GetModuleHandle( nullptr ) ), hWnd( NULL ), wndclass( L"electro thing/" )
{
	WNDCLASSW wc = {};
	wc.hInstance = this->hInst;
	wc.lpszClassName = wndclass.c_str();
	wc.hIcon = (HICON)(LoadImageW( nullptr, L"icon1.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE ));
	wc.lpfnWndProc = reinterpret_cast<WNDPROC>( WinProc );

	RegisterClassW( &wc );
	hWnd = CreateWindowExW(
		0, wndclass.c_str(), title.c_str(), WS_SYSMENU | WS_VISIBLE, 
		200, 100, ScreenWidth, ScreenHeight, 
		nullptr, nullptr, hInst, nullptr);

	if ( hWnd == NULL )
	{
		assert( "could not get a handle to the window, so bye" );
	}
	else
	{
		SetWindowLongPtrW( hWnd, GWLP_USERDATA, reinterpret_cast<LONG>( this ) );
	}
}

Window::~Window()
{
	UnregisterClassW( wndclass.c_str(), hInst );
}

bool Window::Run()
{
	MSG msg = {};
	PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE );
	TranslateMessage( &msg );
	if ( msg.message == WM_QUIT ) return false;
	DispatchMessage( &msg );
	return true;
}

LRESULT Window::WinProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	Window* pWindow = reinterpret_cast<Window*>( GetWindowLongPtr( hwnd, GWLP_USERDATA ) );
	if ( pWindow ) return pWindow->ProcessMessage( hwnd, msg, wParam, lParam );
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT Window::ProcessMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch ( msg )
	{
	case WM_CLOSE:
	{
		PostQuitMessage( 0x0 );
		return 0;
	}
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
}