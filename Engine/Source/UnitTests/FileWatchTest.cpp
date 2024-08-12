#include <Catch2/catch_amalgamated.hpp>

#include "Engine.h"
#include "FileIO.h"
#include "Utilities/FileWatch.h"

using namespace Ball;

class FileWatchTestObject : public IFileWatchListener
{
public:
	FileWatchTestObject(FileIO::DirectoryType dt, const std::string& path) : m_SourcePath(path)
	{
		AddFileWatch(dt, m_SourcePath);
	}

	void OnFileWatchEvent(const std::string& path) override { m_FileWatched = true; }

	std::string m_SourcePath;

	std::string m_FileWatchedPath;
	bool m_FileWatched = false;
};

CATCH_TEST_CASE("FileWatch")
{
	const char* m_FileToWatch = "filewatchTest.txt";

	// We don't care about the data in the file other tests cover that !
	FileIO::Write(FileIO::TempData, m_FileToWatch, "");

	FileWatchTestObject watcher = FileWatchTestObject(FileIO::TempData, m_FileToWatch);

	// Call this to get the callbacks invoked
	GetEngine().GetFileWatchSystem().Update();

	// The file should not have received a watch event, as the file content was not changed since we started watching.
	CATCH_REQUIRE_FALSE(watcher.m_FileWatched);

	FileIO::Write(FileIO::TempData, m_FileToWatch, "Some test data", true);

	GetEngine().GetFileWatchSystem().Update();

#ifdef ENABLE_FILE_WATCH
	// The file content should now receive this event !
	CATCH_REQUIRE(watcher.m_FileWatched);
#else
	// File watch is disabled should have not received an update !
	CATCH_REQUIRE_FALSE(watcher.m_FileWatched);
#endif

	FileIO::Delete(FileIO::TempData, m_FileToWatch);
}
