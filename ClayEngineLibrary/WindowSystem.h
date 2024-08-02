#pragma once
/******************************************************************************/
/*                                                                            */
/* ClayEngineOSS (C) 2024 Elideus                                             */
/* WindowSystem header provides a Windows GUI window that provides messages   */
/* from the OS and callback hooks to which other engine objects may subscribe */
/* https://github.com/elide-us                                                */
/*                                                                            */
/******************************************************************************/

#include <Windows.h>
#include "Strings.h"
#include "Services.h"

namespace ClayEngine
{
	using Message = std::function<void(WPARAM, LPARAM)>;
	using Messages = std::vector<Message>;

	using Function = std::function<void()>;
	using Functions = std::vector<Function>;

	//constexpr auto c_default_window_width = 1920U;
	//constexpr auto c_default_window_height = 1080U;
	constexpr auto c_default_window_width = 800U;
	constexpr auto c_default_window_height = 600U;

	/// <summary>
	/// This is the primary class for a window, and handles all of the message callbacks
	/// </summary>
	class WindowSystem
	{
		AffinityData m_affinity;

		HINSTANCE m_instance_handle = NULL;
		int m_show_flags = SW_SHOWDEFAULT;

		HWND m_window_handle = NULL;
		RECT m_window_size = { 0, 0, c_default_window_width, c_default_window_height };
		Unicode m_class_name = L"defaultWindowClassName";
		Unicode m_window_name = L"ClayEngine Default Window Title";
			
		bool m_in_sizemove = false;
		bool m_in_suspend = false;
		bool m_minimized = false;
		bool m_fullscreen = false;

		Functions s_onchanged = {};
		Functions s_onactivated = {};
		Functions s_ondeactivated = {};
		Functions s_onsuspended = {};
		Functions s_onresumed = {};
		
		Messages m_on_char = {};
		Messages m_on_keydown = {};
		Messages m_on_keyup = {};

	public:
		WindowSystem(AffinityData affinityId, HINSTANCE hInstance, int nCmdShow, Unicode className, Unicode windowName);
		~WindowSystem();

		#pragma region Window class accessors
		const HWND GetWindowHandle() const
		{
			if (m_window_handle)
				return m_window_handle;
			else return NULL;
		}
		const LONG GetWindowWidth() const
		{
			return m_window_size.right - m_window_size.left;
		}
		const LONG GetWindowHeight() const
		{
			return m_window_size.bottom - m_window_size.top;
		}
		const RECT& GetWindowSize() const
		{
			return m_window_size;
		}
		void SetWindowSize()
		{
			SetWindowPos(m_window_handle, HWND_TOP, m_window_size.left, m_window_size.top, m_window_size.right, m_window_size.bottom, SWP_SHOWWINDOW);			//SetWindowSize(size);
		}
		const Unicode& GetWindowName() const
		{
			return m_window_name;
		}
		void SetWindowName(Unicode windowName)
		{
			m_window_name = windowName;
			SetWindowTextW(m_window_handle, m_window_name.c_str());
		}
		#pragma endregion

		#pragma region State management accessors
		bool GetInSizeMove()
		{
			return m_in_sizemove;
		}
		void SetInSizeMove(bool inSizeMove)
		{
			m_in_sizemove = inSizeMove;
		}
		bool GetInSuspend()
		{
			return m_in_suspend;
		}
		void SetInSuspend(bool inSuspend)
		{
			m_in_suspend = inSuspend;
		}
		bool GetMinimized()
		{
			return m_minimized;
		}
		void SetMinimized(bool minimized)
		{
			m_minimized = minimized;
		}
		bool GetFullscreen()
		{
			return m_fullscreen;
		}
		void SetFullscreen(bool fullscreen)
		{
			m_fullscreen = fullscreen;
		}
		#pragma endregion

		#pragma region Window event callback handlers
		void OnActivated()
		{
			for (auto& element : s_onactivated) { element(); }
		}
		void OnResuming()
		{
			for (auto& element : s_onresumed) { element(); }
		}
		void OnSuspending()
		{
			for (auto& element : s_onsuspended) { element(); }
		}
		void OnDeactivated()
		{
			for (auto& element : s_ondeactivated) { element(); }
		}
		void OnChanged()
		{
			GetClientRect(m_window_handle, &m_window_size);

			for (auto& element : s_onchanged) { element(); }
		}

		void AddOnCharCallback(Message fn)
		{
			m_on_char.push_back(fn);
		}
		void OnChar(WPARAM wParam, LPARAM lParam)
		{
			for (auto& element : m_on_char) { element(wParam, lParam); }
		}
		void AddOnKeyDownCallback(Message fn)
		{
			m_on_keydown.push_back(fn);
		}
		void OnKeyDown(WPARAM wParam, LPARAM lParam)
		{
			for (auto& element : m_on_keydown) { element(wParam, lParam); }
		}
		void AddOnKeyUpCallback(Message fn)
		{
			m_on_keyup.push_back(fn);
		}
		void OnKeyUp(WPARAM wParam, LPARAM lParam)
		{
			for (auto& element : m_on_keyup) { element(wParam, lParam); }
		}
		#pragma endregion
	};
	using WindowSystemPtr = std::unique_ptr<WindowSystem>;
	using WindowSystemRaw = WindowSystem*;

}