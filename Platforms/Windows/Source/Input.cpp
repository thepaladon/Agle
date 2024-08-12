#include "Input/Input.h"
#include <stdio.h>
#include <Windows.h>
#include <Xinput.h>

#include "Engine.h"
#include "Log.h"
#include "Window.h"
#include "Utilities/MathUtilities.h"

using namespace Ball;
namespace Ball
{
	namespace InputInternal
	{
		const std::unordered_map<unsigned int, uint32_t> KeyCodemap{
			{GAMEPAD_START, XINPUT_GAMEPAD_START},
			{GAMEPAD_BACK, XINPUT_GAMEPAD_BACK},
			{GAMEPAD_DPAD_UP, XINPUT_GAMEPAD_DPAD_UP},
			{GAMEPAD_DPAD_RIGHT, XINPUT_GAMEPAD_DPAD_RIGHT},
			{GAMEPAD_DPAD_DOWN, XINPUT_GAMEPAD_DPAD_DOWN},
			{GAMEPAD_DPAD_LEFT, XINPUT_GAMEPAD_DPAD_LEFT},
			{GAMEPAD_L3, XINPUT_GAMEPAD_LEFT_THUMB},
			{GAMEPAD_R3, XINPUT_GAMEPAD_RIGHT_THUMB},
			// GAMEPAD_L2 && GAMEPAD_R2 are implemented a other way (They are axis keys...)
			{GAMEPAD_L1, XINPUT_GAMEPAD_LEFT_SHOULDER},
			{GAMEPAD_R1, XINPUT_GAMEPAD_RIGHT_SHOULDER},
			{GAMEPAD_Y, XINPUT_GAMEPAD_Y},
			{GAMEPAD_B, XINPUT_GAMEPAD_B},
			{GAMEPAD_A, XINPUT_GAMEPAD_A},
			{GAMEPAD_X, XINPUT_GAMEPAD_X}};

		static inline float scrollWheelValues[2] = {0, 0};

		bool isControllerConnected = false;

		/// <summary>
		/// List with keys that got pressed since last frame.
		/// </summary>
		std::set<KeyCode> M_KeysPressed;
		/// <summary>
		/// List with keys that got released since last frame.
		/// </summary>
		std::set<KeyCode> M_KeysReleased;

		HCURSOR defaultCursor;
		HCURSOR invisibleCursor;

		static inline std::unordered_map<KeyCode, bool> ProcKeyStates;
		static inline void UpdateKeyState(bool KeyDown, KeyCode keyCode)
		{
			ProcKeyStates.insert({keyCode, KeyDown});
		}
	} // namespace InputInternal
} // namespace Ball

static LONG_PTR OtherWndProc;
static void InputSystemWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void Input::SetController(unsigned short id)
{
	m_ControllerID = id;

	InputInternal::defaultCursor = LoadCursor(NULL, IDC_ARROW);
	BYTE ANDmaskCursor[] = {0x00};
	BYTE XORmaskCursor[] = {0x00};

	InputInternal::invisibleCursor = CreateCursor(NULL, 0, 0, 1, 1, ANDmaskCursor, XORmaskCursor);

	// TODO: have a platform specific init function, Doing this here is not ideal..
	OtherWndProc = SetWindowLongPtr(
		static_cast<HWND>(GetWindow().GetWindowHandle()), GWLP_WNDPROC, (LONG_PTR)InputSystemWindowProc);
}

bool Input::IsControllerConnected() const
{
	return InputInternal::isControllerConnected;
}

ControllerType Input::GetControllerType() const
{
	return ControllerType::Xbox;
}

