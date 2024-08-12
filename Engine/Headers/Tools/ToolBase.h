#pragma once
#include <string>

#include "GameObjects/Serialization/ObjectSerializer.h"
#include "Tools/ToolManager.h"

namespace Ball
{
	class ToolBase
	{
	public:
		ToolBase() {}
		virtual ~ToolBase() {}

		virtual void Init() {}
		void Init(std::string name, ToolCatagory toolCatagory);

		virtual void Draw() {}
		virtual void Update() {}
		virtual void Event() {}

		const std::string& GetName() const { return m_Name; }
		ToolCatagory GetToolCatagory() const { return m_ToolCatagory; }
		ToolInterfaceType GetInterfaceType() const { return m_ToolInterfaceType; }

		void ToggleOpen() { m_Open = !m_Open; }
		void Open() { m_Open = true; }
		void Close() { m_Open = false; }
		bool IsOpen() const { return m_Open; }

	protected:
		/// <summary>
		/// When this tool gets saved this allows you to serialize the object
		/// </summary>
		/// <param name="archive"></param>
		virtual void Serialize(SerializeArchive& archive) {}
		void DrawSaveBar();
		void SerializeSettings(const SerializeArchiveType& serializationType, const std::string& fileName);

		std::string m_Name;
		ToolCatagory m_ToolCatagory;
		ToolInterfaceType m_ToolInterfaceType;
		bool m_Open;
	};
} // namespace Ball
