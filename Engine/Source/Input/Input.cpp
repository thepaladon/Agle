#include "Input/Input.h"

#include "Engine.h"
#include "Log.h"
#include "Window.h"

#include <algorithm>
#include <ImGui/imgui.h>

using namespace Ball;
Input::Input()
{
	INFO(LOG_INPUT, "Input system got Created...");
}
Input::~Input()
{
	// Before we shutdown, stop all vibration
	if (IsControllerConnected())
		SetControllerVibration(controllerMotor::MOTORBOTH, 0);

	INFO(LOG_INPUT, "Input system got destroyed...");
}
void Input::Init(unsigned short playerIndex)
{
	SetController(playerIndex);

	m_AxisValues = {
		{LEFTX, 0}, {LEFTY, 0}, {RIGHTX, 0}, {RIGHTY, 0}, {L2, 0}, {R2, 0}, {SCROLL_WHEEL, 0}, {SCROLL_WHEEL_H, 0}};

	// KeyState map is initialized here, so that we don't have to validate if the key exist at runtime.
	m_KeyStates = {
		{KeyCode::MOUSE_L, KeyState::NONE},
		{KeyCode::MOUSE_R, KeyState::NONE},
		{KeyCode::MOUSE_MIDDLE, KeyState::NONE},
		{KeyCode::KEY_BACKSPACE, KeyState::NONE},
		{KeyCode::KEY_TAB, KeyState::NONE},
		{KeyCode::KEY_CLEAR, KeyState::NONE},
		{KeyCode::KEY_ENTER, KeyState::NONE},
		{KeyCode::KEY_SHIFT, KeyState::NONE},
		{KeyCode::KEY_CONTROL, KeyState::NONE},
		{KeyCode::KEY_ALT, KeyState::NONE},
		{KeyCode::KEY_PAUSE, KeyState::NONE},
		{KeyCode::KEY_CAPSLOCK, KeyState::NONE},
		{KeyCode::KEY_ESCAPE, KeyState::NONE},
		{KeyCode::KEY_0, KeyState::NONE},
		{KeyCode::KEY_1, KeyState::NONE},
		{KeyCode::KEY_2, KeyState::NONE},
		{KeyCode::KEY_3, KeyState::NONE},
		{KeyCode::KEY_4, KeyState::NONE},
		{KeyCode::KEY_5, KeyState::NONE},
		{KeyCode::KEY_6, KeyState::NONE},
		{KeyCode::KEY_7, KeyState::NONE},
		{KeyCode::KEY_8, KeyState::NONE},
		{KeyCode::KEY_9, KeyState::NONE},
		{KeyCode::KEY_SPACE, KeyState::NONE},
		{KeyCode::KEY_PAGEUP, KeyState::NONE},
		{KeyCode::KEY_PAGEDOWN, KeyState::NONE},
		{KeyCode::KEY_END, KeyState::NONE},
		{KeyCode::KEY_HOME, KeyState::NONE},
		{KeyCode::KEY_LEFT, KeyState::NONE},
		{KeyCode::KEY_UP, KeyState::NONE},
		{KeyCode::KEY_RIGHT, KeyState::NONE},
		{KeyCode::KEY_DOWN, KeyState::NONE},
		{KeyCode::KEY_SELECT, KeyState::NONE},
		{KeyCode::KEY_PRINT, KeyState::NONE},
		{KeyCode::KEY_PRINTSCREEN, KeyState::NONE},
		{KeyCode::KEY_INSERT, KeyState::NONE},
		{KeyCode::KEY_DELETE, KeyState::NONE},
		{KeyCode::KEY_HELP, KeyState::NONE},
		{KeyCode::KEY_ZERO, KeyState::NONE},
		{KeyCode::KEY_ONE, KeyState::NONE},
		{KeyCode::KEY_TWO, KeyState::NONE},
		{KeyCode::KEY_THREE, KeyState::NONE},
		{KeyCode::KEY_FOUR, KeyState::NONE},
		{KeyCode::KEY_FIVE, KeyState::NONE},
		{KeyCode::KEY_SIX, KeyState::NONE},
		{KeyCode::KEY_SEVEN, KeyState::NONE},
		{KeyCode::KEY_EIGHT, KeyState::NONE},
		{KeyCode::KEY_NINE, KeyState::NONE},
		{KeyCode::KEY_A, KeyState::NONE},
		{KeyCode::KEY_B, KeyState::NONE},
		{KeyCode::KEY_C, KeyState::NONE},
		{KeyCode::KEY_D, KeyState::NONE},
		{KeyCode::KEY_E, KeyState::NONE},
		{KeyCode::KEY_F, KeyState::NONE},
		{KeyCode::KEY_G, KeyState::NONE},
		{KeyCode::KEY_H, KeyState::NONE},
		{KeyCode::KEY_I, KeyState::NONE},
		{KeyCode::KEY_J, KeyState::NONE},
		{KeyCode::KEY_K, KeyState::NONE},
		{KeyCode::KEY_L, KeyState::NONE},
		{KeyCode::KEY_M, KeyState::NONE},
		{KeyCode::KEY_N, KeyState::NONE},
		{KeyCode::KEY_O, KeyState::NONE},
		{KeyCode::KEY_P, KeyState::NONE},
		{KeyCode::KEY_Q, KeyState::NONE},
		{KeyCode::KEY_R, KeyState::NONE},
		{KeyCode::KEY_S, KeyState::NONE},
		{KeyCode::KEY_T, KeyState::NONE},
		{KeyCode::KEY_U, KeyState::NONE},
		{KeyCode::KEY_V, KeyState::NONE},
		{KeyCode::KEY_W, KeyState::NONE},
		{KeyCode::KEY_X, KeyState::NONE},
		{KeyCode::KEY_Y, KeyState::NONE},
		{KeyCode::KEY_Z, KeyState::NONE},
		{KeyCode::KEY_LEFTWINDOWSKEY, KeyState::NONE},
		{KeyCode::KEY_RIGHTWINDOWSKEY, KeyState::NONE},
		{KeyCode::KEY_APPLICATIONSKEY, KeyState::NONE},
		{KeyCode::KEY_SLEEP, KeyState::NONE},
		{KeyCode::KEY_NUMPAD0, KeyState::NONE},
		{KeyCode::KEY_NUMPAD1, KeyState::NONE},
		{KeyCode::KEY_NUMPAD2, KeyState::NONE},
		{KeyCode::KEY_NUMPAD3, KeyState::NONE},
		{KeyCode::KEY_NUMPAD4, KeyState::NONE},
		{KeyCode::KEY_NUMPAD5, KeyState::NONE},
		{KeyCode::KEY_NUMPAD6, KeyState::NONE},
		{KeyCode::KEY_NUMPAD7, KeyState::NONE},
		{KeyCode::KEY_NUMPAD8, KeyState::NONE},
		{KeyCode::KEY_NUMPAD9, KeyState::NONE},
		{KeyCode::KEY_MULTIPLY, KeyState::NONE},
		{KeyCode::KEY_ADD, KeyState::NONE},
		{KeyCode::KEY_SEPERATOR, KeyState::NONE},
		{KeyCode::KEY_SUBTRACT, KeyState::NONE},
		{KeyCode::KEY_DECIMAL, KeyState::NONE},
		{KeyCode::KEY_DIVIDE, KeyState::NONE},
		{KeyCode::KEY_F1, KeyState::NONE},
		{KeyCode::KEY_F2, KeyState::NONE},
		{KeyCode::KEY_F3, KeyState::NONE},
		{KeyCode::KEY_F4, KeyState::NONE},
		{KeyCode::KEY_F5, KeyState::NONE},
		{KeyCode::KEY_F6, KeyState::NONE},
		{KeyCode::KEY_F7, KeyState::NONE},
		{KeyCode::KEY_F8, KeyState::NONE},
		{KeyCode::KEY_F9, KeyState::NONE},
		{KeyCode::KEY_F10, KeyState::NONE},
		{KeyCode::KEY_F11, KeyState::NONE},
		{KeyCode::KEY_F12, KeyState::NONE},
		{KeyCode::KEY_NUMLOCK, KeyState::NONE},
		{KeyCode::KEY_SCROLLLOCK, KeyState::NONE},
		{KeyCode::KEY_LEFTSHIFT, KeyState::NONE},
		{KeyCode::KEY_RIGHTSHIFT, KeyState::NONE},
		{KeyCode::KEY_LEFTCONTROL, KeyState::NONE},
		{KeyCode::KEY_RIGHTCONTOL, KeyState::NONE},
		{KeyCode::KEY_LEFTMENU, KeyState::NONE},
		{KeyCode::KEY_RIGHTMENU, KeyState::NONE},
		{KeyCode::KEY_BROWSERBACK, KeyState::NONE},
		{KeyCode::KEY_BROWSERFORWARD, KeyState::NONE},
		{KeyCode::KEY_BROWSERREFRESH, KeyState::NONE},
		{KeyCode::KEY_BROWSERSTOP, KeyState::NONE},
		{KeyCode::KEY_BROWSERSEARCH, KeyState::NONE},
		{KeyCode::KEY_BROWSERFAVORITES, KeyState::NONE},
		{KeyCode::KEY_BROWSERHOME, KeyState::NONE},
		{KeyCode::KEY_VOLUMEMUTE, KeyState::NONE},
		{KeyCode::KEY_VOLUMEDOWN, KeyState::NONE},
		{KeyCode::KEY_VOLUMEUP, KeyState::NONE},
		{KeyCode::KEY_NEXTTRACK, KeyState::NONE},
		{KeyCode::KEY_PREVIOUSTRACK, KeyState::NONE},
		{KeyCode::KEY_STOPMEDIA, KeyState::NONE},
		{KeyCode::KEY_PLAYPAUSE, KeyState::NONE},
		{KeyCode::GAMEPAD_DPAD_UP, KeyState::NONE},
		{KeyCode::GAMEPAD_DPAD_DOWN, KeyState::NONE},
		{KeyCode::GAMEPAD_DPAD_LEFT, KeyState::NONE},
		{KeyCode::GAMEPAD_DPAD_RIGHT, KeyState::NONE},
		{KeyCode::GAMEPAD_START, KeyState::NONE},
		{KeyCode::GAMEPAD_BACK, KeyState::NONE},
		{KeyCode::GAMEPAD_L3, KeyState::NONE},
		{KeyCode::GAMEPAD_R3, KeyState::NONE},
		{KeyCode::GAMEPAD_L1, KeyState::NONE},
		{KeyCode::GAMEPAD_R1, KeyState::NONE},
		{KeyCode::GAMEPAD_A, KeyState::NONE},
		{KeyCode::GAMEPAD_B, KeyState::NONE},
		{KeyCode::GAMEPAD_X, KeyState::NONE},
		{KeyCode::GAMEPAD_Y, KeyState::NONE},
		{KeyCode::GAMEPAD_L2, KeyState::NONE},
		{KeyCode::GAMEPAD_R2, KeyState::NONE},
	};
}

