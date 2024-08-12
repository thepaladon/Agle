#include "Logger/LoggerSystem.h"
#include "Log.h"

#include <sstream>
#include <string>
#include <chrono>
#include <wrl.h>

using namespace Ball;

void LoggerSystem::Clear(bool clearConsole)
{
	m_MemLog.clear();
	if (clearConsole)
		system("cls");
}

void LoggerSystem::SetColor(ELogLevel level)
{
	static const std::unordered_map<ELogLevel, int> m_ColorLookup{
		{EINFO, 8}, {ELOG, 7}, {EWARN, 6}, {EERROR, 4}, {EASSERT, 4}, {EALL, 7}};

	const auto foundColor = m_ColorLookup.find(level);
	ASSERT(LOG_LOGGING, foundColor != m_ColorLookup.end());

	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	SetConsoleTextAttribute(hConsole, foundColor->second);
}

void Ball::AssertWindow(const char* Category, const char* func, int line, const char* file, const char* message)
{
	std::ostringstream messageStream;
	messageStream << "Assertion failed ! \n";
	messageStream << file << "->" << func << "() [" << line << "]\n";
	messageStream << message;

	MessageBoxA(NULL,
				messageStream.str().c_str(),
				("[" + std::string(Category) + "] Assertion Failed").c_str(),
				MB_ICONERROR | MB_OK);
}