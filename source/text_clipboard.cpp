#include "winsdk.h"

#include "clipboard.h"
#include "resource.h"

const LPCWSTR g_windowTitle = L"TextClipboard";
const LPCWSTR g_windowClass = L"TextClipboardClass";

HINSTANCE g_instance;
UINT_PTR g_timerId;

ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(
	HINSTANCE instance,
    HINSTANCE /*prevInstance*/,
    LPWSTR /*commandLine*/,
    int showMode
)
{
    MyRegisterClass(instance);

    if (!InitInstance(instance, showMode))
    {
        return FALSE;
    }

    MSG message;
    while (GetMessage(&message, nullptr, 0, 0))
    {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    return static_cast<int>(message.wParam);
}

ATOM MyRegisterClass(HINSTANCE instance)
{
	WNDCLASSEXW options{};

    options.cbSize         = sizeof(options);
    options.style          = CS_HREDRAW | CS_VREDRAW;
    options.lpfnWndProc    = WindowProc;
    options.cbClsExtra     = 0;
    options.cbWndExtra     = 0;
    options.hInstance      = instance;
    options.hIcon          = LoadIcon(instance, MAKEINTRESOURCE(IDI_APP));
	options.hIconSm        = LoadIcon(options.hInstance, MAKEINTRESOURCE(IDI_SMALL));
	options.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    options.hbrBackground  = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    options.lpszClassName  = g_windowClass;

    return RegisterClassExW(&options);
}

BOOL InitInstance(HINSTANCE instance, int showMode)
{
	g_instance = instance;

	const HWND window = CreateWindowW(g_windowClass, g_windowTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, instance, nullptr);

	if (!window)
	{
		return FALSE;
	}

	::AddClipboardFormatListener(window); // Todo: check the result (BOOL)

	ShowWindow(window, showMode);
	UpdateWindow(window);

	return TRUE;
}

void ClearTextFormattingInClipboard(HWND window)
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
