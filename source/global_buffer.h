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
