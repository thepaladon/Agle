#pragma once
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <set>

#include "Input/KeyCodes.h"

namespace Ball
{
	enum class KeyState : unsigned char
	{
		PRESSED, // The key got pressed this frame
		NONE, // Key is not being pressed/held down
		// Note this order allows for >= Down or <= Down
		DOWN, // Key is being held down
		RELEASED // Key has been released this frame
	};

	enum class CursorState : unsigned char
	{
		/// <summary>
		/// Window does not restrict cursor
		/// </summary>
		NONE,
		/// <summary>
		/// Cursor is locked within the window (and is visible)
		/// </summary>
		LOCKED,
		/// <summary>
		/// Cursor is locked within the window (but is not visible)
		/// </summary>
		INVISIBLE
	};

	enum class ControllerType : unsigned char
	{
		PlayStation,
		Xbox
	};

	class Axis
	{
	public:
		Axis(const std::string& name);
		Axis(const Axis&) = delete;
		Axis(Axis&&) = default;

		Axis& AddKeyBind(KeyCode minKey, KeyCode maxKey);
		Axis& RemoveKeyBind(KeyCode minKey, KeyCode maxKey);
		Axis& AddStickBind(JoyStick key);
		Axis& Magnitude(float mag);
		Axis& DeadZone(float val);

		float GetMagnitude() const;
		float GetDeadZone() const;

	private:
		friend class Input;
		friend class InputViewTool;

		float m_Deadzone = 0.085f; // Default deadzone, please use the visual debugger before changing defaults.
		float m_Magnitude = 1.0f;
		KeyCode m_Keys[4]{};
		JoyStick m_JoyStick{};
		std::string m_Name = ""; // Do we save name here or only in the Input maps ?
	};

	class Action
	{
	public:
		Action(const std::string& name);
		Action(const Action&) = delete;
		Action(Action&&) = default;

		Action& AddKeyBind(KeyCode key);
		Action& RemoveKeyBind(KeyCode key);

	private:
		friend class InputViewTool;
		friend class Input;

		static const int m_KeyCount = 3;
		KeyCode m_Keys[m_KeyCount]{};
		// How many keys are bound to this action
		int m_BoundKeyCount = 0;
	};

	//
	class Input
	{
	public:
		Input(const Input&) = delete;

		Axis& CreateAxis(const std::string& name);
		bool ContainsAxis(const std::string& name) const;

		/// <summary>
		/// Get the axis object that has been created before, used for rebinding ect.
		/// </summary>
		/// <param name="name"></param>
		/// <returns></returns>
		Axis& GetAxisBinding(const std::string& name);

		/// <summary>
		/// Get current value of the axis
		/// </summary>
		/// <param name="name">Axis created with `CreateAxis`</param>
		/// <returns></returns>
		float GetAxis(const std::string& name) const;

		// Check if any input has been pressed
		bool GetAnyDown() const;

		Action& CreateAction(const std::string& name);
		bool ContainsAction(const std::string& name) const;
		/// <summary>
		/// Get the action object that has been created before, used for rebinding ect.
		/// </summary>
		/// <param name="name"></param>
		/// <returns></returns>
		Action& GetActionBinding(const std::string& name);
		// Check If this Action is being held down
		bool GetAction(const std::string& name) const;
		// Check if the Action got pressed this frame
		bool GetActionDown(const std::string& name) const;
		// Check if the action got Released this frame
		bool GetActionReleased(const std::string& name) const;
		/// <summary>
		/// Get the Raw state of an action
		/// </summary>
		/// <param name="name">Name of the action</param>
		/// <returns>The raw state of the Action</returns>
		KeyState GetActionRaw(const std::string& name) const;

		/// <summary>
		/// Get raw keycode, please do not use this for gameplay!!!
		///	These cannot be rebound... action/axis can.
		/// </summary>
		/// <param name="key">Key to check for</param>
		/// <returns> currentKeystate </returns>
		const KeyState& GetRawKeyState(KeyCode key) const;

		float GetJoystickRaw(JoyStick joyStick) const;

		/// <summary>
		/// Get all keys that have there state changed since last frame.
		/// </summary>
		/// <returns></returns>
		const std::unordered_map<KeyCode, KeyState>& GetUpdatedKeys() const;

		/// <summary>
		/// Checks if a key is a controller or keyboard button.
		/// </summary>
		/// <param name="key">Keycode that needs to be checked.</param>
		/// <returns>Returns true if the key is a controller button.</returns>
		static bool IsControllerKey(KeyCode key);

		static bool IsMouseKey(KeyCode key);

		bool IsControllerConnected() const;

		/// <summary>
		/// Checks if the controller is a PS5 or Xbox controller.
		/// </summary>
		/// <returns>Returns the controller type.</returns>
		ControllerType GetControllerType() const;

		/// <summary>
		///
		/// </summary>
		/// <param name="side">Which motor do we want to activate</param>
		/// <param name="value">factor value of rumble amount. Value has to be between 0 and 1.</param>
		void SetControllerVibration(controllerMotor side, float value);

		glm::vec2 GetMousePosition() const;
		glm::vec2 GetMouseDelta() const { return m_MouseDelta; }

		void SetCursorState(CursorState newState);
		CursorState GetCursorState() const { return m_CursorState; };

		void SetPreUnfocusCursorState(CursorState newState) { m_PreUnfocusCursorState = newState; }
		CursorState GetPreUnfocusCursorState() const { return m_PreUnfocusCursorState; };

	private:
		friend class Engine;
		friend class InputViewTool;

		Input();
		~Input();

		/// <summary>
		/// Initialize input
		/// </summary>
		/// <param name="id"></param>
		void Init(unsigned short playerIndex = 0);

		void SetController(unsigned short id);
		void Update();

		/// <summary>
		/// Generate platform specific input data, Use this to call UpdateKeyState.
		/// </summary>
		void GatherInputUpdate();

		/// <summary>
		/// Update an internal keystate.
		/// </summary>
		/// <param name="KeyDown"></param>
		/// <param name="key"></param>
		void UpdateKeyState(bool KeyDown, KeyCode key);

		std::unordered_map<std::string, Action> m_Actions{};
		std::unordered_map<std::string, Axis> m_Axes{};

		// This does not look like a ideal structure of storing keystates, but makes lookup easier
		std::unordered_map<KeyCode, KeyState> m_KeyStates{};
		std::unordered_map<JoyStick, float> m_AxisValues{};

		std::unordered_map<KeyCode, KeyState> m_UpdatedKeys{};

		float m_Vibrations[2]{0, 0};
		int m_ControllerID = 0;

		CursorState m_CursorState = CursorState::NONE;
		CursorState m_PreUnfocusCursorState = CursorState::NONE;
		glm::vec2 m_MouseDelta = glm::vec2(0.f);
	};

} // namespace Ball