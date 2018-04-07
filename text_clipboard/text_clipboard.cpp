// test_win32_app.cpp : Defines the entry point for the application.
//

#include "header.h"
#include "text_clipboard.h"
#include <stdexcept>
#include <sstream>
#include <optional>
#include <algorithm>

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_TESTWIN32APP, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TESTWIN32APP));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TESTWIN32APP));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_TESTWIN32APP);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	::OutputDebugStringA("!!! test a");
	::OutputDebugStringW(L"!!! test w");
	hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ::AddClipboardFormatListener(hWnd); // Todo: check the result (BOOL)


   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

class Clipboard
{
public:
	explicit Clipboard(HWND window)
		: opened_(!!::OpenClipboard(window))
	{
		if (!opened_)
			::OutputDebugStringA("!!! cannot open clipboard");
	}

	bool IsValid() const
	{
		return opened_;
	}

	std::optional<std::wstring> GetText() const
	{
		std::optional<std::wstring> text;
		if (!::IsClipboardFormatAvailable(CF_UNICODETEXT))
			return text;
		const HANDLE hGlobal = ::GetClipboardData(CF_UNICODETEXT);
		if (hGlobal == NULL)
			return text;
		const LPCWSTR data = reinterpret_cast<LPCWSTR>(::GlobalLock(hGlobal));
		if (data != NULL)
		{
			text = data;
			::GlobalUnlock(hGlobal);
		}
		return text;
	}

	~Clipboard()
	{
		if (opened_)
			::CloseClipboard();
	}

private:
	const bool opened_;
};

UINT_PTR g_timerId;

void DumpClipboard(HWND window)
{
	const auto owner = ::GetClipboardOwner();
	if (owner == window)
		return;

	Clipboard c(window);
	if (!c.IsValid())
	{
		if (g_timerId == 0)
			g_timerId = ::SetTimer(window, 1, 10, nullptr);
		return;
	}
	if (g_timerId != 0)
	{
		::KillTimer(window, g_timerId);
		g_timerId = 0;
	}

	const auto text = c.GetText();
	if (!text.has_value())
		return;

	if (::EmptyClipboard())
	{
		const auto length = text->length();
		const HANDLE hGlobal = ::GlobalAlloc(GMEM_MOVEABLE, (length + 1)*sizeof(WCHAR));
		if (hGlobal == NULL)
			return;
		const LPWSTR data = reinterpret_cast<LPWSTR>(::GlobalLock(hGlobal));
		::CopyMemory(data, text->data(), length * sizeof(WCHAR));
		data[length] = L'\0';
		::GlobalUnlock(hGlobal);
		::SetClipboardData(CF_UNICODETEXT, hGlobal);
	}
}


//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
	case WM_CLIPBOARDUPDATE:
		::OutputDebugStringA("!!! Clipboard update");
		DumpClipboard(hWnd);
		break;
	case WM_TIMER:
		DumpClipboard(hWnd);
		break;
	case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
				::OutputDebugStringA("!!! test");
				DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);


            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