void Input::Update()
{
	m_UpdatedKeys.clear();
	// Get new frame keys
	GatherInputUpdate();
}

Action::Action(const std::string& n)
{
	for (size_t i = 0; i < m_KeyCount; i++)
	{
		m_Keys[i] = KeyCode::NONE;
	}
}

Axis::Axis(const std::string& n)
{
	m_Name = n;
}

Action& Action::AddKeyBind(KeyCode key)
{
	m_Keys[m_BoundKeyCount] = key;
	m_BoundKeyCount++;
	if (m_BoundKeyCount > m_KeyCount - 1)
	{
		m_BoundKeyCount -= m_KeyCount;
		m_BoundKeyCount--;
	}
	return *this;
}

Action& Action::RemoveKeyBind(KeyCode key)
{
	auto it = std::remove(m_Keys, m_Keys + m_KeyCount, key);
	m_BoundKeyCount = static_cast<int>(it - m_Keys);

	// Ensure m_BoundKeyCount is within valid range (0 to m_KeyCount - 1)
	if (m_BoundKeyCount < 0)
		m_BoundKeyCount = m_KeyCount - 1;

	return *this;
}

void Input::UpdateKeyState(bool KeyDown, KeyCode keyCode)
{
	auto& key = m_KeyStates[keyCode];

	// Cheap early out
	// No state change has happend
	if ((KeyDown && key == KeyState::DOWN) || (!KeyDown && key == KeyState::NONE))
		return;

	if (KeyDown)
	{
		switch (key)
		{
		case KeyState::RELEASED:
		case KeyState::NONE:
			m_KeyStates[keyCode] = KeyState::PRESSED;
			break;

		case KeyState::PRESSED:
		case KeyState::DOWN:
			m_KeyStates[keyCode] = KeyState::DOWN;
			break;
		}
	}
	else
	{
		switch (key)
		{
		case KeyState::PRESSED:
		case KeyState::DOWN:
			m_KeyStates[keyCode] = KeyState::RELEASED;
			break;

		case KeyState::RELEASED:
		case KeyState::NONE:
			m_KeyStates[keyCode] = KeyState::NONE;
			break;
		}
	}

	m_UpdatedKeys.insert({keyCode, m_KeyStates[keyCode]});
}

