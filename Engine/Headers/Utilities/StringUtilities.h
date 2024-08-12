#pragma once
#include <string>

namespace Ball
{
	enum class KeyState : unsigned char;
	enum KeyCode : unsigned int;
	enum JoyStick : unsigned char;
	class Input;
} // namespace Ball

namespace Ball
{
	namespace Utilities
	{
		// This function is used for taking the name of a class object (retrieved from using
		// "typeid(OBJECT_TYPE).name()") and returns it as a "clean" string Example: On Windows, using
		// "typeid(MovingPlatform).name()" results in the string "class Ball::MovingPlatform". This function will take
		// that string
		//		   and changes it to "MovingPlatform".
		std::string GetCleanClassTypeName(std::string str);

		// Removes the "m_..." from a string.
		std::string RemoveStringMemberPrefix(std::string str);

		/// <summary>
		/// Convert a keystate to string
		/// </summary>
		const char* ToString(const KeyState& key);

		/// <summary>
		/// Convert a KeyCode to string
		/// </summary>
		const char* ToString(const KeyCode& key);

		/// <summary>
		/// Convert a JoyStick to string
		/// </summary>
		const char* ToString(const JoyStick& key);

	} // namespace Utilities
} // namespace Ball