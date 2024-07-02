#include "pch.h"
#include "InputSystem.h"

#include "WindowSystem.h"

using namespace DirectX;
using namespace ClayEngine;

InputSystem::InputSystem()
{
	m_input_buffer = std::make_unique<InputBuffer>();
	m_scrollback_buffer = std::make_unique<ScrollbackBuffer>();
	m_display_buffer = std::make_unique<DisplayBuffer>();

	m_caps_lock = 0x01 & GetKeyState(VK_CAPITAL);

	m_mouse = std::make_unique<Mouse>();
	m_mouse->SetWindow(Services::GetService<WindowSystem>(std::this_thread::get_id())->GetWindowHandle());
}

InputSystem::~InputSystem()
{
	m_mouse.reset();
	m_mouse = nullptr;

	m_display_buffer.reset();
	m_display_buffer = nullptr;

	m_scrollback_buffer.reset();
	m_scrollback_buffer = nullptr;

	m_input_buffer.reset();
	m_input_buffer = nullptr;
}

void InputSystem::OnMouseEvent(UINT message, WPARAM wParam, LPARAM lParam)
{
	//TODO: Currently deferred to DXTK Mouse class, replacing mouse functionality would be done here...
}

Mouse::State InputSystem::GetMouseState()
{
	return m_mouse->GetState();
}

Tracker& InputSystem::GetButtonStateTracker()
{
	return m_tracker;
}

void InputSystem::OnKeyDown(WPARAM wParam, LPARAM lParam)
{
	SHORT lshift = 0;
	SHORT rshift = 0;
	SHORT lcontrol = 0;
	SHORT rcontrol = 0;
	SHORT lmenu = 0;
	SHORT rmenu = 0;

	switch (wParam)
	{
	case VK_CAPITAL:
		m_caps_lock = 0x01 & GetKeyState(VK_CAPITAL);
		break;
	case VK_SHIFT:
		lshift = GetKeyState(VK_LSHIFT);
		rshift = GetKeyState(VK_RSHIFT);

		if (0x8000 & lshift) m_caps_lock = 0x01 & lshift;
		if (0x8000 & rshift) m_caps_lock = 0x01 & rshift;

		m_shift_pressed = (0x8000 & lshift) | (0x8000 & rshift);
		break;
	case VK_CONTROL:
		lcontrol = GetKeyState(VK_LCONTROL);
		rcontrol = GetKeyState(VK_RCONTROL);

		m_control_pressed = (0x8000 & lcontrol) | (0x8000 & rcontrol);
		break;
	case VK_MENU: // Alt
		lmenu = GetKeyState(VK_LMENU);
		rmenu = GetKeyState(VK_RMENU);

		m_alt_pressed = (0x8000 & lmenu) | (0x8000 & rmenu);
		break;
	case VK_INSERT: // Process the INS key
		m_input_buffer->ToggleOverwrite(); // lParam contains some state data we could use here
		break;
	case VK_DELETE: // Process the DEL key
		m_input_buffer->RemoveNext();
		break;
	case VK_HOME: // Process the HOME key
		if (m_control_pressed)
		{
			m_scrollback_buffer->MoveCarat(-(c_max_scrollback_length));
		}
		else
		{
			m_input_buffer->MoveCarat(-(c_max_stringbuffer_length));
		}
		break;
	case VK_END: // Process the END key
		if (m_control_pressed)
		{
			m_scrollback_buffer->MoveCarat(c_max_scrollback_length);
		}
		else
		{
			m_input_buffer->MoveCarat(c_max_stringbuffer_length);
		}
		break;
	case VK_UP: // Process the UP ARROW key
		m_scrollback_buffer->MoveCarat(-1);
		m_input_buffer->SetString(m_scrollback_buffer->GetString());
		break;
	case VK_DOWN: // Process the DOWN ARROW key
		m_scrollback_buffer->MoveCarat(1);
		m_input_buffer->SetString(m_scrollback_buffer->GetString());
		break;
	case VK_LEFT: // Process the LEFT ARROW key
		//if (m_control_pressed) {} //TODO: Move to next word or symbol
		m_input_buffer->MoveCarat(-1);
		break;
	case VK_RIGHT: // Process the RIGHT ARROW key
		//if (m_control_pressed) {} //TODO: Move to beginning of current or previous word or symbol
		m_input_buffer->MoveCarat(1);
		break;
	case VK_F1: // Process the F1 key
	case VK_NUMLOCK: // Toggle run default for many...
	case VK_SNAPSHOT: // Screenshot tool...
	case VK_SCROLL: // Toggle behavior of chat history versus mousewheel scrolling
	case VK_PAUSE: // Used for miscellaneous characters; it can vary by keyboard
	case VK_OEM_1: // For the US standard keyboard, the ';:' key
	case VK_OEM_2: // For the US standard keyboard, the '/?' key
	case VK_OEM_3: // For the US standard keyboard, the '`~' key
		//TODO: Toggle Console
	case VK_OEM_4: // For the US standard keyboard, the '[{' key
	case VK_OEM_5: // For the US standard keyboard, the '\|' key
	case VK_OEM_6: // For the US standard keyboard, the ']}' key
	case VK_OEM_7: // For the US standard keyboard, the ''"' key
	default:
		break;
	}
} // Updates Keyboard state

void InputSystem::OnKeyUp(WPARAM wParam, LPARAM lParam)
{
	SHORT lshift = 0;
	SHORT rshift = 0;
	SHORT controls = 0;
	SHORT menus = 0;

	switch (wParam)
	{
	case VK_SHIFT:
		lshift = GetKeyState(VK_LSHIFT);
		rshift = GetKeyState(VK_RSHIFT);

		m_shift_pressed = ((0x8000 & lshift) | (0x8000 & rshift));
		break;
	case VK_CONTROL:
		m_control_pressed = 0x8000 & GetKeyState(VK_CONTROL);
		break;
	case VK_MENU:
		m_alt_pressed = 0x8000 & GetKeyState(VK_MENU);
		break;
	default:
		break;
	}
}

void InputSystem::OnChar(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
	case 0x08: // Process a backspace
		m_input_buffer->RemovePrev();
		break;
	case 0x1B: // Process an escape
		PostQuitMessage(0);
		break;
	case 0x09:
		// Process a tab
		break;
	case 0x0A: // Process a linefeed. (Shift-Enter)
	case 0x0D: // Process a carriage return
		m_scrollback_buffer->InsertString(m_input_buffer->GetString());
		m_display_buffer->InsertString(m_input_buffer->GetString());
		m_input_buffer->Clear();
		break;
	default: // Process displayable characters
		m_input_buffer->InsertChar(static_cast<wchar_t>(wParam));
		break;
	}
} // Processes character input (dependent on input state)

Unicode InputSystem::GetBuffer()
{
	return m_input_buffer->GetString();
}
