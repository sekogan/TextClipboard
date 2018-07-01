#include "winsdk.h"

#include "clipboard.h"
#include "crc32.h"
#include "micro_crt.h"
#include "res/resource.h"

const LPCWSTR WindowClassName = L"TextClipboardClass";
const UINT InitialDelayMs = 50;
const UINT RetryIntervalMs = 10;
const UINT MaxNumberOfAttempts = (2*60*1000 - InitialDelayMs) / RetryIntervalMs;

UINT_PTR g_timerId;
UINT g_numberOfAttempts;
HWND g_window;
crc_t g_lastClearedTextCrc;
DWORD g_lastClearedTextTime;

crc_t CalculateCrc(const GlobalBuffer& text)
{
	GlobalBuffer::ReadOnlyLock lock(text);
	const auto length = wcslen(reinterpret_cast<LPCWSTR>(lock.Data()));
	const auto sizeInBytes = (length + 1) * sizeof(WCHAR);

	crc_t crc = crc_init();
	crc = crc_update(crc, lock.Data(), sizeInBytes);
	crc = crc_finalize(crc);

	return crc;
}

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

	const crc_t crc = CalculateCrc(text);
	const DWORD now = ::GetTickCount();
	const bool isDoubleCopy = now - g_lastClearedTextTime < 1000 &&
		crc == g_lastClearedTextCrc;
	if (isDoubleCopy)
		return true; // Do not clear formatting on second copy
	g_lastClearedTextCrc = crc;
	g_lastClearedTextTime = now;

	clipboard.ReplaceWithUnicodeText(text);
	return true;
}

void StartTimer(UINT intervalMs)
{
	g_timerId = ::SetTimer(g_window, 1, intervalMs, NULL);
}

void StopTimer()
{
	if (g_timerId != 0)
	{
		::KillTimer(g_window, g_timerId);
		g_timerId = 0;
	}
}

void OnClipboardUpdate()
{
	StopTimer();
	g_numberOfAttempts = 0;
	StartTimer(InitialDelayMs); // Do not do the job immediately because it may interfere
								// with other software also accessing the clipboard
}

void OnTimer()
{
	StopTimer();
	if (++g_numberOfAttempts > MaxNumberOfAttempts)
		return;
	if (TryToClearTextFormattingInClipboard())
		return;
	StartTimer(RetryIntervalMs);
}

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CLIPBOARDUPDATE:
		OnClipboardUpdate();
		break;
	case WM_TIMER:
		OnTimer();
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

	g_window = CreateWindowW(WindowClassName, NULL, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, instance, NULL);

	if (!g_window)
		return false;

	if (!::AddClipboardFormatListener(g_window))
		return false;

	OnClipboardUpdate();

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
