#pragma once
/******************************************************************************/
/*                                                                            */
/* ClayEngineOSS (C) 2024 Elideus                                             */
/* Input handling system                                                      */
/* https://github.com/elide-us                                                */
/*                                                                            */
/******************************************************************************/

#include "Strings.h"
#include "Services.h"

#include "Mouse.h"

#include <ctime>

namespace ClayEngine
{
	constexpr auto c_max_stringbuffer_length = 1024LL;
	constexpr auto c_max_scrollback_length = 20LL;
	constexpr auto c_max_displaybuffer_length = 256LL;
	constexpr auto c_max_stringarray_length = 2048LL;

	// Create context objects for each of these that sets a Size in a normal manner...

	template<size_t Size>
	class StringBuffer
	{
		static_assert(Size >= 0);
		static_assert(Size <= c_max_stringbuffer_length);

		using Buffer = std::array<wchar_t, Size>;
		Buffer m_buffer = {};
		std::mutex m_buffer_mtx = {};

		// Internally, we want to store this as a signed long int to support negative index movement
		int64_t m_size = static_cast<int64_t>(Size);

		int64_t m_carat = 0;
		int64_t m_end = 0;

		bool m_is_overwrite = false;

	public:
		StringBuffer() = default;
		~StringBuffer() = default;

		/// <summary>
		/// Return to the caller where in the buffer the carat is currently located (use for rendering cursor, for example)
		/// </summary>
		int GetCarat()
		{
			m_buffer_mtx.lock();
			auto r = static_cast<int>(m_carat);
			m_buffer_mtx.unlock();
			return r;
		}

		int GetBufferLength()
		{
			m_buffer_mtx.lock();
			auto r = static_cast<int>(m_end);
			m_buffer_mtx.unlock();
			return r;
		}

		/// <summary>
		/// Move the carat forward or backwards in the buffer, bounds are checked against size constant
		/// </summary>
		void MoveCarat(const int64_t offset)
		{
			if (m_carat == 0 && m_end == 0)
			{
				WriteLine("StringBuffer MoveCarat DEBUG: Cannot move carat on empty string.");
				return;
			}

			auto v = m_carat + offset;

			m_buffer_mtx.lock();
			if (v >= m_end)
			{
				m_carat = m_end;
			}
			else if (v >= m_size)
			{
				m_carat = m_size;
			}
			else if (v < 0)
			{
				m_carat = 0;
			}
			else
			{
				m_carat = v;
			}
			m_buffer_mtx.unlock();
		}

		/// <summary>
		/// Return if the buffer is in insert or overwrite mode, this controls the behavior of how we parse OnChar messages.
		/// </summary>
		bool GetOverwrite()
		{
			m_buffer_mtx.lock();
			auto v = m_is_overwrite;
			m_buffer_mtx.unlock();

			return v;
		}

		/// <summary>
		/// Set the buffer insert/overwrite mode to the mode that is provided.
		/// </summary>
		/// <param name="value">true = overwrite, false = insert</param>
		void SetOverwrite(bool overwrite)
		{
			m_buffer_mtx.lock();
			m_is_overwrite = overwrite;
			m_buffer_mtx.unlock();
		}

		/// <summary>
		/// Toggle whatever the current insert/overwrite mode is to the other mode
		/// </summary>
		void ToggleOverwrite()
		{
			m_buffer_mtx.lock();
			m_is_overwrite = !m_is_overwrite;
			m_buffer_mtx.unlock();
		}

		/// <summary>
		/// A function to insert a character into the buffer, will push chars forward or append
		/// </summary>
		void InsertChar(const wchar_t character)
		{
			m_buffer_mtx.lock();
			if (m_carat >= m_size) //TODO: What to do with full buffer?
			{
				WriteLine("StringBuffer InsertChar DEBUG: We don'm_thread know what to do with a full buffer yet, doing nothing.");
				m_buffer_mtx.unlock();
				return;
			}

			if (m_carat == m_end) // Normal condition
			{
				m_buffer[m_carat] = character;

				++m_carat;
				++m_end;
			}
			if (m_carat < m_end)
			{
				if (m_is_overwrite)
				{
					// Replace character in array with character provided
					m_buffer[m_carat] = character;

					++m_carat;
				}
				else
				{
					// Push values between carat and end forward one... Check 1024 bounds
					auto v = m_end + 1;

					if (v >= m_size)
					{
						m_buffer_mtx.unlock();
						return;
					}

					for (; v > m_carat; --v)
					{
						m_buffer[v] = m_buffer[v - 1];
					}
					m_buffer[m_carat] = character;

					++m_carat;
					++m_end;
				}
			}
			m_buffer_mtx.unlock();
		}