bool Input::IsControllerKey(KeyCode key)
{
	// Every key, starting from GAMEPAD_DPAD_UP, is a controller button.
	// Because of that, we can check if the value of "key" is higher or equal to GAMEPAD_DPAD_UP to check if it's a
	// controller button.
	return key >= GAMEPAD_DPAD_UP;
}
bool Input::IsMouseKey(KeyCode key)
{
	return key == KeyCode::MOUSE_L || key == KeyCode::MOUSE_MIDDLE || key == KeyCode::MOUSE_R;
}

KeyState Input::GetActionRaw(const std::string& name) const
{
	KeyState bestState = KeyState::NONE;

	assert(m_Actions.find(name) != m_Actions.end());

	const auto& action = m_Actions.find(name)->second;

	for (int i = 0; i < action.m_KeyCount; i++)
	{
		KeyCode key = action.m_Keys[i];
		if (key == KeyCode::NONE)
			continue; // TODO: I think we can break here.

		KeyState keyState = GetInput().GetRawKeyState(key);

		if (keyState != KeyState::NONE)
		{
			if (bestState == KeyState::RELEASED && keyState == KeyState::PRESSED)
				continue;

			bestState = keyState;

			if (keyState == KeyState::DOWN)
				break;
		}
	}

	return bestState;
}

