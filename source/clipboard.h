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

	~Clipboard()
	{
		if (opened_)
			::CloseClipboard();
	}

	bool IsOpened() const
	{
		return opened_;
	}

	bool GetAsUnicodeText(GlobalBuffer& buffer) const
	{
		if (!opened_)
			return false;

		if (!::IsClipboardFormatAvailable(CF_UNICODETEXT))
			return false;

		const HANDLE unicodeTextHandle = ::GetClipboardData(CF_UNICODETEXT);
		if (unicodeTextHandle == NULL)
			return false;

		const auto sizeInBytes = ::GlobalSize(unicodeTextHandle);
		if (sizeInBytes == 0)
			return false;

		if (!buffer.Allocate(sizeInBytes))
			return false;

		GlobalBuffer::ReadOnlyLock sourceLock(unicodeTextHandle);
		if (!sourceLock.IsValid())
			return false;

		GlobalBuffer::Lock destLock(buffer);
		if (!destLock.IsValid())
			return false;

		memcpy(destLock.Data(), sourceLock.Data(), sizeInBytes);

		return true;
	}

	bool ReplaceWithUnicodeText(GlobalBuffer& buffer)
	{
		if (!opened_)
			return false;

		if (!::EmptyClipboard())
			return false;

		const HANDLE data = buffer.Release();
		if (NULL == ::SetClipboardData(CF_UNICODETEXT, data))
		{
			buffer.Attach(data);
			return false;
		}
		return true;
	}

private:
	const bool opened_;
};