		/// <summary>
		/// Function will delete the character before the cursor in the buffer and move the carat back
		/// </summary>
		void RemovePrev()
		{
			m_buffer_mtx.lock();
			if (m_carat == 0)
			{
				WriteLine("StringArray::RemovePrev DEBUG: We're at the beginning of the line, can't backspace.");
				m_buffer_mtx.unlock();
				return;
			}

			if (m_carat == m_end)
			{
				--m_carat;

				m_buffer[m_carat] = 0;

				--m_end;
			}
			else
			{
				--m_carat;
				auto v = m_carat;

				for (; v < m_end; ++v)
				{
					m_buffer[v] = m_buffer[v + 1ull];
				}
				m_buffer[v] = 0;

				--m_end;
			}
			m_buffer_mtx.unlock();
		}

		/// <summary>
		/// Function will delete the character after the cursor and shuffle the buffer back
		/// </summary>
		void RemoveNext()
		{
			m_buffer_mtx.lock();
			if (m_end == 0)
			{
				WriteLine("StringArray::RemoveNext DEBUG: There's no text to delete.");
				m_buffer_mtx.unlock();
				return;
			}

			if (m_end >= m_size)
			{
				WriteLine("StringArray::RemoveNext DEBUG: We're at the end of the line, can't delete.");
				m_buffer_mtx.unlock();
				return;
			}

			auto v = m_carat;

			for (; v < m_end; ++v)
			{
				m_buffer[v] = m_buffer[v + 1ull];
			}
			m_buffer[v] = 0;

			--m_end;
			m_buffer_mtx.unlock();
		}

		/// <summary>
		/// Returns the buffer as a Unicode (std::wstring) string
		/// </summary>
		Unicode GetString()
		{
			m_buffer_mtx.lock();
			auto r = Unicode{ m_buffer.data() };
			m_buffer_mtx.unlock();

			return r;
		}

		/// <summary>
		/// Replace the buffer with the provided Unicode string. This function does dangerous low-level memory operations
		/// while inside the mutex, be careful of locks.
		/// </summary>
		void SetString(const Unicode& string)
		{
			m_buffer_mtx.lock();
			m_buffer.fill(0);
			m_carat = 0;
			m_end = 0;

			ThrowIfFailed(memcpy_s(&m_buffer, sizeof(wchar_t) * string.length(), string.c_str(), sizeof(wchar_t) * string.length()));

			m_carat = static_cast<int64_t>(string.length());
			m_end = static_cast<int64_t>(string.length());
			m_buffer_mtx.unlock();
		}

		/// <summary>
		/// Clear the buffer, typically used when you pull the string out and stick it somewhere else so you can start a new line
		/// </summary>
		void Clear()
		{
			m_buffer_mtx.lock();
			m_buffer.fill(0);

			m_carat = 0;
			m_end = 0;
			m_buffer_mtx.unlock();
		}
	};

	template<size_t Size>
	class StringArray
	{
		static_assert(Size >= 0);
		static_assert(Size <= c_max_stringarray_length);

		using Buffer = std::array<Unicode, Size>;
		Buffer m_buffer = {};
		std::mutex m_buffer_mtx = {};

		int64_t m_size = static_cast<int64_t>(Size);

		int64_t m_carat = 0;
		int64_t m_end = 0;

		bool m_is_reset = true;

	public:
		StringArray() = default;
		~StringArray() = default;