bool Input::GetAction(const std::string& name) const
{
	assert(m_Actions.find(name) != m_Actions.end());

	if (!GetWindow().IsActive())
		return false;

#ifndef NO_IMGUI
	if (ImGui::IsAnyItemActive())
		return false;
#endif

	const auto& action = m_Actions.find(name)->second;

	for (int i = 0; i < action.m_KeyCount; i++)
	{
		KeyCode key = action.m_Keys[i];
		if (key == KeyCode::NONE)
			continue; // TODO: I think we can break here.

		KeyState keyState = GetInput().GetRawKeyState(key);

		if (keyState == KeyState::PRESSED || keyState == KeyState::DOWN)
			return true;
	}

	return false;
}

bool Input::GetActionDown(const std::string& name) const
{
	assert(m_Actions.find(name) != m_Actions.end());

	if (!GetWindow().IsActive())
		return false;

#ifndef NO_IMGUI
	if (ImGui::IsAnyItemActive())
		return false;
#endif

	const auto& action = m_Actions.find(name)->second;

	for (int i = 0; i < action.m_KeyCount; i++)
	{
		KeyCode key = action.m_Keys[i];
		if (key == KeyCode::NONE)
			continue; // TODO: I think we can break here.

		KeyState keyState = GetInput().GetRawKeyState(key);

		if (keyState == KeyState::PRESSED)
			return true;
	}

	return false;
}

bool Input::GetActionReleased(const std::string& name) const
{
	assert(m_Actions.find(name) != m_Actions.end());

	if (!GetWindow().IsActive())
		return false;

#ifndef NO_IMGUI
	if (ImGui::IsAnyItemActive())
		return false;
#endif

	const auto& action = m_Actions.find(name)->second;

	for (int i = 0; i < action.m_KeyCount; i++)
	{
		KeyCode key = action.m_Keys[i];
		if (key == KeyCode::NONE)
			continue; // TODO: I think we can break here.

		KeyState keyState = GetInput().GetRawKeyState(key);

		if (keyState == KeyState::RELEASED)
			return true;
	}

	return false;
}

