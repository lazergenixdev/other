#pragma once
#include "Windows.h"
#include <string>

class Graphics;

class Window
{
	friend Graphics;
public:
	Window() = delete;
	Window(const Window& src) = delete;
	Window& operator=(const Window& src) = delete;
	Window( UINT width, UINT height, std::wstring title );
	~Window();
	bool Run();
private:
	static LRESULT WinProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT ProcessMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
private:
	std::wstring title, wndclass;
	UINT ScreenWidth, ScreenHeight;
	HINSTANCE hInst;
	HWND hWnd;
};

