#include "Utilities/StringUtilities.h"
#include "Input/Input.h"
#include "Input/KeyCodes.h"

namespace Ball
{
	namespace Utilities
	{
		const char* ToString(const KeyState& key)
		{
			switch (key)
			{
			case KeyState::DOWN:
				return "DOWN";
			case KeyState::NONE:
				return "NONE";
			case KeyState::PRESSED:
				return "PRESSED";
			case KeyState::RELEASED:
				return "RELEASED";
			}

			return "ERROR";
		}

		const char* ToString(const KeyCode& key)
		{
			const auto& lookup = InputInternal::m_KeyCodeNameLookup.find(key);
			if (lookup != InputInternal::m_KeyCodeNameLookup.end())
				return lookup->second;
			else
				return "Undef";
		}

		const char* ToString(const JoyStick& key)
		{
			return InputInternal::m_JoystickNameLookup.find(key)->second;
		}
	} // namespace Utilities
} // namespace Ball
