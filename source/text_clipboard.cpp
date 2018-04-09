#include "winsdk.h"

#include "clipboard.h"
#include "resource.h"

const LPCWSTR g_windowClass = L"TextClipboardClass";

HINSTANCE g_instance;
UINT_PTR g_timerId;

ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE);
LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);

void EntryPoint()
{
	const HINSTANCE instance = ::GetModuleHandle(NULL);
    MyRegisterClass(instance);

	if (!InitInstance(instance))
		::ExitProcess(1);

    MSG message;
    while (GetMessage(&message, nullptr, 0, 0))
    {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }
	::ExitProcess(0);
}

ATOM MyRegisterClass(HINSTANCE instance)
{
	WNDCLASSEXW options;

    options.cbSize         = sizeof(options);
    options.style          = CS_HREDRAW | CS_VREDRAW;
    options.lpfnWndProc    = WindowProc;
    options.cbClsExtra     = 0;
    options.cbWndExtra     = 0;
    options.hInstance      = instance;
    options.hIcon          = LoadIcon(instance, MAKEINTRESOURCE(IDI_MAIN));
	options.hIconSm        = options.hIcon;
	options.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    options.hbrBackground  = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    options.lpszClassName  = g_windowClass;
	options.lpszMenuName   = nullptr;

    return RegisterClassExW(&options);
}

BOOL InitInstance(HINSTANCE instance)
{
	g_instance = instance;

	const HWND window = CreateWindowW(g_windowClass, nullptr, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, instance, nullptr);

	if (!window)
	{
		return FALSE;
	}

	::AddClipboardFormatListener(window); // Todo: check the result (BOOL)

	ShowWindow(window, SW_SHOWNORMAL);
	UpdateWindow(window);

	return TRUE;
}

void ClearTextFormattingInClipboard(HWND window)
{
	const auto clipboardOwnerWindow = ::GetClipboardOwner();
	if (clipboardOwnerWindow == window)
		return;

	Clipboard clipboard(window);
	if (!clipboard.IsValid())
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

	GlobalBuffer buffer;
	if (!clipboard.GetText(buffer))
		return;

	if (::EmptyClipboard())
	{
		const HANDLE data = buffer.Release();
		if (NULL == ::SetClipboardData(CF_UNICODETEXT, data))
			buffer.Attach(data);
	}
}

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
	case WM_CLIPBOARDUPDATE:
		ClearTextFormattingInClipboard(window);
		break;
	case WM_TIMER:
		ClearTextFormattingInClipboard(window);
		break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(window, message, wParam, lParam);
    }
    return 0;
}
