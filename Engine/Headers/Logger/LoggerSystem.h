#pragma once
#include <string>
#include <unordered_map>

namespace Ball
{
	namespace Logger
	{
		enum ELogLevel
		{
			NONE = 0,
			EINFO = 1 << 0,
			ELOG = 1 << 1,
			EWARN = 1 << 2,
			EERROR = 1 << 3,
			EASSERT = 1 << 4, // DON'T USE THIS IN USER CODE PLZ..
			EALL = -1 // This is unused but required for visual studio to visualize enums in debugging....
		};
	} // namespace Logger
	using namespace Logger;
	class LoggerSystem
	{
	public:
		LoggerSystem(bool allowWritingToFile);
		~LoggerSystem();

		void Init();

		/// <summary>
		/// This function will generate a log entry, Do not call this directly instead use the macros from Log.h
		/// </summary>
		/// <param name="Level">The serverity of the log entry</param>
		/// <param name="Category">Category of Log entry, macros defined in Log.h</param>
		/// <param name="fileName">filename of the log origin</param>
		///	/// <param name="fileName">line number of the log origin</param>
		/// <param name="text"> The log to be printed to the console</param>
		void GenerateLogEntry(ELogLevel Level, const char* Category, const char* fileName, int lineNumber,
							  const char* text = "");

		/// <summary>
		/// This function will generate a log entry, Do not call this directly instead use the macros from Log.h
		/// </summary>
		/// <param name="Level">The serverity of the log entry</param>
		/// <param name="Category">Category of Log entry, macros defined in Log.h</param>
		/// <param name="fileName">filename of the log origin</param>
		///	/// <param name="fileName">line number of the log origin</param>
		/// <param name="fmt"> The log to be printed to the console</param>
		/// <param name="...params">List of parameters formatted with sprintf</param>
		template<class... Types>
		void GenerateLogEntry(ELogLevel Level, const char* Category, const char* fileName, int lineNumber,
							  const char* fmt, Types... params);

		// This will clear the console log AND BUFFER!, but not Log file
		void Clear(bool clearConsole = true);
		void Save();

		void SetLogCategory(ELogLevel level, const char* category,
							bool enabled); // LOG_LEVEL_ENABLE / LOG_LEVEL_DISABLE
		void SetLogLevel(ELogLevel level, bool enabled); // LOG_ENABLE / LOG_DISABLE

		const std::string& GetLogCache();

		/// <summary>
		/// Enable functionality of writing to file...
		/// </summary>
		/// <returns></returns>
		void AllowFileWriting();

	private:
		// Function called to log to file & console, User responsible to end line.
		void LogMessage(const char* message);
		void SetColor(ELogLevel);

		const std::string& LogLevelToString(ELogLevel logLevel);
		std::string GetTimeStampString();

		std::unordered_map<std::string, ELogLevel> m_BlockedCategories{};
		// Global ignore mask for Logging
		ELogLevel m_BlockedLevelMask = ELogLevel::NONE;

		bool IsBlocked(ELogLevel Level, const char* Category);

		void CreateLogLevel(const char* Category);

		bool m_AllowWritingToFile = true;
		static constexpr int m_MaxMemLogSize = 500; // The max size of MemLog before we write it to disk (in characters)
		std::string m_MemLog{}; // the memory cache of text we still have to write to disk
	};

	template<class... Types>
	void LoggerSystem::GenerateLogEntry(ELogLevel Level, const char* Category, const char* fileName, int lineNumber,
										const char* fmt, Types... params)
	{
		if (IsBlocked(Level, Category))
			return;

		// Cursed way of getting string out of const char*'s
		std::string formattedText;
		{
			// Calculate the required buffer size
			const int bufferSize = std::snprintf(nullptr, 0, fmt, params...) + 1; // +1 for null terminator

			formattedText = std::string(bufferSize, '\0');

			// Format the string
			std::sprintf(formattedText.data(), fmt, params...);
		}

		GenerateLogEntry(Level, Category, fileName, lineNumber, formattedText.c_str());
	}

	// Visual window notifying user that application has halted execution
	extern void AssertWindow(const char* Category, const char* func, int line, const char* file,
							 const char* message = "");

	template<class... Types>
	static void AssertWindow(const char* Category, const char* func, int line, const char* file, const char* fmt,
							 Types... params)
	{
		// Asserts always work even if its ignored category..

		// Cursed way of getting string out of const char*'s
		std::string formattedText;
		{
			// Calculate the required buffer size
			const int bufferSize = std::snprintf(nullptr, 0, fmt, params...) + 1; // +1 for null terminator

			formattedText = std::string(bufferSize, '\0');

			// Format the string
			std::sprintf(formattedText.data(), fmt, params...);
		}

		AssertWindow(Category, func, line, file, formattedText.c_str());
	}
} // namespace Ball