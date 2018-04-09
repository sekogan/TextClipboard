#pragma once

#include "winsdk.h"
#include "global_buffer.h"
#include "micro_crt.h"

class Clipboard final
{
public:
	explicit Clipboard(HWND window)
		: opened_(!!::OpenClipboard(window))
	{}

	bool IsValid() const
	{
		return opened_;
	}

	bool GetText(GlobalBuffer& buffer) const
	{
		if (!::IsClipboardFormatAvailable(CF_UNICODETEXT))
			return false;

		GlobalBuffer::Lock sourceLock(::GetClipboardData(CF_UNICODETEXT));
		if (!sourceLock.IsValid())
			return false;

		const auto source = reinterpret_cast<LPCWSTR>(sourceLock.Data());
		const auto length = wcslen(reinterpret_cast<LPCWSTR>(sourceLock.Data()));
		const auto sizeInBytes = (length + 1) * sizeof(*source);

		if (!buffer.Allocate(sizeInBytes))
			return false;

		GlobalBuffer::Lock destLock(buffer);
		if (!destLock.IsValid())
			return false;

		memcpy(destLock.Data(), source, sizeInBytes);

		return true;
	}

	~Clipboard()
	{
		if (opened_)
			::CloseClipboard();
	}

private:
	const bool opened_;
};
