#pragma once

#include <Windows.h>
#include <DirectXMath.h>

#include <memory>
#include <exception>

#include "Strings.h"

namespace ClayEngine
{
	namespace Platform
	{
		constexpr auto c_min_window_width = 320U;
		constexpr auto c_min_window_height = 240U;

		constexpr auto c_default_window_width = 1920U;
		constexpr auto c_default_window_height = 1080U;

		inline void ThrowIfFailed(HRESULT hr)
		{
			if (FAILED(hr))
			{
				// Set a breakpoint on this line to catch DirectX API errors
				throw std::exception();
			}
		}
		inline void ThrowIfFailed(HRESULT hr, String reason)
		{
			if (FAILED(hr))
			{
				// Set a breakpoint on this line to catch DirectX API errors
				throw std::exception(reason.c_str());
			}
		}

		inline bool PlatformStart()
		{
			if (FAILED(CoInitializeEx(nullptr, COINITBASE_MULTITHREADED))) return false;
			if (!DirectX::XMVerifyCPUSupport()) return false;

			return true;
		}
		inline void PlatformStop()
		{
			CoUninitialize();
		}
	}
}