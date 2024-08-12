#include <Catch2/catch_amalgamated.hpp>

#include "Utilities/LaunchParameters.h"

#include "FileIO.h"
#include "Log.h"

using namespace Ball;
using namespace Catch::Matchers;

CATCH_TEST_CASE("LaunchParameters")
{
	// TODO BEFORE MERGE RETURN THIS
	/*
	std::vector<std::string> exampleParameters;
	// Add integer examples
	exampleParameters.push_back(std::string("-testingWidth=800"));
	exampleParameters.push_back(std::string("-testingHeight=600"));

	// Add float examples
	exampleParameters.push_back(std::string("-testingR=0.2"));
	exampleParameters.push_back(std::string("-testingG=0.3"));
	exampleParameters.push_back(std::string("-testingB=0.5"));

	// Add string examples
	exampleParameters.push_back(std::string("-testingStartingLevel=lvl1.json"));
	exampleParameters.push_back(std::string("-testingPlatform=Windows"));

	// Add empty example
	exampleParameters.push_back(std::string("-testingDebugMode"));

	LaunchParameters::SetParameters(exampleParameters);

	CATCH_SECTION("Checking if parameters can be found")
	{
		//First, check integers
		bool containsInt1 = LaunchParameters::Contains("testingWidth");
		bool containsInt2 = LaunchParameters::Contains("testingHeight");
		bool containsInt3 = LaunchParameters::Contains("testingDepth");

		CATCH_REQUIRE(containsInt1);
		CATCH_REQUIRE(containsInt2);
		CATCH_REQUIRE(!containsInt3);

		//Next, check floats
		bool containsFloat1 = LaunchParameters::Contains("testingR");
		bool containsFloat2 = LaunchParameters::Contains("testingG");
		bool containsFloat3 = LaunchParameters::Contains("testingW");

		CATCH_REQUIRE(containsFloat1);
		CATCH_REQUIRE(containsFloat2);
		CATCH_REQUIRE(!containsFloat3);

		//After that, check strings
		bool containsString1 = LaunchParameters::Contains("testingStartingLevel");
		bool containsString2 = LaunchParameters::Contains("testingPlatform");
		bool containsString3 = LaunchParameters::Contains("otherThing");

		CATCH_REQUIRE(containsString1);
		CATCH_REQUIRE(containsString2);
		CATCH_REQUIRE(!containsString3);

		//Lastly, check empty parameters
		bool containsEmpty1 = LaunchParameters::Contains("testingDebugMode");
		bool containsEmpty2 = LaunchParameters::Contains("testingSomeOtherThing");

		CATCH_REQUIRE(containsEmpty1);
		CATCH_REQUIRE(!containsEmpty2);
	}

	CATCH_SECTION("Getting integers")
	{
		int value1 = LaunchParameters::GetInt("testingWidth", 0);
		int value2 = LaunchParameters::GetInt("testingHeight", 0);
		int value3 = LaunchParameters::GetInt("depth", 0);

		CATCH_REQUIRE(value1 == 800);
		CATCH_REQUIRE(value2 == 600);
		CATCH_REQUIRE(value3 == 0);
	}

	CATCH_SECTION("Getting floats")
	{
		float value1 = LaunchParameters::GetFloat("testingR", 0.f);
		float value2 = LaunchParameters::GetFloat("testingG", 0.f);
		float value3 = LaunchParameters::GetFloat("testingB", 0.f);
		float value4 = LaunchParameters::GetFloat("testingA", 1.f);

		float epsilon = 0.001f;
		CATCH_REQUIRE(fabs(value1 - 0.2f) < epsilon);
		CATCH_REQUIRE(fabs(value2 - 0.3f) < epsilon);
		CATCH_REQUIRE(fabs(value3 - 0.5f) < epsilon);
		CATCH_REQUIRE(fabs(value4 - 1.0f) < epsilon);
	}

	CATCH_SECTION("Getting strings")
	{
		std::string value1 = LaunchParameters::GetString("testingStartingLevel", "mainMenu.json");
		std::string value2 = LaunchParameters::GetString("testingPlatform", "Undefined");
		std::string value3 = LaunchParameters::GetString("testingVersion", "1.0.0");

		CATCH_REQUIRE(value1 == "lvl1.json");
		CATCH_REQUIRE(value2 == "Windows");
		CATCH_REQUIRE(value3 == "1.0.0");
	}

	CATCH_SECTION("Clearing parameters")
	{
		int preClearSize = LaunchParameters::Count();
		CATCH_REQUIRE(preClearSize > 0);

		LaunchParameters::Clear();
		int postClearSize = LaunchParameters::Count();
		CATCH_REQUIRE(postClearSize == 0);
	}

	FileIO::Shutdown();
	*/
}