// This is only part of gathering input, For windows events we use `InputSystemWindowProc`
void Input::GatherInputUpdate()
{
	// If the game is not the active window, we don't gather input.
	if (GetWindow().GetWindowHandle() != GetActiveWindow())
	{
		// Reset all key states and return early.
		for (auto& keyState : m_KeyStates)
			UpdateKeyState(false, keyState.first);

		return;
	}

	// Controller update
	// In do while to allow me to break
	do
	{
		static DWORD oldPacketNumber = 0;
		XINPUT_STATE state;
		const DWORD dwResult = XInputGetState(m_ControllerID, &state);
		InputInternal::isControllerConnected = dwResult != ERROR_DEVICE_NOT_CONNECTED;

		// If packet number is the same, no input update happend since last time.
		if (state.dwPacketNumber == oldPacketNumber || !InputInternal::isControllerConnected)
			break;

		oldPacketNumber = state.dwPacketNumber;

		for (unsigned int i = KeyCode::GAMEPAD_DPAD_UP; i <= GAMEPAD_Y; i++)
		{
			bool newKeyState = (bool)(state.Gamepad.wButtons & InputInternal::KeyCodemap.at((KeyCode)i));

			InputInternal::UpdateKeyState(newKeyState, (KeyCode)i);
		}

		// Also allow getting L2/R2
		InputInternal::UpdateKeyState(state.Gamepad.bRightTrigger > 0, KeyCode::GAMEPAD_R2);
		InputInternal::UpdateKeyState(state.Gamepad.bLeftTrigger > 0, KeyCode::GAMEPAD_L2);

		for (int i = 0; i < 4; ++i)
		{
			// Divide value by the range, Now its between -1,1
			float value = *((&state.Gamepad.sThumbLX) + i) / 32767.0f;

			m_AxisValues[(JoyStick)((int)JoyStick::LEFTX + i)] = value;
		}

		m_AxisValues[JoyStick::L2] = (float)state.Gamepad.bLeftTrigger / 255.f;
		m_AxisValues[JoyStick::R2] = (float)state.Gamepad.bRightTrigger / 255.f;

	} while (false);

	// Proccess inputkeymap
	{
		// Update old frame keys
		{
			for (int i = KeyCode::MOUSE_L; i <= KeyCode::KEY_PLAYPAUSE; i++)
			{
				KeyCode keyCode = static_cast<KeyCode>(i);
				const auto& keyLookup = m_KeyStates.find(keyCode);
				if (keyLookup == m_KeyStates.end())
					continue;

				int keyState = GetAsyncKeyState(static_cast<KeyCode>(keyCode));
				if ((keyState) && (keyLookup->second == KeyState::NONE || keyLookup->second == KeyState::PRESSED))
				{
					UpdateKeyState(true, keyCode);
				}
				else if (keyState == 0 && keyLookup->second != KeyState::NONE)
				{
					UpdateKeyState(false, keyCode);
				}
			}

			for (auto& key : InputInternal::M_KeysPressed)
			{
				UpdateKeyState(true, key);
			}
			for (auto& key : InputInternal::M_KeysReleased)
			{
				UpdateKeyState(false, key);
			}

			InputInternal::M_KeysPressed.clear();
			InputInternal::M_KeysReleased.clear();
		}

		for (const auto& keyState : InputInternal::ProcKeyStates)
		{
			// If key got released this frame, we want to update state to none next frame... platform has to handle
			// this. as for most we quary every frame. but not with win api
			if (keyState.second)
				InputInternal::M_KeysPressed.insert(keyState.first);
			else
				InputInternal::M_KeysReleased.insert(keyState.first);

			UpdateKeyState(keyState.second, keyState.first);
		}

		InputInternal::ProcKeyStates.clear();

		for (int i = 0; i < 2; ++i)
		{
			m_AxisValues[(JoyStick)((int)JoyStick::SCROLL_WHEEL + i)] = InputInternal::scrollWheelValues[i];
			InputInternal::scrollWheelValues[i] = 0;
		}

		if (m_CursorState != CursorState::NONE)
		{
			HWND winHandle = static_cast<HWND>(GetWindow().GetWindowHandle());
			RECT winRect = {};
			GetClientRect(winHandle, &winRect);
			ClientToScreen(winHandle, reinterpret_cast<POINT*>(&winRect.left));
			ClientToScreen(winHandle, reinterpret_cast<POINT*>(&winRect.right));

			glm::vec2 centerPos = glm::ivec2((winRect.right + winRect.left) / 2, (winRect.bottom + winRect.top) / 2);

			// Calculate mouse delta
			POINT mousePos;
			GetCursorPos(&mousePos);
			m_MouseDelta = glm::vec2(mousePos.x, mousePos.y) - centerPos;

			SetCursorPos(centerPos.x, centerPos.y);
		}
		else
		{
			m_MouseDelta = glm::vec2(0, 0);
		}
	}
}