		/// <summary>
		/// Rudimentary insert into a vector of Unicode strings
		/// </summary>
		void InsertString(const Unicode& string)
		{
			auto in_time_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
			auto out_time = tm{};

			auto ec = localtime_s(&out_time, &in_time_t);

			std::stringstream ss;
			ss << std::put_time(&out_time, "%Y-%m-%d %X");
			auto str = ToUnicode(ss.str());
			std::wstringstream wss;
			wss << L"[" << str << L"] " << string.c_str();


			if (m_carat < m_end) { m_carat = m_end; }

			if (m_carat < m_size)
			{
				m_buffer[m_carat] = wss.str();

				++m_carat;
				++m_end;
			}
			else
			{
				for (int64_t i = 1; i < m_size; ++i)
				{
					m_buffer[i - 1] = m_buffer[i];
				}

				m_buffer[m_carat - 1] = string;
			}
		}

		/// <summary>
		/// Return the string at the scrollback carat location. Copy to StringBuffer to work on it. Prefer to use
		/// InsertString to add a new copy of the edited line into the end of the buffer instead of UpdateString.
		/// </summary>
		Unicode GetString()
		{
			auto r = Unicode{ L"" };
			if (m_carat == m_end) return r;

			m_buffer_mtx.lock();
			r = m_buffer[m_carat];
			m_buffer_mtx.unlock();

			return r;
		}

		const Buffer& GetBuffer()
		{
			return m_buffer;
		}

		size_t GetCarat()
		{
			m_buffer_mtx.lock();
			auto r = static_cast<size_t>(m_carat);
			m_buffer_mtx.unlock();

			return r;
		}

		void MoveCarat(int64_t offset)
		{
			if (m_carat == 0 && m_end == 0)
			{
				WriteLine("StringArray MoveCarat DEBUG: Cannot move carat on empty buffer.");
				return;
			}

			auto v = m_carat + offset;

			m_buffer_mtx.lock();
			if (v >= m_end)
			{
				m_carat = m_end;
			}
			else if (v >= m_size)
			{
				m_carat = m_size;
			}
			else if (v < 0)
			{
				m_carat = 0;
			}
			else
			{
				m_carat = v;
			}
			m_buffer_mtx.unlock();
		}

		bool IsInScrollback()
		{
			return m_carat < m_end;
		}

		size_t GetBufferSize()
		{
			return static_cast<size_t>(m_end);
		}

		void Clear()
		{
			m_buffer_mtx.lock();
			m_buffer.fill(L"");

			m_carat = 0;
			m_end = 0;
			m_buffer_mtx.unlock();
		}
	};

	using MousePtr = std::unique_ptr<DirectX::Mouse>;
	using Tracker = DirectX::Mouse::ButtonStateTracker;

	using InputBuffer = StringBuffer<c_max_stringbuffer_length>;
	using InputBufferPtr = std::unique_ptr<InputBuffer>;
	using InputBufferRaw = InputBuffer*;

	using ScrollbackBuffer = StringArray<c_max_scrollback_length>;
	using ScrollbackBufferPtr = std::unique_ptr<ScrollbackBuffer>;
	using ScrollbackBufferRaw = ScrollbackBuffer*;

	using DisplayBuffer = StringArray<c_max_displaybuffer_length>;
	using DisplayBufferPtr = std::unique_ptr<DisplayBuffer>;
	using DisplayBufferRaw = DisplayBuffer*;

	/// <summary>
	/// Input system is the top level API for handling user input events
	/// </summary>
	class InputSystem
	{
		AffinityData m_affinity_data;

		//MousePtr m_mouse = nullptr;
		//Tracker m_tracker = {};

		InputBufferPtr m_input_buffer = nullptr;
		ScrollbackBufferPtr m_scrollback_buffer = nullptr;
		DisplayBufferPtr m_display_buffer = nullptr;

		bool m_caps_lock = false;
		bool m_shift_pressed = false;
		bool m_control_pressed = false;
		bool m_alt_pressed = false;

	public:
		InputSystem(AffinityData affinityData);
		~InputSystem();

		void OnMouseEvent(UINT message, WPARAM wParam, LPARAM lParam);
		//DirectX::Mouse::State GetMouseState();
		//DirectX::Mouse::ButtonStateTracker& GetButtonStateTracker();

