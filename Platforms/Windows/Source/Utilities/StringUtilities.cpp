#include "Utilities/StringUtilities.h"

namespace Ball
{
	namespace Utilities
	{
		std::string GetCleanClassTypeName(std::string classTypeName)
		{
			if ((classTypeName.find("::")) != std::string::npos)
				classTypeName.erase(classTypeName.begin(),
									classTypeName.begin() + classTypeName.find_last_of("::") + 1);
			return classTypeName;
		}

		std::string RemoveStringMemberPrefix(std::string str)
		{
			auto prefixIndex = str.find_first_of("m_");
			if (prefixIndex != std::string::npos)
				str.erase(str.begin(), str.begin() + prefixIndex + std::string("m_").length());
			return str;
		}
	} // namespace Utilities
} // namespace Ball