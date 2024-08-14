#pragma once
#include <string>
#include <unordered_map>
#include <vector>

namespace Ball
{
	enum class ToolCatagory
	{
		MENU,
		ENGINE,
		GRAPHICS,
		RESOURCES,
		MODIO,
		PHYSICS,
		AUDIO,
		OTHER
	};

	enum class ToolInterfaceType
	{
		WINDOW,
		EVENT
	};

	class ToolBase;
	class ToolManager
	{
	public:
		ToolManager() {}
		~ToolManager() {}

		void Init();
		void Shutdown();
		void OnImgui();

	private:
		template<typename T>
		void RegisterTool(const std::string& ToolName)
		{
			T* tool = new T();
			tool->Init();
			m_Tools.push_back(tool);
			m_ToolLookup.insert({ToolName, tool});
		}

		void OpenTool(const std::string& name) const;
		void CreateToolMenu(const char* menuName, ToolCatagory category) const;

		// Tools
		std::vector<ToolBase*> m_Tools;

		std::unordered_map<std::string, ToolBase*> m_ToolLookup;

		// Flags
		bool m_ShowDemoWindow = false;
		bool m_ShowToolManager = true;
	};
} // namespace Ball
