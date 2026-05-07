#include <windows.h>
#include "Game.h"

Game* g_pGame = nullptr;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_KEYDOWN: if (g_pGame) g_pGame->OnKeyDown(wParam); break;
    case WM_KEYUP:   if (g_pGame) g_pGame->OnKeyUp(wParam);   break;
    case WM_DESTROY: PostQuitMessage(0); break;
    default: return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    WNDCLASSEX wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.lpszClassName = L"UnrailedClass";
    RegisterClassEx(&wcex);

    HWND hWnd = CreateWindow(L"UnrailedClass", L"Unrailed",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, 1920, 1080,
        nullptr, nullptr, hInstance, nullptr);
    ShowWindow(hWnd, nCmdShow);

    g_pGame = new Game(hWnd);
    g_pGame->Init();

    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            g_pGame->Update();
            g_pGame->Render();
        }
    }

    delete g_pGame;
    return (int)msg.wParam;
}