		void OnKeyDown(WPARAM wParam, LPARAM lParam);
		void OnKeyUp(WPARAM wParam, LPARAM lParam);
		void OnChar(WPARAM wParam, LPARAM lParam);

		auto GetBuffer();

		auto GetInputBuffer() { return m_input_buffer.get(); }
		auto GetCursorIndex() { return m_input_buffer->GetCarat(); }
		auto GetBufferLength() { return m_input_buffer->GetBufferLength(); }
		auto GetOverwrite() { return m_input_buffer->GetOverwrite(); }

		auto GetRCBuffer() { return m_scrollback_buffer.get(); }
		auto GetRCBufferSize() { return m_scrollback_buffer->GetBufferSize(); }
		void AddRCBufferMessage(Unicode string) { m_scrollback_buffer->InsertString(string); }

		auto GetDisplayBuffer() { return m_display_buffer.get(); }
		auto GetDisplayBufferSize() { return m_display_buffer->GetBufferSize(); }
		void AddDisplayBufferMessage(Unicode string) { m_display_buffer->InsertString(string); }
	};
	using InputSystemPtr = std::unique_ptr<InputSystem>;
	using InputSystemRaw = std::unique_ptr<InputSystem>::pointer;

	//class InputHandler
	//{
	//	static inline bool m_caps_lock = false;
	//	static inline bool m_shift_pressed = false;
	//	static inline bool m_control_pressed = false;
	//	static inline bool m_alt_pressed = false;

	//	static inline InputSystemPtr m_input;
	//	static inline InputBufferRaw m_input_buffer;
	//	static inline ScrollbackBufferRaw m_scrollback_buffer;
	//	static inline DisplayBufferRaw m_display_buffer;

	//public:
	//	static void Initialize()
	//	{
	//		auto _ad = AffinityData{ std::this_thread::get_id(), std::this_thread::get_id() };

	//		m_caps_lock = 0x01 & GetKeyState(VK_CAPITAL);
	//		m_input = Services::MakeService<InputSystem>(_ad); // Happens during static init, so this never hits the stdout. Future versions should hit the log file...
	//		m_input_buffer = m_input->GetInputBuffer();
	//		m_scrollback_buffer = m_input->GetRCBuffer();
	//		m_display_buffer = m_input->GetDisplayBuffer();
	//	}

	//	static void OnKeyDown(WPARAM wParam, LPARAM lParam)
	//	{
	//		SHORT lshift = 0;
	//		SHORT rshift = 0;
	//		SHORT lcontrol = 0;
	//		SHORT rcontrol = 0;
	//		SHORT lmenu = 0;
	//		SHORT rmenu = 0;

	//		switch (wParam)
	//		{
	//		case VK_CAPITAL:
	//			m_caps_lock = 0x01 & GetKeyState(VK_CAPITAL);
	//			break;
	//		case VK_SHIFT:
	//			lshift = GetKeyState(VK_LSHIFT);
	//			rshift = GetKeyState(VK_RSHIFT);
	//			if (0x8000 & lshift) m_caps_lock = 0x01 & lshift;
	//			if (0x8000 & rshift) m_caps_lock = 0x01 & rshift;
	//			m_shift_pressed = (0x8000 & lshift) | (0x8000 & rshift);
	//			break;
	//		case VK_CONTROL:
	//			lcontrol = GetKeyState(VK_LCONTROL);
	//			rcontrol = GetKeyState(VK_RCONTROL);
	//			m_control_pressed = (0x8000 & lcontrol) | (0x8000 & rcontrol);
	//			break;
	//		case VK_MENU: // Alt
	//			lmenu = GetKeyState(VK_LMENU);
	//			rmenu = GetKeyState(VK_RMENU);
	//			m_alt_pressed = (0x8000 & lmenu) | (0x8000 & rmenu);
	//			break;
	//		case VK_INSERT: // Process the INS key
	//			m_input_buffer->ToggleOverwrite(); // lParam contains some state data we could use here
	//			break;

