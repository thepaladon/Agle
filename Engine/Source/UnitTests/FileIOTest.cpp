#include <Catch2/catch_amalgamated.hpp>

#include "FileIO.h"

using namespace Ball;
using namespace Catch::Matchers;

struct BinaryDataTest
{
	int i = 0;
	float w = 0;

	char text[10] = "\0";
};

CATCH_TEST_CASE("FileIO")
{
	constexpr const char* FILE_PATH = "TestFile.file";
	constexpr const char* BINARY_FILE_PATH = "BinaryTestFile.file";
	constexpr const char* PLACE_HOLDER_DATA = "TEST DATA FOR FILE";
	CATCH_SECTION("Create File")
	{
		CATCH_CHECK(Ball::FileIO::Write(FileIO::TempData, FILE_PATH, PLACE_HOLDER_DATA));

		CATCH_SECTION("File Exist")
		{
			CATCH_CHECK(FileIO::Exist(FileIO::TempData, FILE_PATH));
		}

		CATCH_SECTION("File Read")
		{
			std::string read = FileIO::Read(FileIO::TempData, FILE_PATH);
			CATCH_REQUIRE_THAT(read, Matches(PLACE_HOLDER_DATA));
		}

		CATCH_SECTION("Delete File")
		{
			CATCH_CHECK(FileIO::Delete(FileIO::TempData, FILE_PATH));

			CATCH_CHECK(!FileIO::Exist(FileIO::TempData, FILE_PATH));
		}

		// Delete file
		if (FileIO::Exist(FileIO::TempData, FILE_PATH))
			FileIO::Delete(FileIO::TempData, FILE_PATH);
	}

	CATCH_SECTION("Create Binary File")
	{
		const char* inputData = "Hello!!!\0";
		BinaryDataTest bdt{};
		bdt.i = 88;
		bdt.w = 4.20f;
		strncpy_s(bdt.text, inputData, 0);

		CATCH_CHECK(Ball::FileIO::WriteBinary(FileIO::TempData, BINARY_FILE_PATH, &bdt, sizeof(BinaryDataTest)));

		CATCH_SECTION("File Exist")
		{
			CATCH_CHECK(FileIO::Exist(FileIO::TempData, BINARY_FILE_PATH));
		}

		CATCH_SECTION("File Read")
		{
			BinaryDataTest data{};
			auto readData = FileIO::ReadBinary(FileIO::TempData, BINARY_FILE_PATH, &data, sizeof(BinaryDataTest));
			CATCH_CHECK(readData);

			CATCH_CHECK(!strcmp(data.text, bdt.text));
			CATCH_CHECK(data.i == bdt.i);
			CATCH_CHECK(data.w == bdt.w);
		}

		CATCH_SECTION("DELETE File")
		{
			CATCH_CHECK(FileIO::Delete(FileIO::TempData, BINARY_FILE_PATH));

			CATCH_CHECK(!FileIO::Exist(FileIO::TempData, BINARY_FILE_PATH));
		}

		// Delete file
		if (FileIO::Exist(FileIO::TempData, BINARY_FILE_PATH))
			FileIO::Delete(FileIO::TempData, BINARY_FILE_PATH);
	}
}