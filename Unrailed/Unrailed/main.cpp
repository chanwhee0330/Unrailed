#include <tchar.h>
#include "Game.h"

// [라이브러리 설정] GDI+를 사용하기 위해 필요한 라이브러리 파일을 링크합니다.
#pragma comment(lib,"gdiplus.lib")

// [화면 설정] 윈도우 창의 테두리 및 메뉴바 크기를 고려한 여백 설정입니다.
#define marginX 16
#define marginY 39

// [글로벌 변수] 프로그램 전체에서 사용되는 변수들입니다.
HINSTANCE g_hInst;                               // 프로그램의 인스턴스 핸들 (메모리 주소)
LPCTSTR lpszClass = L"Window Class Name";        // 윈도우 클래스 이름
LPCTSTR lpszWindowName = L"Unrailed";            // 창 제목표시줄에 뜰 이름

// [함수 선언] 윈도우에서 발생하는 이벤트를 처리하는 함수입니다.
LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);

// [GDI+ & 게임 객체] 그래픽 엔진 토큰과 실제 게임 로직을 담당하는 객체입니다.
ULONG_PTR g_gdiplusToken;
Game* g_game = nullptr;

/**
 * [WinMain] 프로그램의 시작점입니다. (마치 C언어의 main 함수)
 * 1. GDI+ 초기화
 * 2. 윈도우 창의 형태(Class) 등록
 * 3. 실제 윈도우 창 생성
 * 4. 메시지 루프 (사용자 입력 대기)
 */
int  WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_  LPSTR lpszCmdParam, _In_  int nCmdShow)
{
	HWND hWnd;
	MSG Message;
	WNDCLASSEX WndClass;
	g_hInst = hInstance;

	// [1단계: GDI+ 시작] 그래픽 기능을 사용하기 위해 엔진을 켭니다.
	GdiplusStartupInput gdiplusStartupInput;
	GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, NULL);

	// [2단계: 윈도우 클래스 설정] 창의 스타일, 배경색, 커서 모양 등을 정의합니다.
	WndClass.cbSize = sizeof(WndClass);
	WndClass.style = CS_HREDRAW | CS_VREDRAW;    // 가로/세로 크기가 변하면 다시 그리기
	WndClass.lpfnWndProc = (WNDPROC)WndProc;    // 이 창에서 발생하는 이벤트는 WndProc이 처리함
	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hInstance = hInstance;
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hCursor = LoadCursor(NULL, IDI_APPLICATION);
	WndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH); // 배경은 흰색
	WndClass.lpszMenuName = NULL;
	WndClass.lpszClassName = lpszClass;
	WndClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	RegisterClassEx(&WndClass); // 설정한 클래스를 운영체제에 등록

	// [3단계: 창 만들기] 실제로 화면에 보일 창을 생성합니다.
	hWnd = CreateWindow(lpszClass, lpszWindowName, WS_OVERLAPPEDWINDOW, 
		0, 0, SCREEN_WIDTH + marginX, SCREEN_HEIGHT + marginY, 
		NULL, (HMENU)NULL, hInstance, NULL);
	
	ShowWindow(hWnd, nCmdShow); // 창을 화면에 보여줌
	UpdateWindow(hWnd);         // 창을 즉시 갱신

	// [4단계: 메시지 루프] 사용자가 마우스를 움직이거나 키보드를 누르는 것을 무한히 기다립니다.
	while (GetMessage(&Message, 0, 0, 0)) {
		TranslateMessage(&Message); // 키보드 입력 메시지를 변환
		DispatchMessage(&Message);  // WndProc 함수로 메시지를 전달 (배달원 역할)
	}

    // [5단계: 종료] 프로그램이 끝나면 GDI+ 엔진도 끕니다.
    GdiplusShutdown(g_gdiplusToken);
	return (int)Message.wParam;
}

/**
 * [WndProc] 윈도우 프로시저 - 프로그램의 "두뇌" 역할을 합니다.
 * 마우스 클릭, 키보드 입력, 화면 그리기 등 모든 사건(Message)을 여기서 처리합니다.
 */
LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hDC;

	switch (iMessage) {
	case WM_CREATE: // 1. 창이 처음 만들어질 때 단 한 번 실행됨
	{
        // 게임 객체를 생성합니다. (플레이어, 맵 등의 초기화가 여기서 일어남)
        g_game = new Game(hWnd);
		return 0;
	}

	case WM_PAINT: // 2. 화면을 그려야 할 때마다 실행됨 (창 크기 조절, 가려졌다 나타날 때 등)
	{
		hDC = BeginPaint(hWnd, &ps);
        // Game 객체에게 "hDC라는 도화지에 그림을 그려라"라고 명령합니다.
        if (g_game) g_game->Draw(hDC);
		EndPaint(hWnd, &ps);
		return 0;
	}

    case WM_KEYDOWN: // 3. 키보드가 눌렸을 때 실행됨
    {
        // 키가 눌리면 게임의 상태(위치, 충돌 등)를 업데이트합니다.
        if (g_game) g_game->Update();
        return 0;
    }

	case WM_DESTROY: // 4. 창이 닫힐 때 실행됨
	{
        // 메모리 누수를 방지하기 위해 생성했던 게임 객체를 삭제합니다.
        delete g_game;
		PostQuitMessage(0); // 메시지 루프를 종료시킴
		return 0;
	}
	}

	// 우리가 처리하지 않은 나머지 기본 동작들(창 끄기 버튼 등)은 운영체제가 처리하도록 넘깁니다.
	return (DefWindowProc(hWnd, iMessage, wParam, lParam));
}

