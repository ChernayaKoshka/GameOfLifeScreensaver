#include <Windows.h>
#include <stdio.h>
#include <time.h>
#include <ShlObj.h>
#include "math_custom.h"
#include "life.h"

#define CELL_SIZE 8

#define STEPS_PER_SECOND 0.25f

BOOL running = TRUE;

int monCount = 0;

int expireCount = 0;

BOOL fullscreenSuccessful = FALSE;

typedef struct tagEnumeratedDisplay
{
	BOOL bIsWindow;
	HWND hwndWindow;
	RECT monRect;
	HDC hdcDrawingContext;
	int ScreenWidth;
	int ScreenHeight;
	POINT Previous;
	void* BackBuffer;
	BITMAPINFO BitMapInfo;
}EnumeratedDisplay;

/*
Either I'm an idiot or Microsoft is an idiot, one of the two.
I had to define this struct myself because MONITORINFOEX only exposed
the szDevice field for some reason
*/
typedef struct tagMONITORINFOEX {
	DWORD cbSize;
	RECT  rcMonitor;
	RECT  rcWork;
	DWORD dwFlags;
	TCHAR szDevice[CCHDEVICENAME];
} MONITORINFOEX2, *LPMONITORINFOEX2;

EnumeratedDisplay* Displays;

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
	if (monCount >= 2) return TRUE;

	MONITORINFOEX2 info = { 0 };
	info.cbSize = sizeof(MONITORINFOEX2);

#pragma warning (suppress : 4133)
	GetMonitorInfoW(hMonitor, &info);
	HDC dc = CreateDC(TEXT("DISPLAY"), info.szDevice, NULL, NULL);

	EnumeratedDisplay display = { 0 };

	display.hdcDrawingContext = dc;

	display.monRect = info.rcMonitor;

	int screenHeight = Difference(info.rcMonitor.bottom, info.rcMonitor.top);
	int screenWidth = Difference(info.rcMonitor.right, info.rcMonitor.left);

	display.ScreenHeight = screenHeight;
	display.ScreenWidth = screenWidth;

	if (monCount == 0)
	{
		int bufferSize = screenHeight*screenWidth * 4;
		display.BackBuffer = malloc(bufferSize); //4 = bytes to display RGB
		ZeroMemory(display.BackBuffer, bufferSize);

		display.BitMapInfo.bmiHeader.biSize = sizeof(display.BitMapInfo.bmiHeader);
		display.BitMapInfo.bmiHeader.biWidth = screenWidth;
		display.BitMapInfo.bmiHeader.biHeight = -screenHeight;
		display.BitMapInfo.bmiHeader.biPlanes = 1;
		display.BitMapInfo.bmiHeader.biBitCount = 32;
		display.BitMapInfo.bmiHeader.biCompression = BI_RGB;
	}
	else
	{
		display.BackBuffer = Displays[0].BackBuffer;
		display.BitMapInfo = Displays[0].BitMapInfo;
	}

	Displays[monCount] = display;
	monCount++;
	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, WPARAM lParam)
{
	LRESULT Result = 0;

	switch (uMsg)
	{
	case WM_KEYUP:
		return Result;
	case WM_ERASEBKGND:
		return TRUE;
	case WM_CLOSE:
		running = FALSE;
		return Result;
	case WM_COMMAND:
		return Result;
	case WM_PAINT:
	default:
		Result = DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return Result;
}

void CreateFullscreenWindows(HINSTANCE hInstance, int nShowCmd)
{
	for (int i = 0; i < monCount; i++)
	{
		//define window
		WNDCLASSEX wc = { 0 };
		wc.cbSize = sizeof(wc);
		wc.lpfnWndProc = WndProc;
		wc.hInstance = hInstance;
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);

		wchar_t buf[20];

		swprintf_s(buf, 20, L"LineSCR%d", i);

		wc.lpszClassName = &buf;

		if (!RegisterClassEx(&wc))
			return;

		HWND hwndFullscreenWindow = CreateWindowExW(
			0,
			&buf,
			&buf,
			WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			Displays[i].ScreenWidth,
			Displays[i].ScreenHeight,
			NULL,
			NULL,
			hInstance,
			0);

		SetWindowLongPtr(hwndFullscreenWindow, GWL_EXSTYLE, WS_EX_APPWINDOW | WS_EX_TOPMOST);
		SetWindowLongPtr(hwndFullscreenWindow, GWL_STYLE, WS_MAXIMIZE);

		SetWindowPos(hwndFullscreenWindow, HWND_TOPMOST,
			Displays[i].monRect.left, Displays[i].monRect.top,
			Displays[i].ScreenWidth, Displays[i].ScreenHeight,
			0);

		Displays[i].hwndWindow = hwndFullscreenWindow;
	}

	//only executed if all windows were defined successfully
	for (int i = 0; i < monCount; i++)
	{
		Displays[i].bIsWindow = TRUE;
		Displays[i].hdcDrawingContext = GetDC(Displays[i].hwndWindow);
		ShowWindow(Displays[i].hwndWindow, nShowCmd);
	}
	fullscreenSuccessful = TRUE;
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	Displays = (EnumeratedDisplay*)malloc(sizeof(EnumeratedDisplay) * 2);
	ZeroMemory(Displays, sizeof(EnumeratedDisplay) * 2);
	EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);

	CreateFullscreenWindows(hInstance, nShowCmd);

	POINT oldPos;
	GetCursorPos(&oldPos);
	MSG msg;

	char* sim = createSimulation(Displays[0].ScreenHeight / CELL_SIZE + 2, Displays[0].ScreenWidth / CELL_SIZE + 2, 1, 0.35);
	clock_t prevTime = clock();

	while (running)
	{
		if (fullscreenSuccessful)
		{
			while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		clock_t curTime = clock();
		if (((double)(curTime - prevTime) / CLOCKS_PER_SEC) >= STEPS_PER_SECOND)
		{
			prevTime = curTime;
			char* newSim = stepSimulation(Displays[0].ScreenHeight / CELL_SIZE + 2, Displays[0].ScreenWidth / CELL_SIZE + 2, 1, sim);
			sim = newSim;
			convertSimulation(Displays[0].BackBuffer, Displays[0].ScreenWidth, 0x002C4566, sim, Displays[0].ScreenHeight / CELL_SIZE + 2, Displays[0].ScreenWidth / CELL_SIZE + 2, CELL_SIZE);
		}

		for (int i = 0; i < monCount; i++)
		{
			StretchDIBits(Displays[i].hdcDrawingContext,
				0, 0, Displays[i].ScreenWidth, Displays[i].ScreenHeight,
				0, 0, Displays[i].BitMapInfo.bmiHeader.biWidth, Abs(Displays[i].BitMapInfo.bmiHeader.biHeight),
				Displays[i].BackBuffer, &Displays[i].BitMapInfo,
				DIB_RGB_COLORS, SRCCOPY);
		}

		POINT newPos;
		GetCursorPos(&newPos);
		if (Abs(newPos.x - oldPos.x) > 5 || Abs(newPos.y - oldPos.y) > 5)
			running = FALSE;
	}

	for (int i = 0; i < monCount; i++)
	{
		ReleaseDC(NULL, Displays[i].hdcDrawingContext);
		if (Displays[i].BackBuffer == Displays[0].BackBuffer && i != 0)
			continue;
		free(Displays[i].BackBuffer);
	}
	free(Displays);

	//refresh explorer to remove remanents
	SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, 0, 0);

	return EXIT_SUCCESS;
}
