#pragma once

#include <Windows.h>
#include <DirectXMath.h>

#include <memory>
#include <exception>

#include "Storage.h"
#include "Strings.h"

namespace ClayEngine
{
	namespace Platform
	{
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

		class ClayEngineEntryPoint
		{
			HINSTANCE m_hInstance;
			int m_nShowCmd;

			ClayEngine::Platform::JsonFilePtr m_clayengine = {};
			ClayEngine::Platform::JsonFilePtr m_startup = {};

		public:
			ClayEngineEntryPoint(HINSTANCE hInstance, int nShowCmd)
			{
				m_hInstance = hInstance;
				m_nShowCmd = nShowCmd;

				ThrowIfFailed(CoInitializeEx(nullptr, COINITBASE_MULTITHREADED), "Core CRITICAL: Failed to Initialize COM");
				if (!DirectX::XMVerifyCPUSupport()) throw std::exception("Core CRITICAL: CPU Unsupported");

				// Intentionally hard-coded, this is your default startup file
				m_clayengine = std::make_unique<ClayEngine::Platform::JsonFile>("clayengine.json");

				//std::locale::global(std::locale(m_clayengine->GetValue<std::string>("locale")));

				// Inside the above file, define the startup key to script the startup process for your game
				m_startup = std::make_unique<ClayEngine::Platform::JsonFile>(m_clayengine->GetValue<std::string>("startup"));




				// EPIPHANY: Here we need to set up our message systems and other things. We've got our application running, 
				// we've initialized some APIs, we need to set up our message handling I/O system now, so we can route messages
				// from the OS and the debug window's stdio in and out of the same places, and eventually also tap this for
				// the GUI system console display as well.

				//std::for_each( , , []() {} );

				// After the message handling system is stood up, iterate the startup collection and wire up the windows and
				// I/O devices to their appropriate interfaces

				// "debug"

				// "client"

				// "guiserver"

				// "headless"

				// "etc..."


			}
			~ClayEngineEntryPoint()
			{
				m_startup.reset();
				m_clayengine.reset();

				CoUninitialize();
			}

			HINSTANCE GetInstance()
			{
				return m_hInstance;
			}
			int GetFlags()
			{
				return m_nShowCmd;
			}

			void Run()
			{
				while (true)
				{
					// Running...
				}
			}
		};
		using ClayEngineEntryPointPtr = std::unique_ptr<ClayEngineEntryPoint>;
		using ClayEngineEntryPointRaw = ClayEngineEntryPoint*;
	}
}