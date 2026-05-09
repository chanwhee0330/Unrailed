#include <tchar.h>
#include "Game.h"

#pragma comment(lib,"gdiplus.lib")

#define marginX 16
#define marginY 39

HINSTANCE g_hInst;
LPCTSTR lpszClass = L"Window Class Name";
LPCTSTR lpszWindowName = L"Unrailed";

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);

ULONG_PTR g_gdiplusToken;
Game* g_game = nullptr;

int  WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_  LPSTR lpszCmdParam, _In_  int nCmdShow)
{
	HWND hWnd;
	MSG Message;
	WNDCLASSEX WndClass;
	g_hInst = hInstance;

	GdiplusStartupInput gdiplusStartupInput;
	GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, NULL);

	WndClass.cbSize = sizeof(WndClass);
	WndClass.style = CS_HREDRAW | CS_VREDRAW;
	WndClass.lpfnWndProc = (WNDPROC)WndProc;
	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hInstance = hInstance;
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hCursor = LoadCursor(NULL, IDI_APPLICATION);
	WndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	WndClass.lpszMenuName = NULL;
	WndClass.lpszClassName = lpszClass;
	WndClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	RegisterClassEx(&WndClass);

	hWnd = CreateWindow(lpszClass, lpszWindowName, WS_OVERLAPPEDWINDOW, 0, 0, SCREEN_WIDTH + marginX, SCREEN_HEIGHT + marginY, NULL, (HMENU)NULL, hInstance, NULL);
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	while (GetMessage(&Message, 0, 0, 0)) {
		TranslateMessage(&Message);
		DispatchMessage(&Message);
	}

    GdiplusShutdown(g_gdiplusToken);
	return (int)Message.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hDC;
	switch (iMessage) {
	case WM_CREATE:
	{
        g_game = new Game(hWnd);
		return 0;
	}
	case WM_PAINT:
	{
		hDC = BeginPaint(hWnd, &ps);
        if (g_game) g_game->Draw(hDC);
		EndPaint(hWnd, &ps);
		return 0;
	}
    case WM_KEYDOWN:
    {
        if (g_game) g_game->Update();
        return 0;
    }
	case WM_DESTROY:
	{
        delete g_game;
		PostQuitMessage(0);
		return 0;
	}
	}
	return (DefWindowProc(hWnd, iMessage, wParam, lParam));
}

