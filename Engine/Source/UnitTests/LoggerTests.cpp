#include <Catch2/catch_amalgamated.hpp>

#include "FileIO.h"
#include "Log.h"

using namespace Ball;
using namespace Catch::Matchers;

CATCH_TEST_CASE("Logger")
{
	constexpr const char* LOG_UNIT_TEST = "UNIT_TEST";
	constexpr const char* LOG_INFO = "INFO should be printed !";
	constexpr const char* LOG_LOG = "Log should be printed !";
	constexpr const char* LOG_WARN = "WARN should be printed !";
	constexpr const char* LOG_ERROR = "ERROR should be printed !";
	constexpr const char* LOG_ASSERT = "ASSERT should be printed !";

	constexpr const char* LOG_NO_INFO = "Info should NOT be printed !";
	constexpr const char* LOG_NO_LOG = "Log should NOT be printed !";
	constexpr const char* LOG_NO_WARN = "WARN should NOT be printed !";
	constexpr const char* LOG_NO_ERROR = "ERROR should NOT be printed !";
	[[maybe_unused]] constexpr const char* LOG_NO_ASSERT = "ASSERT should NOT be printed !";

	{ // This scope makes sure Logger is destroyed before we Deconstruct FileIO.
		LoggerSystem log = Ball::LoggerSystem(false);
		// We cannot test logging macro's here... Maybe we test that where ever we have the engine initialized..
		CATCH_SECTION("Generic Log test")
		{
			log.GenerateLogEntry(ELogLevel::ELOG, LOG_UNIT_TEST, __FILE__, __LINE__, LOG_LOG);
			CATCH_REQUIRE_THAT(log.GetLogCache(), !IsEmpty());
			CATCH_REQUIRE_THAT(log.GetLogCache(), EndsWith(std::string(LOG_LOG) + "\n"));
			CATCH_REQUIRE_THAT(log.GetLogCache(), ContainsSubstring("[" + std::string(LOG_UNIT_TEST) + "]"));
			// Note that this is a hardcoded test case, we check if the filename (__FILE__) and line number (__LINE__)
			// work as intended.
			CATCH_REQUIRE_THAT(log.GetLogCache(), ContainsSubstring("[LoggerTests.cpp:29]"));
		}

		CATCH_SECTION("Clear log")
		{
			log.GenerateLogEntry(ELogLevel::ELOG, LOG_UNIT_TEST, __FILE__, __LINE__, LOG_LOG);
			log.Clear(false); // clear console is not safe within unit tests..
			CATCH_REQUIRE_THAT(log.GetLogCache(), Catch::Matchers::IsEmpty());
		}

		CATCH_SECTION("Disable INFO Category by default")
		{
			log.GenerateLogEntry(ELogLevel::EINFO, LOG_UNIT_TEST, __FILE__, __LINE__, LOG_NO_INFO);
			CATCH_REQUIRE_THAT(log.GetLogCache(), Catch::Matchers::IsEmpty()); // Info is disabled by default;
		}

		CATCH_SECTION("Enable EINFO Category")
		{
			log.SetLogCategory(ELogLevel::EINFO, LOG_UNIT_TEST, true);

			log.GenerateLogEntry(ELogLevel::EINFO, LOG_UNIT_TEST, __FILE__, __LINE__, LOG_INFO);
			CATCH_REQUIRE_THAT(log.GetLogCache(), !IsEmpty());
		}

		CATCH_SECTION("Category LogLevel")
		{
			log.SetLogCategory(ELogLevel::EWARN, LOG_UNIT_TEST, false);
			CATCH_SECTION("Disable Warnings", "This should prevent printing from warning, Log and info")
			{
				log.GenerateLogEntry(ELogLevel::EINFO, LOG_UNIT_TEST, __FILE__, __LINE__, LOG_NO_INFO);
				log.GenerateLogEntry(ELogLevel::ELOG, LOG_UNIT_TEST, __FILE__, __LINE__, LOG_NO_LOG);
				log.GenerateLogEntry(ELogLevel::EWARN, LOG_UNIT_TEST, __FILE__, __LINE__, LOG_NO_WARN);
				CATCH_REQUIRE_THAT(log.GetLogCache(), IsEmpty());

				log.GenerateLogEntry(ELogLevel::EERROR, LOG_UNIT_TEST, __FILE__, __LINE__, LOG_ERROR);
				CATCH_REQUIRE_THAT(log.GetLogCache(), !IsEmpty());
			}

			CATCH_SECTION("Enable Log", "This should allow printing of Warning and Log again")
			{
				log.SetLogCategory(ELogLevel::ELOG, LOG_UNIT_TEST, true);

				log.GenerateLogEntry(ELogLevel::EINFO, LOG_UNIT_TEST, __FILE__, __LINE__, LOG_NO_INFO);
				log.GenerateLogEntry(ELogLevel::ELOG, LOG_UNIT_TEST, __FILE__, __LINE__, LOG_LOG);
				log.GenerateLogEntry(ELogLevel::EWARN, LOG_UNIT_TEST, __FILE__, __LINE__, LOG_WARN);

				CATCH_REQUIRE_THAT(log.GetLogCache(), !ContainsSubstring(LOG_NO_INFO));
				CATCH_REQUIRE_THAT(log.GetLogCache(), ContainsSubstring(LOG_LOG));
				CATCH_REQUIRE_THAT(log.GetLogCache(), ContainsSubstring(LOG_WARN));
			}
		}

		CATCH_SECTION("Global LogLevel")
		{
			log.SetLogLevel(ELogLevel::EERROR, false);
			CATCH_SECTION("Disable Errors", "This should allow only printing of assertions..")
			{
				log.GenerateLogEntry(ELogLevel::EINFO, LOG_UNIT_TEST, __FILE__, __LINE__, LOG_NO_INFO);
				log.GenerateLogEntry(ELogLevel::ELOG, LOG_UNIT_TEST, __FILE__, __LINE__, LOG_NO_LOG);
				log.GenerateLogEntry(ELogLevel::EWARN, LOG_UNIT_TEST, __FILE__, __LINE__, LOG_NO_WARN);
				log.GenerateLogEntry(ELogLevel::EERROR, LOG_UNIT_TEST, __FILE__, __LINE__, LOG_NO_ERROR);
				CATCH_REQUIRE_THAT(log.GetLogCache(), IsEmpty());
			}

			CATCH_SECTION("Can assert")
			{
				log.GenerateLogEntry(ELogLevel::EASSERT, LOG_UNIT_TEST, __FILE__, __LINE__, LOG_ASSERT);
				CATCH_REQUIRE_THAT(log.GetLogCache(), ContainsSubstring(LOG_ASSERT));
			}

			CATCH_SECTION("Enable Warnings", "Warnings and Errors should work again, but nothing else")
			{
				log.SetLogLevel(ELogLevel::EWARN, true);
				log.GenerateLogEntry(ELogLevel::EINFO, LOG_UNIT_TEST, __FILE__, __LINE__, LOG_NO_INFO);
				log.GenerateLogEntry(ELogLevel::ELOG, LOG_UNIT_TEST, __FILE__, __LINE__, LOG_NO_LOG);
				CATCH_REQUIRE_THAT(log.GetLogCache(), IsEmpty());

				log.GenerateLogEntry(ELogLevel::EWARN, LOG_UNIT_TEST, __FILE__, __LINE__, LOG_WARN);
				log.GenerateLogEntry(ELogLevel::EERROR, LOG_UNIT_TEST, __FILE__, __LINE__, LOG_ERROR);

				CATCH_REQUIRE_THAT(log.GetLogCache(), ContainsSubstring(LOG_WARN));
				CATCH_REQUIRE_THAT(log.GetLogCache(), ContainsSubstring(LOG_ERROR));
			}
		}
	}
}