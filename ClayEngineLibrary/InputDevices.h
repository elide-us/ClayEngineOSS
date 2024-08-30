#pragma once

#include <memory>
#include <vector>
#include <array>

#include "Services.h"
#include "WindowSystem.h"

namespace ClayEngine
{
	enum class ButtonState
	{
		STATE_UP = 0,
		STATE_PRESSED = 1,
		STATE_DOWN = 2,
		STATE_RELEASED = 3,
	};
	enum class PositionMode
	{
		MODE_ABSOLUTE = 0, // From top left
		MODE_RELATIVE = 1, // How much moved ince last frame
	};

	struct KeyboardState
	{
		std::array<ButtonState, 256> Keys = { ButtonState::STATE_UP };  // Assuming 256 possible virtual keys
	};

	struct MouseState
	{
		ButtonState LeftButton;
		ButtonState MiddleButton;
		ButtonState RightButton;
		ButtonState XButton1;
		ButtonState XButton2;
		PositionMode Mode = PositionMode::MODE_RELATIVE;
		int X; // Mouse position
		int Y; // Mouse position
		int W; // ScrollWheel amount
	};

	struct GamepadState
	{
		ButtonState DPadUp; // 0x0001
		ButtonState DPadDown; // 0x0002
		ButtonState DPadLeft; // 0x0004
		ButtonState DPadRight; // 0x0008

		ButtonState Start; // 0x0010
		ButtonState Back; // 0x0020

		ButtonState LStick; // 0x0040
		ButtonState RStick; // 0x0080

		ButtonState L1; // 0x0100
		ButtonState R1; // 0x0200
		ButtonState A; //
		ButtonState B; // 
		ButtonState X; // 
		ButtonState Y; // 
		ButtonState Guide;

		unsigned char L2;
		unsigned char R2;
		short LeftStickX;
		short LeftStickY;
		short RightStickX;
		short RightStickY;
	};


	class KeyboardHandler
	{
		KeyboardState m_current = {};
		KeyboardState m_last = {};

	public:
		KeyboardHandler() = default;
		~KeyboardHandler() = default;

		void RegisterKeyboardDevice(Affinity threadId)
		{
			auto hwnd = Services::GetService<WindowSystem>(threadId)->GetWindowHandle();

			RAWINPUTDEVICE rid;
			rid.usUsagePage = 0x01;          // HID_USAGE_PAGE_GENERIC
			rid.usUsage = 0x06;              // HID_USAGE_GENERIC_KEYBOARD
			rid.dwFlags = RIDEV_INPUTSINK;   // Get events even when not in focus
			rid.hwndTarget = hwnd;

			if (RegisterRawInputDevices(&rid, 1, sizeof(rid)) == FALSE)
			{
				// Handle registration failure
			}
		}

		void ProcessRawInput(LPARAM lParam)
		{

		};
	};

	class MouseHandler
	{
		MouseState m_current = { ButtonState::STATE_UP };
		MouseState m_last = { ButtonState::STATE_UP };

	public:
		MouseHandler() = default;
		~MouseHandler() = default;

		void RegisterMouseDevice(HWND hWnd)
		{
			RAWINPUTDEVICE rid;
			rid.usUsagePage = 0x01;          // HID_USAGE_PAGE_GENERIC
			rid.usUsage = 0x02;              // HID_USAGE_GENERIC_MOUSE
			rid.dwFlags = RIDEV_INPUTSINK;   // Get events even when not in focus
			rid.hwndTarget = hWnd;

			if (RegisterRawInputDevices(&rid, 1, sizeof(rid)) == FALSE)
			{
				// Handle registration failure
			}
		}

		void ProcessRawInput(LPARAM lParam)
		{
			// Get the size of the raw input data
			UINT dwSize;
			GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));

			// Allocate a vector to store the raw input data
			std::vector<BYTE> rawData(dwSize);

