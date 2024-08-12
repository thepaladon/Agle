#pragma once
#include <string>
#include <unordered_map>

namespace Ball
{
	class LaunchParameters
	{
	public:
		static int GetArgc();
		static char** GetArgv();

		static void SetParameters(const std::vector<std::string>& parameters);
		static void Clear();
		inline static int Count() { return static_cast<int>(m_CommandMap.size()); };

		// TODO: Discuss with team whether this should be "Contains" or "Get"
		static bool Contains(const std::string& parameter);

		static int GetInt(const std::string& parameter, int fallbackValue);
		static float GetFloat(const std::string& parameter, float fallbackValue);
		static std::string GetString(const std::string& parameter, std::string fallbackValue);

		static void PrintLaunchParameters();
		static std::string GetDisplayString() { return m_ParamDisplayString; }

	private:
		LaunchParameters() = default;
		~LaunchParameters() = default;

		inline static std::string m_ParamDisplayString;

		inline static std::unordered_map<std::string, std::string> m_CommandMap{};
	};
} // namespace Ball