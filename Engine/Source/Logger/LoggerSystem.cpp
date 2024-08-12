#include "Logger/LoggerSystem.h"

#include <filesystem>

#include "Log.h"
#include <sstream>
#include "FileIO.h"

#include "Engine.h"
#include "Utilities/LaunchParameters.h"

using namespace Ball;

std::string currentSaveFilePath = "";

namespace LoggerInternal
{
	LoggerSystem& GetLogger()
	{
		return GetEngine().GetLogger();
	}
} // namespace LoggerInternal

LoggerSystem::LoggerSystem(const bool allowWritingToFile) : m_AllowWritingToFile(allowWritingToFile)
{
}
LoggerSystem::~LoggerSystem()
{
	Save();
}

void LoggerSystem::Init()
{
	std::string source = Ball::LaunchParameters::GetString("LogInfo", "");
	if (!source.empty())
	{
		std::stringstream ss(source);
		std::string category;
		while (!ss.eof())
		{
			getline(ss, category, ',');
			SetLogCategory(EINFO, category.c_str(), true);
		}
	}

	// Find a valid log file index
	if (m_AllowWritingToFile)
		AllowFileWriting();
}

bool LoggerSystem::IsBlocked(ELogLevel Level, const char* Category)
{
	if ((m_BlockedLevelMask & Level) != 0)
		return true; // LogLevel is blocked

	const auto categoryMask = m_BlockedCategories.find(Category);

	// If catogory doesn't exists create it and mute verbose
	if (categoryMask == m_BlockedCategories.end())
	{
		CreateLogLevel(Category);
		return (m_BlockedCategories.find(Category)->second & Level) != 0;
	}

	return (categoryMask->second & Level) != 0;
}

void LoggerSystem::CreateLogLevel(const char* Category)
{
	m_BlockedCategories.insert({Category, ELogLevel::EINFO});
}

void LoggerSystem::GenerateLogEntry(ELogLevel Level, const char* Category, const char* fileName, int lineNumber,
									const char* text)
{
	// Yes we have to do these checks in both functions,
	// At gen time it can can call this one directly if no formatting parameters are present
	if (IsBlocked(Level, Category))
		return;

	SetColor(Level);

	LogMessage(("[" + GetTimeStampString() + "]").c_str());
	LogMessage(("[" + std::string(Category) + "]").c_str());
	LogMessage(
		("[" + std::filesystem::path(fileName).filename().string() + ":" + std::to_string(lineNumber) + "]").c_str());
	LogMessage((LogLevelToString(Level) + " ").c_str());

	LogMessage(text);
	SetColor(EALL); // we use all as a "reset" value here
	LogMessage("\n");

	if (m_MemLog.size() > m_MaxMemLogSize)
	{
		// We cannot log assertions of fileio.... something horrible went wrong and we can't write..
		if (Level == ELogLevel::EASSERT && Category == LOG_FILEIO)
			return;

		Save();
	}
}

void LoggerSystem::Save()
{
	if (m_MemLog.empty() || !m_AllowWritingToFile)
		return;

	// We need to copy then clear as writeToFile will also log to memlog
	std::string memCopy = m_MemLog;
	m_MemLog.resize(0, '\0');

	ASSERT_MSG(LOG_LOGGING, !currentSaveFilePath.empty(), "Logger.Initialize has not been called");

	FileIO::Write(FileIO::Log, currentSaveFilePath, memCopy, true);

	// We don't want this to be logged to the text file.
	// Note that changing this may cause it to still not be written to the text file...
	INFO(LOG_LOGGING, "Saving logfile to disk [%s]...", FileIO::GetPath(FileIO::Log, currentSaveFilePath).c_str());
}

void LoggerSystem::LogMessage(const char* message)
{
	printf("%s", message);

	// Buildup Cache.. will be saved to disk later
	m_MemLog += std::string(message);
}

std::string LoggerSystem::GetTimeStampString()
{
	// Yes this implementation looks identical to pc.... LocalTime_s parameters are swapped around... thats why its not
	// the same...

	auto currentTime = (std::chrono::system_clock::now());
	auto startupTime = (GetEngine().GetStartupTime());
	auto timeDiff = std::chrono::duration<float>(currentTime - startupTime).count();
	std::ostringstream timeStream;

	//+1 when timeDiff = 60 we want to showcase 1 minute, otherwise it only happens at 61.. idk why
	int minutes = static_cast<int>((timeDiff + 1) / 60);
	int seconds = static_cast<int>(std::fmod(timeDiff, 60.0f));

	// Use ostringstream to format the string
	if (minutes > 0)
		timeStream << std::setfill('0') << minutes << "m ";

	timeStream << std::setfill('0') << std::setw(2) << seconds << "s";

	return timeStream.str();
}

const std::string& LoggerSystem::LogLevelToString(ELogLevel logLevel)
{
	static const std::unordered_map<ELogLevel, std::string> TEXT_MAP{
		{EINFO, "[INFO]"}, {ELOG, "[LOG]"}, {EWARN, "[WARN]"}, {EERROR, "[ERROR]"}, {EASSERT, "[ASSERT]"}};

	const auto result = TEXT_MAP.find(logLevel);
	ASSERT(LOG_LOGGING, result != TEXT_MAP.end());

	return result->second;
}

void LoggerSystem::SetLogCategory(ELogLevel level, const char* category, bool enabled)
{
	auto categoryBlock = m_BlockedCategories.find(category);
	if (categoryBlock == m_BlockedCategories.end())
	{
		CreateLogLevel(category);
		categoryBlock = m_BlockedCategories.find(category);
	}

	// Idealy we auto disable/enable lower priority categories, but math for it is funky
	if (enabled)
		for (ELogLevel i = level; (level <= categoryBlock->second) && categoryBlock->second != NONE;
			 i = static_cast<ELogLevel>(i << 1))
			categoryBlock->second = static_cast<ELogLevel>(categoryBlock->second & ~i);
	else
		for (ELogLevel i = level; i > NONE; i = static_cast<ELogLevel>(i >> 1))
			categoryBlock->second = static_cast<ELogLevel>(categoryBlock->second | i);
}

void LoggerSystem::SetLogLevel(ELogLevel level, bool enabled)
{
	if (enabled)
		for (ELogLevel i = level; (level <= m_BlockedLevelMask) && m_BlockedLevelMask != NONE;
			 i = static_cast<ELogLevel>(i << 1))
			m_BlockedLevelMask = static_cast<ELogLevel>(m_BlockedLevelMask & ~i);
	else
		for (ELogLevel i = level; i > NONE; i = static_cast<ELogLevel>(i >> 1))
			m_BlockedLevelMask = static_cast<ELogLevel>(m_BlockedLevelMask | i);
}

const std::string& LoggerSystem::GetLogCache()
{
	return m_MemLog;
}

void LoggerSystem::AllowFileWriting()
{
	if (m_AllowWritingToFile)
	{
		WARN(LOG_LOGGING, "Attempted to allow writing to file while this is already");
		return;
	}

	for (unsigned int i = 0; i < UINT_MAX; ++i)
	{
		if (!FileIO::Exist(FileIO::Log, (std::to_string(i) + ".log")))
		{
			currentSaveFilePath = std::to_string(i) + ".log";
			break;
		}
	}
	m_AllowWritingToFile = true;

	// Check if we should write current data to file...
	if (m_MemLog.size() > m_MaxMemLogSize)
	{
		Save();
	}
}
