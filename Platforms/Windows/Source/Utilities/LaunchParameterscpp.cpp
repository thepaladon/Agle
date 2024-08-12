#include "Utilities/LaunchParameters.h"

namespace Ball
{
	int LaunchParameters::GetArgc()
	{
		return __argc;
	}

	char** LaunchParameters::GetArgv()
	{
		return __argv;
	}
} // namespace Ball