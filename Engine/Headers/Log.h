#pragma once
#include "Logger/LoggerSystem.h"

namespace LoggerInternal
{
	Ball::LoggerSystem& GetLogger();
}

constexpr const char* LOG_GENERIC = "GENERIC";
constexpr const char* LOG_LOGGING = "LOGGING";
constexpr const char* LOG_FILEIO = "FILEIO";
constexpr const char* LOG_GAMEOBJECTS = "GAME-OBJECTS";
constexpr const char* LOG_SERIALIZER = "SERIALIZER";
constexpr const char* LOG_MODIO = "MODIO";
constexpr const char* LOG_INPUT = "INPUT";
constexpr const char* LOG_GRAPHICS = "GRAPHICS";
constexpr const char* LOG_RESOURCE = "RESOURCES";
constexpr const char* LOG_USER_SERVICE = "USER-SERVICE";
constexpr const char* LOG_LAUNCHPARAM = "LAUNCH-PARAMS";
constexpr const char* LOG_REPLAYSYSTEM = "REWIND-SYSTEM";
constexpr const char* LOG_AUDIO = "AUDIO";
constexpr const char* LOG_PHYSICS = "PHYSICS";
constexpr const char* LOG_ULTRALIGHT = "ULTRALIGHT"; // used for logs from ultralight Libary
constexpr const char* LOG_UI = "UI"; // Used for our UI code/system
constexpr const char* LOG_STEAMWORKS = "STEAM";
constexpr const char* LOG_LEVEL_EDITOR = "LEVEL-EDITOR";
constexpr const char* LOG_STATEMACHINE = "STATE";

#undef ERROR

#define ASSERT(LOG_CATEGORY, CONDITION) ASSERT_MSG(LOG_CATEGORY, CONDITION, "")

#ifdef ENABLE_LOGGING
#define LOG_CLEAR() LoggerInternal::GetLogger().Clear()
#define LOG_ENABLE(LOG_CATEGORY, LOG_LEVEL) LoggerInternal::GetLogger().SetLogCategory(LOG_LEVEL, LOG_CATEGORY, true)
#define LOG_DISABLE(LOG_CATEGORY, LOG_LEVEL) LoggerInternal::GetLogger().SetLogCategory(LOG_LEVEL, LOG_CATEGORY, false)
#define LOG_LEVEL_ENABLE(LOG_LEVEL) LoggerInternal::GetLogger().SetLogLevel(LOG_LEVEL, true)
#define LOG_LEVEL_DISABLE(LOG_LEVEL) LoggerInternal::GetLogger().SetLogLevel(LOG_LEVEL, false)

#define INFO(LOG_CATEGORY, ...)                   \
	LoggerInternal::GetLogger().GenerateLogEntry( \
		Ball::Logger::ELogLevel::EINFO, LOG_CATEGORY, __FILE__, __LINE__, __VA_ARGS__)
#define LOG(LOG_CATEGORY, ...)                    \
	LoggerInternal::GetLogger().GenerateLogEntry( \
		Ball::Logger::ELogLevel::ELOG, LOG_CATEGORY, __FILE__, __LINE__, __VA_ARGS__)
#define WARN(LOG_CATEGORY, ...)                   \
	LoggerInternal::GetLogger().GenerateLogEntry( \
		Ball::Logger::ELogLevel::EWARN, LOG_CATEGORY, __FILE__, __LINE__, __VA_ARGS__)
#define ERROR(LOG_CATEGORY, ...)                  \
	LoggerInternal::GetLogger().GenerateLogEntry( \
		Ball::Logger::ELogLevel::EERROR, LOG_CATEGORY, __FILE__, __LINE__, __VA_ARGS__)