void Input::SetControllerVibration(controllerMotor motor, float value)
{
	// controller vibration
	XINPUT_VIBRATION vib;

	vib.wLeftMotorSpeed = static_cast<WORD>(m_Vibrations[0] * 65535.f);
	vib.wRightMotorSpeed = static_cast<WORD>(m_Vibrations[1] * 65535.f);

	switch (motor)
	{
	case MOTORLOWFREQ:
		vib.wLeftMotorSpeed = static_cast<WORD>(value * 65535.f);
		break;

	case MOTORHIGHFREQ:
		vib.wRightMotorSpeed = static_cast<WORD>(value * 65535.f);
		break;

	case MOTORBOTH:
		vib.wLeftMotorSpeed = static_cast<WORD>(value * 65535.f);
		vib.wRightMotorSpeed = static_cast<WORD>(value * 65535.f);
		break;
	}

	m_Vibrations[0] = vib.wLeftMotorSpeed / 65535.f;
	m_Vibrations[1] = vib.wRightMotorSpeed / 65535.f;

	auto returnCode = XInputSetState(m_ControllerID, &vib); // TODO make this fetch correct controllerID
	if (returnCode != ERROR_SUCCESS)
	{
		// If no controlled is connected, just log a warning to console.
		// If some other error occurred, call assert.
		if (returnCode == ERROR_DEVICE_NOT_CONNECTED)
		{
			WARN(LOG_INPUT, "Attempted to set controller rumble, but no controller is connected");
		}
		else
		{
			LPSTR messageBuffer = nullptr;
			size_t size = FormatMessageA(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				returnCode,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPSTR)&messageBuffer,
				0,
				NULL);

			// Copy the error message into a std::string.
			std::string message(messageBuffer, size);

			// Free the Win32's string's buffer.
			LocalFree(messageBuffer);

			ASSERT_MSG(LOG_INPUT, returnCode == ERROR_SUCCESS, "Failed to set Controller rumble: %s", message.c_str());
		}
	}
}

glm::vec2 Input::GetMousePosition() const
{
	if (!GetWindow().IsActive())
		return {};

	POINT p;
	GetCursorPos(&p);

	if (ScreenToClient((HWND)GetWindow().GetWindowHandle(), &p))
	{
		return glm::vec2(p.x, p.y);
	}

	return {};
}

void Input::SetCursorState(CursorState newState)
{
	m_CursorState = newState;

	switch (m_CursorState)
	{
	case CursorState::NONE:
		{
			SetCursor(InputInternal::defaultCursor);
			ClipCursor(NULL);
			ShowCursor(true);

			break;
		}
	case CursorState::LOCKED:
	case CursorState::INVISIBLE:
		{
			RECT clientRect;
			GetClientRect(static_cast<HWND>(GetWindow().GetWindowHandle()), &clientRect);

			// Convert client-area coordinates to screen coordinates
			POINT topLeft = {clientRect.left + 1, clientRect.top + 1};
			POINT bottomRight = {clientRect.right - 1, clientRect.bottom - 1};
			ClientToScreen(static_cast<HWND>(GetWindow().GetWindowHandle()), &topLeft);
			ClientToScreen(static_cast<HWND>(GetWindow().GetWindowHandle()), &bottomRight);

			// Now create the final rect used by the ClipCursor function
			RECT clipRect = {topLeft.x, topLeft.y, bottomRight.x, bottomRight.y};

			// Clip the cursor to the client area
			ClipCursor(&clipRect);

			if (m_CursorState == CursorState::INVISIBLE)
			{
				SetFocus((HWND)GetWindow().GetWindowHandle());
				SetCursor(NULL);

				// A while loop is required here, as ShowCursor sets an internal display counter that determines whether
				// the cursor should be displayed. The cursor is displayed only if the display count is greater than or
				// equal to 0. Because of that, we need to loop on ShowCursor until the value is negative.
				while (ShowCursor(false) >= 0)
				{
				}
			}
			else
			{
				SetCursor(InputInternal::defaultCursor);
				while (ShowCursor(TRUE) < 0)
				{
				}
			}

			// Move the mouse to the center of the screen. This is done to prevent a large mouse delta value when hiding
			// or locking the mouse cursor.
			HWND winHandle = static_cast<HWND>(GetWindow().GetWindowHandle());
			RECT winRect = {};
			GetClientRect(winHandle, &winRect);
			ClientToScreen(winHandle, reinterpret_cast<POINT*>(&winRect.left));
			ClientToScreen(winHandle, reinterpret_cast<POINT*>(&winRect.right));

			glm::vec2 centerPos = glm::ivec2((winRect.right + winRect.left) / 2, (winRect.bottom + winRect.top) / 2);
			SetCursorPos(centerPos.x, centerPos.y);

			break;
		}
	}
}

static void InputSystemWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		// We only check for mouse wheel here, as that can only be tracked through window events.
		// Mouse button presses are checked in the GatherInputUpdate() function.
	case WM_MOUSEWHEEL:
		{
			InputInternal::scrollWheelValues[0] = (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
			break;
		}
	case WM_MOUSEHWHEEL:
		{
			InputInternal::scrollWheelValues[1] = (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
			break;
		}
	}

	CallWindowProc((WNDPROC)OtherWndProc, hWnd, msg, wParam, lParam);
}