float Input::GetAxis(const std::string& name) const
{
	assert(m_Axes.find(name) != m_Axes.end());
	auto& axis = m_Axes.find(name)->second;

	if (!GetWindow().IsActive())
		return 0;

#ifndef NO_IMGUI
	if (ImGui::IsAnyItemActive())
		return 0;
#endif

	float m_Value = 0;
	for (int i = 0; i < 4; i += 2)
	{
		if (axis.m_Keys[i] == KeyCode::NONE)
			break;

		auto keystate = GetInput().GetRawKeyState(axis.m_Keys[i]);
		bool lKey = keystate == KeyState::PRESSED || keystate == KeyState::DOWN;

		keystate = GetInput().GetRawKeyState(axis.m_Keys[i + 1]);
		bool rKey = keystate == KeyState::PRESSED || keystate == KeyState::DOWN;

		m_Value = (rKey - lKey);
		if (m_Value != 0)
		{
			return m_Value * axis.GetMagnitude();
		}
	}

	m_Value = GetInput().m_AxisValues[axis.m_JoyStick];

	if (abs(m_Value) < axis.GetDeadZone())
	{
		m_Value = 0;
	}

	return m_Value * axis.GetMagnitude();
}

Axis& Input::CreateAxis(const std::string& name)
{
	// Axis already exists
	assert(m_Axes.find(name) == m_Axes.end());

	auto inserted = m_Axes.emplace(name, Axis(name));
	return inserted.first->second;
}

bool Input::ContainsAxis(const std::string& name) const
{
	return m_Axes.find(name) != m_Axes.end();
}
Axis& Input::GetAxisBinding(const std::string& name)
{
	ASSERT_MSG(LOG_INPUT, !ContainsAxis(name), "Failed to find axis called %s", name.c_str());

	return m_Axes.find(name)->second;
}

bool Input::GetAnyDown() const
{
	return !m_UpdatedKeys.empty();
}

bool Input::ContainsAction(const std::string& name) const
{
	return m_Actions.find(name) != m_Actions.end();
}

Action& Input::GetActionBinding(const std::string& name)
{
	ASSERT_MSG(LOG_INPUT, !ContainsAction(name), "Failed to find axis called %s", name.c_str());

	return m_Actions.find(name)->second;
}

const KeyState& Input::GetRawKeyState(KeyCode key) const
{
	return m_KeyStates.at(key);
}
float Input::GetJoystickRaw(JoyStick joyStick) const
{
	return m_AxisValues.at(joyStick);
}

const std::unordered_map<KeyCode, KeyState>& Input::GetUpdatedKeys() const
{
	return m_UpdatedKeys;
}

Axis& Axis::AddKeyBind(KeyCode minKey, KeyCode maxKey)
{
	assert(m_Keys[2] == KeyCode::NONE); // Out of key slots

	int i = m_Keys[0] == KeyCode::NONE ? 0 : 2;

	m_Keys[i] = minKey;
	m_Keys[i + 1] = maxKey;

	return *this;
}

Axis& Axis::RemoveKeyBind(KeyCode minKey, KeyCode maxKey)
{
	assert(m_Keys[2] == KeyCode::NONE); // Out of key slots

	int i = m_Keys[0] == KeyCode::NONE ? 0 : 2;

	auto it = std::remove(m_Keys + i, m_Keys + i + 2, minKey);

	// If minKey was found, remove maxKey as well
	if (it != m_Keys + i + 2)
	{
		it = std::remove(it, m_Keys + i + 2, maxKey);
	}

	return *this;
}

Axis& Axis::AddStickBind(JoyStick stick)
{
	assert(m_JoyStick == JoyStick::JOYSTICK_NONE);
	m_JoyStick = stick;
	return *this;
}

Axis& Axis::Magnitude(float mag)
{
	m_Magnitude = mag;
	return *this;
}

Axis& Axis::DeadZone(float val)
{
	m_Deadzone = val;
	return *this;
}

float Axis::GetMagnitude() const
{
	return m_Magnitude;
}

float Axis::GetDeadZone() const
{
	return m_Deadzone;
}

Action& Input::CreateAction(const std::string& name)
{
	auto inserted = m_Actions.emplace(name, Action(name));

	return inserted.first->second;
}