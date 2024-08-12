#pragma once
#include <exception>
#include <wrl.h>

#include "Log.h"
// temporary until engine assert is here.
inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		ERROR(LOG_GRAPHICS, "DirectX12 Exception HRESULT: 0x%08X\n", (unsigned)hr);
		throw std::exception();
	}
}
