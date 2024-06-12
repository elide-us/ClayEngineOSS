#pragma once

#include "Platform.h"

namespace ClayEngine
{
	namespace Platform
	{
		class ClayEngineDebugConsole
		{
		public:
			ClayEngineDebugConsole()
			{
				if (AllocConsole())
				{
					FILE* file = nullptr;
					_wfreopen_s(&file, L"CONIN$", L"r", stdin);
					_wfreopen_s(&file, L"CONOUT$", L"w", stdout);
					_wfreopen_s(&file, L"CONOUT$", L"w", stderr);
					std::wcout << L"PlatformStart INFO: Allocated default console" << std::endl;
				}
				else throw;

			}
			~ClayEngineDebugConsole()
			{
				FreeConsole();
			}

			// This is just a stub for now, this will eventually send its input to the message system
			void Run()
			{
				auto run = true;
				while (run)
				{
					Unicode input;
					std::wcin >> input;

					if (input == L"quit")
					{
						run = false;
						std::cout << "Beginning system shutdown, press ENTER to exit..." << std::endl;
					}
				}
			}
		};
		using ClayEngineDebugConsolePtr = std::unique_ptr<ClayEngineDebugConsole>;

        // Forward Declarations
        LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

		/// <summary>
		/// This is the primary class for a window, and handles all of the message callbacks
		/// </summary>
		class WindowSystem
		{
			HINSTANCE m_instance_handle = NULL;
			int m_show_flags = 0;

			HWND m_window_handle = NULL;
			Unicode m_window_name = L"";
			Unicode m_class_name = L"";
			
			bool m_in_sizemove = false;
			bool m_in_suspend = false;
			bool m_minimized = false;
			bool m_fullscreen = false;

			RECT m_window_size = { 0, 0, c_default_window_width, c_default_window_height };

		public:
			WindowSystem(HINSTANCE hInstance, int nCmdShow = SW_SHOWDEFAULT, Unicode className = L"default", Unicode windowName = L"Default");
			~WindowSystem();

			HWND GetWindowHandle()
			{
				if (m_window_handle)
					return m_window_handle;
			}
			LONG GetWindowWidth()
			{
				return m_window_size.right - m_window_size.left;
			}
			LONG GetWindowHeight()
			{
				return m_window_size.bottom - m_window_size.top;
			}

			bool GetInSizeMove()
			{
				return m_in_sizemove;
			}
			bool GetInSuspend()
			{
				return m_in_suspend;
			}
			bool GetMinimized()
			{
				return m_minimized;
			}
			bool GetFullscreen()
			{
				return m_fullscreen;
			}

			void SetInSizeMove(bool inSizeMove)
			{
				m_in_sizemove = inSizeMove;
			}
			void SetInSuspend(bool inSuspend)
			{
				m_in_suspend = inSuspend;
			}
			void SetMinimized(bool minimized)
			{
				m_minimized = minimized;
			}
			void SetFullscreen(bool fullscreen)
			{
				m_fullscreen = fullscreen;
			}

			// Callbacks like the old game object in the template
			void OnActivated()
			{
				std::cout << "OnActivated" << std::endl;
			}

			void UpdateWindowSize()
			{
				std::cout << "UpdateWindowSize" << std::endl;
			}

			void OnSuspending()
			{
				std::cout << "OnSuspending" << std::endl;
			}

			void OnResuming()
			{
				std::cout << "OnResuming" << std::endl;
			}

			void OnDeactivated()
			{
				std::cout << "OnDeactivated" << std::endl;
			}
		};
		using WindowSystemPtr = std::unique_ptr<WindowSystem>;
		using WindowSystemRaw = WindowSystem*;
	}
}