#include "winsdk.h"

#include "clipboard.h"
#include "res/resource.h"

const LPCWSTR WindowClassName = L"TextClipboardClass";
const UINT RetryIntervalMs = 10;
const UINT MaxNumberOfRetries = 2*60*1000 / RetryIntervalMs;

UINT_PTR g_retryTimerId;
UINT g_numberOfRetries;
HWND g_window;

bool TryToClearTextFormattingInClipboard()
{
	const auto clipboardOwnerWindow = ::GetClipboardOwner();
	if (clipboardOwnerWindow == g_window)
		return true; // Done already

	Clipboard clipboard(g_window);
	if (!clipboard.IsOpened())
		return false;

	GlobalBuffer text;
	if (!clipboard.GetAsUnicodeText(text))
		return true; // No text in the clipboard

	clipboard.ReplaceWithUnicodeText(text);
	return true;
}

void StartRetries()
{
	g_retryTimerId = ::SetTimer(g_window, 1, RetryIntervalMs, NULL);
}

void StopRetries()
{
	g_numberOfRetries = 0;
	if (g_retryTimerId != 0)
	{
		::KillTimer(g_window, g_retryTimerId);
		g_retryTimerId = 0;
	}
}

void ClearTextFormattingInClipboard()
{
	StopRetries();
	if (!TryToClearTextFormattingInClipboard())
		StartRetries();
}

void RetryClearTextFormattingInClipboard()
{
	if (++g_numberOfRetries > MaxNumberOfRetries)
	{
		StopRetries();
		return;
	}
	if (TryToClearTextFormattingInClipboard())
		StopRetries();
}

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CLIPBOARDUPDATE:
		ClearTextFormattingInClipboard();
		break;
	case WM_TIMER:
		RetryClearTextFormattingInClipboard();
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(g_window, message, wParam, lParam);
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

	g_window = CreateWindowW(WindowClassName, L"aaa", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, instance, NULL);

	if (!g_window)
		return false;

	if (!::AddClipboardFormatListener(g_window))
		return false;

	ClearTextFormattingInClipboard();

	ShowWindow(g_window, SW_SHOWNORMAL);
	UpdateWindow(g_window);

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
