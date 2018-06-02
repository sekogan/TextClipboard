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

	template<typename BufferType, typename DataPointerType>
	class LockImpl final
	{
	public:
		explicit LockImpl(BufferType& buffer)
			: LockImpl(buffer.handle_)
		{}

		explicit LockImpl(HANDLE handle)
			: handle_(handle)
			, data_(::GlobalLock(handle_))
		{}

		~LockImpl()
		{
			if (data_)
				::GlobalUnlock(handle_);
		}

		bool IsValid() const
		{
			return data_ != nullptr;
		}

		DataPointerType Data() const
		{
			return data_;
		}

	private:
		const HANDLE handle_;
		const DataPointerType data_;
	};

	using Lock = LockImpl<GlobalBuffer, LPVOID>;
	using ReadOnlyLock = LockImpl<const GlobalBuffer, LPCVOID>;

private:
	HANDLE handle_ = nullptr;
};
