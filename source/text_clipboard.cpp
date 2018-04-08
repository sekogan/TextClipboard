#include "header.h"
#include "text_clipboard.h"
#include <stdexcept>
#include <optional>

HINSTANCE g_instance;
WCHAR g_windowTitle[] = L"TextClipboard";
WCHAR g_windowClass[] = L"TextClipboardClass";

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

    options.cbSize = sizeof(WNDCLASSEX);
    options.style          = CS_HREDRAW | CS_VREDRAW;
    options.lpfnWndProc    = WindowProc;
    options.cbClsExtra     = 0;
    options.cbWndExtra     = 0;
    options.hInstance      = instance;
    options.hIcon          = LoadIcon(instance, MAKEINTRESOURCE(IDI_TESTWIN32APP));
    options.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    options.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    options.lpszClassName  = g_windowClass;
    options.hIconSm        = LoadIcon(options.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&options);
}

BOOL InitInstance(HINSTANCE instance, int showMode)
{
	g_instance = instance;

	HWND window = CreateWindowW(g_windowClass, g_windowTitle, WS_OVERLAPPEDWINDOW,
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

class Clipboard
{
public:
	explicit Clipboard(HWND window)
		: opened_(!!::OpenClipboard(window))
	{}

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

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
	case WM_CLIPBOARDUPDATE:
		DumpClipboard(window);
		break;
	case WM_TIMER:
		DumpClipboard(window);
		break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(window, message, wParam, lParam);
    }
    return 0;
}
