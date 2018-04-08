#pragma once

#include "winsdk.h"
#include <optional>
#include <string>

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