#define ASSERT_MSG(LOG_CATEGORY, CONDITION, ...)                                                          \
	do                                                                                                    \
	{                                                                                                     \
		if (!(CONDITION))                                                                                 \
		{                                                                                                 \
			if constexpr (sizeof(#__VA_ARGS__) > 1)                                                       \
			{                                                                                             \
				LoggerInternal::GetLogger().GenerateLogEntry(                                             \
					Ball::Logger::ELogLevel::EASSERT, LOG_CATEGORY, __FILE__, __LINE__, __VA_ARGS__);     \
			}                                                                                             \
			else                                                                                          \
			{                                                                                             \
				LoggerInternal::GetLogger().GenerateLogEntry(Ball::Logger::ELogLevel::EASSERT,            \
															 LOG_CATEGORY,                                \
															 __FILE__,                                    \
															 __LINE__,                                    \
															 "Condition failed, no extra info given");    \
			}                                                                                             \
			if (LOG_CATEGORY != LOG_FILEIO) /*Only save if its not fileio crashing*/                      \
				LoggerInternal::GetLogger().Save();                                                       \
			if constexpr (sizeof(#__VA_ARGS__) > 1)                                                       \
			{                                                                                             \
				Ball::AssertWindow(LOG_CATEGORY, __func__, __LINE__, __FILE__, __VA_ARGS__);              \
			}                                                                                             \
			else                                                                                          \
			{                                                                                             \
				Ball::AssertWindow(                                                                       \
					LOG_CATEGORY, __func__, __LINE__, __FILE__, "Condition failed, no extra info given"); \
			}                                                                                             \
			__debugbreak();                                                                               \
		}                                                                                                 \
	} while (0)

#define THROW(LOG_CATEGORY, ...)                                                                                     \
	do                                                                                                               \
	{                                                                                                                \
		if constexpr (sizeof(#__VA_ARGS__) > 1)                                                                      \
		{                                                                                                            \
			LoggerInternal::GetLogger().GenerateLogEntry(                                                            \
				Ball::Logger::ELogLevel::EASSERT, LOG_CATEGORY, __FILE__, __LINE__, __VA_ARGS__);                    \
		}                                                                                                            \
		else                                                                                                         \
		{                                                                                                            \
			LoggerInternal::GetLogger().GenerateLogEntry(Ball::Logger::ELogLevel::EASSERT,                           \
														 LOG_CATEGORY,                                               \
														 __FILE__,                                                   \
														 __LINE__,                                                   \
														 "Condition failed, no extra info given");                   \
		}                                                                                                            \
		if (LOG_CATEGORY != LOG_FILEIO) /*Only save if its not fileio crashing*/                                     \
			LoggerInternal::GetLogger().Save();                                                                      \
		if constexpr (sizeof(#__VA_ARGS__) > 1)                                                                      \
		{                                                                                                            \
			Ball::AssertWindow(LOG_CATEGORY, __func__, __LINE__, __FILE__, __VA_ARGS__);                             \
		}                                                                                                            \
		else                                                                                                         \
		{                                                                                                            \
			Ball::AssertWindow(LOG_CATEGORY, __func__, __LINE__, __FILE__, "Condition failed, no extra info given"); \
		}                                                                                                            \
		__debugbreak();                                                                                              \
	} while (0)

#else
#define LOG_ENABLE(LOG_CATEGORY, LOG_LEVEL) \
	do                                      \
	{                                       \
		(void)sizeof(LOG_CATEGORY);         \
		(void)sizeof(LOG_LEVEL);            \
	} while (0)
#define LOG_DISABLE(LOG_CATEGORY, LOG_LEVEL) \
	do                                       \
	{                                        \
		(void)sizeof(LOG_CATEGORY);          \
		(void)sizeof(LOG_LEVEL);             \
	} while (0)
#define LOG_LEVEL_ENABLE(LOG_LEVEL) \
	do                              \
	{                               \
		(void)sizeof(LOG_LEVEL);    \
	} while (0)
#define LOG_LEVEL_DISABLE(LOG_LEVEL) \
	do                               \
	{                                \
		(void)sizeof(LOG_LEVEL);     \
	} while (0)

#define INFO(LOG_CATEGORY, ...)     \
	do                              \
	{                               \
		(void)sizeof(LOG_CATEGORY); \
		(void)sizeof(__VA_ARGS__);  \
	} while (0)
#define LOG(LOG_CATEGORY, ...)      \
	do                              \
	{                               \
		(void)sizeof(LOG_CATEGORY); \
		(void)sizeof(__VA_ARGS__);  \
	} while (0)
#define WARN(LOG_CATEGORY, ...)     \
	do                              \
	{                               \
		(void)sizeof(LOG_CATEGORY); \
		(void)sizeof(__VA_ARGS__);  \
	} while (0)
#define ERROR(LOG_CATEGORY, ...)    \
	do                              \
	{                               \
		(void)sizeof(LOG_CATEGORY); \
		(void)sizeof(__VA_ARGS__);  \
	} while (0)

#define ASSERT_MSG(LOG_CATEGORY, CONDITION, ...) \
	do                                           \
	{                                            \
		(void)sizeof(LOG_CATEGORY);              \
		(void)sizeof(CONDITION);                 \
		(void)sizeof(__VA_ARGS__);               \
	} while (0)

#define THROW(LOG_CATEGORY, ...)    \
	do                              \
	{                               \
		(void)sizeof(LOG_CATEGORY); \
		(void)sizeof(__VA_ARGS__);  \
	} while (0)

#define LOG_CLEAR() \
	do              \
	{               \
	} while (0)
#endif