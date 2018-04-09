#include "winsdk.h"

#include "clipboard.h"
#include "res/resource.h"

const LPCWSTR WindowClassName = L"TextClipboardClass";

UINT_PTR g_timerId;

void ClearTextFormattingInClipboard(HWND window)
{
	const auto clipboardOwnerWindow = ::GetClipboardOwner();
	if (clipboardOwnerWindow == window)
		return;

	Clipboard clipboard(window);
	if (!clipboard.IsValid())
	{
		if (g_timerId == 0)
			g_timerId = ::SetTimer(window, 1, 10, NULL);
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

ATOM RegisterMainWindowClass(LPCWSTR windowClassName, HINSTANCE instance)
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
	options.hCursor        = LoadCursor(NULL, IDC_ARROW);
    options.hbrBackground  = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    options.lpszClassName  = windowClassName;
	options.lpszMenuName   = NULL;

    return RegisterClassExW(&options);
}

bool InitInstance(HINSTANCE instance)
{
	if (NULL != ::FindWindow(WindowClassName, NULL))
		return false;

	if (0 == RegisterMainWindowClass(WindowClassName, instance))
		return false;

	const HWND window = CreateWindowW(WindowClassName, NULL, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, instance, NULL);

	if (!window)
		return false;

	if (!::AddClipboardFormatListener(window))
		return false;

	ShowWindow(window, SW_SHOWNORMAL);
	UpdateWindow(window);

	return true;
}

void EntryPoint()
{
	if (!InitInstance(::GetModuleHandle(NULL)))
		::ExitProcess(1);

	MSG message;
	while (GetMessage(&message, NULL, 0, 0))
	{
		TranslateMessage(&message);
		DispatchMessage(&message);
	}
	::ExitProcess(0);
}