	//		case VK_DELETE: // Process the DEL key
	//			m_input_buffer->RemoveNext();
	//			break;
	//		case VK_HOME: // Process the HOME key
	//			if (m_control_pressed)
	//			{
	//				m_scrollback_buffer->MoveCarat(-(c_max_scrollback_length));
	//			}
	//			else
	//			{
	//				m_input_buffer->MoveCarat(-(c_max_stringbuffer_length));
	//			}
	//			break;
	//		case VK_END: // Process the END key
	//			if (m_control_pressed)
	//			{
	//				m_scrollback_buffer->MoveCarat(c_max_scrollback_length);
	//			}
	//			else
	//			{
	//				m_input_buffer->MoveCarat(c_max_stringbuffer_length);
	//			}
	//			break;
	//		case VK_UP: // Process the UP ARROW key
	//			m_scrollback_buffer->MoveCarat(-1);
	//			m_input_buffer->SetString(m_scrollback_buffer->GetString());
	//			break;
	//		case VK_DOWN: // Process the DOWN ARROW key
	//			m_scrollback_buffer->MoveCarat(1);
	//			m_input_buffer->SetString(m_scrollback_buffer->GetString());
	//			break;
	//		case VK_LEFT: // Process the LEFT ARROW key
	//			//if (m_control_pressed) {} //TODO: Move to next word or symbol
	//			m_input_buffer->MoveCarat(-1);
	//			break;
	//		case VK_RIGHT: // Process the RIGHT ARROW key
	//			//if (m_control_pressed) {} //TODO: Move to beginning of current or previous word or symbol
	//			m_input_buffer->MoveCarat(1);
	//			break;
	//		case VK_F1: // Process the F1 key
	//		case VK_NUMLOCK: // Toggle run default for many...
	//		case VK_SNAPSHOT: // Screenshot tool...
	//		case VK_SCROLL: // Toggle behavior of chat history versus mousewheel scrolling
	//		case VK_PAUSE: // Used for miscellaneous characters; it can vary by keyboard
	//		case VK_OEM_1: // For the US standard keyboard, the ';:' key
	//		case VK_OEM_2: // For the US standard keyboard, the '/?' key
	//		case VK_OEM_3: // For the US standard keyboard, the '`~' key
	//			//TODO: Toggle Console
	//		case VK_OEM_4: // For the US standard keyboard, the '[{' key
	//		case VK_OEM_5: // For the US standard keyboard, the '\|' key
	//		case VK_OEM_6: // For the US standard keyboard, the ']}' key
	//		case VK_OEM_7: // For the US standard keyboard, the ''"' key
	//		default:
	//			break;
	//		}
	//	}

	//	static void OnKeyUp(WPARAM wParam, LPARAM lParam)
	//	{
	//		SHORT lshift = 0;
	//		SHORT rshift = 0;

	//		switch (wParam)
	//		{
	//		case VK_SHIFT:
	//			lshift = GetKeyState(VK_LSHIFT);
	//			rshift = GetKeyState(VK_RSHIFT);
	//			m_shift_pressed = ((0x8000 & lshift) | (0x8000 & rshift));
	//			break;
	//		case VK_CONTROL:
	//			m_control_pressed = 0x8000 & GetKeyState(VK_CONTROL);
	//			break;
	//		case VK_MENU:
	//			m_alt_pressed = 0x8000 & GetKeyState(VK_MENU);
	//			break;
	//		}
	//	}

	//	static void OnChar(WPARAM wParam, LPARAM lParam)
	//	{
	//		switch (wParam)
	//		{
	//		case 0x08: // Process a backspace
	//			m_input_buffer->RemovePrev();
	//			break;
	//		case 0x1B: // Process an escape
	//			PostQuitMessage(0);
	//			break;
	//		case 0x09: // Process a tab (Is there a shift-tab or do I have to check shift?)
	//			break;
	//		case 0x0A: // Process a Shift-Enter
	//		case 0x0D: // Process an Enter
	//			m_scrollback_buffer->InsertString(m_input_buffer->GetString());
	//			m_display_buffer->InsertString(m_input_buffer->GetString());
	//			m_input_buffer->Clear();
	//			break;
	//		default: // Process displayable characters
	//			m_input_buffer->InsertChar(static_cast<wchar_t>(wParam));
	//			break;
	//		}
	//	}
	//};
}