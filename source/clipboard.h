#pragma once

#include "winsdk.h"

class GlobalBuffer final
{
public:
	~GlobalBuffer()
	{
		Free();
	}

	bool Allocate(SIZE_T size)
	{
		Free();
		handle_ = ::GlobalAlloc(GMEM_MOVEABLE, size);
		return handle_ != NULL;
	}

	void Free()
	{
		if (handle_)
		{
			::GlobalFree(handle_);
			handle_ = nullptr;
		}
	}

	void Attach(HANDLE handle)
	{
		Free();
		handle_ = handle;
	}

	HANDLE Release()
	{
		const auto result = handle_;
		handle_ = nullptr;
		return result;
	}

	class Lock final
	{
	public:
		explicit Lock(GlobalBuffer& buffer)
			: Lock(buffer.handle_)
		{}

		explicit Lock(HANDLE handle)
			: handle_(handle)
			, data_(::GlobalLock(handle_))
		{}

		~Lock()
		{
			if (data_)
				::GlobalUnlock(handle_);
		}

		bool IsValid() const
		{
			return data_ != nullptr;
		}

		LPVOID Data() const
		{
			return data_;
		}

	private:
		HANDLE handle_;
		LPVOID data_;
	};

private:
	HANDLE handle_ = nullptr;
};

template<typename Char>
unsigned GetStringLength(const Char* string) noexcept
{
	const Char* scan;
	for (scan = string; *scan; ++scan) {}
	return static_cast<unsigned>(scan - string);
}

template<typename From, typename To, typename Size>
void CopyData(To* dest, const From* source, Size size) noexcept
{
	const auto from = reinterpret_cast<const BYTE*>(source);
	const auto to = reinterpret_cast<BYTE*>(dest);

	for (Size i = 0; i < size; ++i)
		to[i] = from[i];
}

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
		const auto length = GetStringLength(reinterpret_cast<LPCWSTR>(sourceLock.Data()));
		const auto sizeInBytes = (length + 1) * sizeof(*source);

		if (!buffer.Allocate(sizeInBytes))
			return false;

		GlobalBuffer::Lock destLock(buffer);
		if (!destLock.IsValid())
			return false;

		CopyData(destLock.Data(), source, sizeInBytes);

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
