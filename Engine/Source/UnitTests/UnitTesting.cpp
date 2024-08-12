#include "UnitTesting.h"

#include <Catch2/catch_amalgamated.hpp>
#include <iostream>

#include "Engine.h"
#include "ObjectManagerTests.cpp"
#include "AudioTests.cpp"
#include "FileIOTest.cpp"
#include "HierarchyTests.cpp"

#include "LaunchParameterTests.cpp"
#include "LoggerTests.cpp"
#include "FileWatchTest.cpp"
#include "ResourceTest.cpp"
#include "PrefabTests.cpp"
#include "TransformUnitTest.cpp"

namespace Ball
{
	int UnitTesting::RunAllTests()
	{
		std::cout << R"(
 _   _   _   _   ___   _____     _____   _____   ____    _____   ___   _   _    ____ 
| | | | | \ | | |_ _| |_   _|   |_   _| | ____| / ___|  |_   _| |_ _| | \ | |  / ___|
| | | | |  \| |  | |    | |       | |   |  _ | \___ \     | |    | |  |  \| | | |  _
| |_| | | |\  |  | |    | |       | |   | |___   ___) |   | |    | |  | |\  | | |_| |
 \___/  |_ |\_| |___|   |_|       |_|   |_____| |____/    |_|   |___| |_| \_|  \____|
	)" << '\n';

		const char* args[2] = {"", "--list-tests"};
		// Reusing session does not work
		auto session = Catch::Session();

		int returncode = session.run();
		session.run(2, args);
		return returncode;
	}
} // namespace Ball
