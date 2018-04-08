#include "header.h"
#include "text_clipboard.h"
#include <stdexcept>
#include <optional>

HINSTANCE hInst;
WCHAR szTitle[] = L"TextClipboard";
WCHAR szWindowClass[] = L"TextClipboardClass";

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    MyRegisterClass(hInstance);

    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex = {};

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TESTWIN32APP));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance;

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

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
	case WM_CLIPBOARDUPDATE:
		DumpClipboard(hWnd);
		break;
	case WM_TIMER:
		DumpClipboard(hWnd);
		break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