			// Get the raw input data
			if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, rawData.data(), &dwSize, sizeof(RAWINPUTHEADER)) != dwSize)
			{
				return; // Failed to retrieve the data
			}

			// Cast the buffer to RAWINPUT structure
			RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(rawData.data());

			// Ensure the raw input is from a mouse
			if (raw->header.dwType == RIM_TYPEMOUSE)
			{
				const RAWMOUSE& mouseData = raw->data.mouse;

				// Update mouse position based on mode
				if (m_current.Mode == PositionMode::MODE_RELATIVE)
				{
					m_current.X += mouseData.lLastX;
					m_current.Y += mouseData.lLastY;
				}
				else if (m_current.Mode == PositionMode::MODE_ABSOLUTE)
				{
					// Handling for absolute mode if needed
				}

				// Update button states
				if (mouseData.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN)
				{
					m_current.LeftButton = ButtonState::STATE_PRESSED;
				}
				if (mouseData.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP)
				{
					m_current.LeftButton = ButtonState::STATE_RELEASED;
				}

				if (mouseData.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN)
				{
					m_current.RightButton = ButtonState::STATE_PRESSED;
				}
				if (mouseData.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP)
				{
					m_current.RightButton = ButtonState::STATE_RELEASED;
				}

				if (mouseData.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN)
				{
					m_current.MiddleButton = ButtonState::STATE_PRESSED;
				}
				if (mouseData.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP)
				{
					m_current.MiddleButton = ButtonState::STATE_RELEASED;
				}

				if (mouseData.usButtonFlags & RI_MOUSE_BUTTON_4_DOWN)
				{
					m_current.XButton1 = ButtonState::STATE_PRESSED;
				}
				if (mouseData.usButtonFlags & RI_MOUSE_BUTTON_4_UP)
				{
					m_current.XButton1 = ButtonState::STATE_RELEASED;
				}

				if (mouseData.usButtonFlags & RI_MOUSE_BUTTON_5_DOWN)
				{
					m_current.XButton2 = ButtonState::STATE_PRESSED;
				}
				if (mouseData.usButtonFlags & RI_MOUSE_BUTTON_5_UP)
				{
					m_current.XButton2 = ButtonState::STATE_RELEASED;
				}

				// Update scroll wheel
				if (mouseData.usButtonFlags & RI_MOUSE_WHEEL)
				{
					m_current.W = static_cast<int>(static_cast<SHORT>(mouseData.usButtonData));
				}
			}
		};

		void Update(float elapsedTime)
		{
			if (m_current.LeftButton == ButtonState::STATE_PRESSED &&
				m_last.LeftButton == ButtonState::STATE_UP)
			{
				// OnButtonPressed()
			}
			if (m_current.LeftButton == ButtonState::STATE_PRESSED &&
				m_last.LeftButton == ButtonState::STATE_PRESSED)
			{
				m_current.LeftButton = ButtonState::STATE_DOWN;
			}
			if (m_current.LeftButton == ButtonState::STATE_RELEASED &&
				m_last.LeftButton == ButtonState::STATE_DOWN)
			{
				// OnButtonReleased()
			}
			if (m_current.LeftButton == ButtonState::STATE_RELEASED &&
				m_last.LeftButton == ButtonState::STATE_RELEASED)
			{
				m_current.LeftButton = ButtonState::STATE_UP;
			}

			if (m_current.RightButton == ButtonState::STATE_PRESSED &&
				m_last.RightButton == ButtonState::STATE_UP)
			{
				// OnButtonPressed()
			}
			if (m_current.RightButton == ButtonState::STATE_PRESSED &&
				m_last.RightButton == ButtonState::STATE_PRESSED)
			{
				m_current.RightButton = ButtonState::STATE_DOWN;
			}
			if (m_current.RightButton == ButtonState::STATE_RELEASED &&
				m_last.RightButton == ButtonState::STATE_DOWN)
			{
				// OnButtonReleased()
			}
			if (m_current.RightButton == ButtonState::STATE_RELEASED &&
				m_last.RightButton == ButtonState::STATE_RELEASED)
			{
				m_current.RightButton = ButtonState::STATE_UP;
			}
			
			if (m_current.MiddleButton == ButtonState::STATE_PRESSED &&
				m_last.MiddleButton == ButtonState::STATE_UP)
			{
				// OnButtonPressed()
			}
			if (m_current.MiddleButton == ButtonState::STATE_PRESSED &&
				m_last.MiddleButton == ButtonState::STATE_PRESSED)
			{
				m_current.MiddleButton = ButtonState::STATE_DOWN;
			}
			if (m_current.MiddleButton == ButtonState::STATE_RELEASED &&
				m_last.MiddleButton == ButtonState::STATE_DOWN)
			{
				// OnButtonReleased()
			}
			if (m_current.MiddleButton == ButtonState::STATE_RELEASED &&
				m_last.MiddleButton == ButtonState::STATE_RELEASED)
			{
				m_current.MiddleButton = ButtonState::STATE_UP;
			}

			if (m_current.XButton1 == ButtonState::STATE_PRESSED &&
				m_last.XButton1 == ButtonState::STATE_UP)
			{
				// OnButtonPressed()
			}
			if (m_current.XButton1 == ButtonState::STATE_PRESSED &&
				m_last.XButton1 == ButtonState::STATE_PRESSED)
			{
				m_current.XButton1 = ButtonState::STATE_DOWN;
			}
			if (m_current.XButton1 == ButtonState::STATE_RELEASED &&
				m_last.XButton1 == ButtonState::STATE_DOWN)
			{
				// OnButtonReleased()
			}
			if (m_current.XButton1 == ButtonState::STATE_RELEASED &&
				m_last.XButton1 == ButtonState::STATE_RELEASED)
			{
				m_current.XButton1 = ButtonState::STATE_UP;
			}

			if (m_current.XButton2 == ButtonState::STATE_PRESSED &&
				m_last.XButton2 == ButtonState::STATE_UP)
			{
				// OnButtonPressed()
			}
			if (m_current.XButton2 == ButtonState::STATE_PRESSED &&
				m_last.XButton2 == ButtonState::STATE_PRESSED)
			{
				m_current.XButton2 = ButtonState::STATE_DOWN;
			}
			if (m_current.XButton2 == ButtonState::STATE_RELEASED &&
				m_last.XButton2 == ButtonState::STATE_DOWN)
			{
				// OnButtonReleased()
			}
			if (m_current.XButton2 == ButtonState::STATE_RELEASED &&
				m_last.XButton2 == ButtonState::STATE_RELEASED)
			{
				m_current.XButton2 = ButtonState::STATE_UP;
			}

			// Update last state
			m_last = m_current;
			m_current.W = 0;
		}

		void OnMouseMessage(UINT message, WPARAM wParam, LPARAM lParam)
		{
			switch (message)
			{
			case WM_ACTIVATE:
			case WM_ACTIVATEAPP:
				break;
			case WM_INPUT:
				if (m_current.Mode == PositionMode::MODE_RELATIVE)
				{
					RAWINPUT _raw;
					UINT _raw_size = sizeof(_raw);

					auto _hr = GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &_raw, &_raw_size, sizeof(RAWINPUTHEADER));
					ThrowIfFailed(_hr);

					if (_raw.header.dwType == RIM_TYPEMOUSE)
					{
						if (!(_raw.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE))
						{
							m_current.X += _raw.data.mouse.lLastX;
							m_current.Y += _raw.data.mouse.lLastY;
						}
					}

					// Parse Header see Mouse.cpp line 1422
				}
			case WM_MOUSEMOVE:
			case WM_MOUSEHOVER:
				break;
			case WM_LBUTTONDOWN:
				m_current.LeftButton = ButtonState::STATE_PRESSED;
				break;
			case WM_LBUTTONUP:
				m_current.LeftButton = ButtonState::STATE_RELEASED;
				break;
			case WM_RBUTTONDOWN:
				m_current.RightButton = ButtonState::STATE_PRESSED;
				break;
			case WM_RBUTTONUP:
				m_current.RightButton = ButtonState::STATE_RELEASED;
				break;
			case WM_MBUTTONDOWN:
				m_current.MiddleButton = ButtonState::STATE_PRESSED;
				break;
			case WM_MBUTTONUP:
				m_current.MiddleButton = ButtonState::STATE_RELEASED;
				break;
			case WM_XBUTTONDOWN:
				switch (GET_XBUTTON_WPARAM(wParam))
				{
				case XBUTTON1:
					m_current.XButton1 = ButtonState::STATE_PRESSED;
					break;
				case XBUTTON2:
					m_current.XButton2 = ButtonState::STATE_RELEASED;
					break;
				default:
					break;
				}
				break;
			case WM_XBUTTONUP:
				switch (GET_XBUTTON_WPARAM(wParam))
				{
				case XBUTTON1:
					m_current.XButton1 = ButtonState::STATE_RELEASED;
					break;
				case XBUTTON2:
					m_current.XButton2 = ButtonState::STATE_RELEASED;
					break;
				default:
					break;
				}
				break;
			case WM_MOUSEWHEEL:
				break;
			case WAIT_TIMEOUT:
			default:
				break;
			}
		}

		const MouseState& GetState() const { return m_current; }
	};

	class GamepadHandler
	{
		GamepadState m_current = {};
		GamepadState m_last = {};

	public:
		GamepadHandler() = default;
		~GamepadHandler() = default;

		void RegisterGamepadDevice(HWND hWnd)
		{
			RAWINPUTDEVICE rid;
			rid.usUsagePage = 0x01;          // HID_USAGE_PAGE_GENERIC
			rid.usUsage = 0x05;              // HID_USAGE_GENERIC_GAMEPAD
			rid.dwFlags = RIDEV_INPUTSINK;   // Get events even when not in focus
			rid.hwndTarget = hWnd;

			if (RegisterRawInputDevices(&rid, 1, sizeof(rid)) == FALSE)
			{
				// Handle registration failure
			}
		}

		void ProcessRawInput(LPARAM lParam)
		{
			// Get the size of the raw input data
			UINT dwSize;
			GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));

			// Allocate a vector to store the raw input data
			std::vector<BYTE> rawData(dwSize);

			// Get the raw input data
			if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, rawData.data(), &dwSize, sizeof(RAWINPUTHEADER)) != dwSize)
			{
				return; // Failed to retrieve the data
			}

			// Cast the buffer to RAWINPUT structure
			RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(rawData.data());

			// Ensure the raw input is from a gamepad
			if (raw->header.dwType == RIM_TYPEHID)
			{
				USHORT buttons = *((USHORT*)&rawData[0]);
				m_current.DPadUp = (buttons & 0x01) ? ButtonState::STATE_DOWN : ButtonState::STATE_UP;
				m_current.DPadDown = (buttons & 0x02) ? ButtonState::STATE_DOWN : ButtonState::STATE_UP;
				m_current.DPadLeft = (buttons & 0x04) ? ButtonState::STATE_DOWN : ButtonState::STATE_UP;
				m_current.DPadRight = (buttons & 0x08) ? ButtonState::STATE_DOWN : ButtonState::STATE_UP;

				// Triggers
				//gamepadState.leftTrigger = rawData[2];
				//gamepadState.rightTrigger = rawData[3];

				// Thumbsticks
				//gamepadState.leftStickX = *((SHORT*)&rawData[4]);
				//gamepadState.leftStickY = *((SHORT*)&rawData[6]);
				//gamepadState.rightStickX = *((SHORT*)&rawData[8]);
				//gamepadState.rightStickY = *((SHORT*)&rawData[10]);

				m_last = m_current;
			}
		};
	};
}