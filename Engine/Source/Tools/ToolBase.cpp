#include "Tools/ToolBase.h"

#include "FileIO.h"
#include "imgui.h"
#include "GameObjects/Serialization/ObjectSerializer.h"
#include "Tools/ToolManager.h"

#include <ImGui/imgui_stdlib.h>

using namespace Ball;

void Ball::ToolBase::Init(std::string name, ToolCatagory toolCatagory)
{
	m_Name = name;
	m_ToolCatagory = toolCatagory;
	m_Open = false;
}

void ToolBase::DrawSaveBar()
{
	static std::string fileName = "";

	if (ImGui::Button(("Save##" + m_Name).c_str()))
	{
		fileName = "";
		ImGui::OpenPopup("SaveTool");
	}
	ImGui::SameLine();

	if (ImGui::Button(("Load##" + m_Name).c_str()))
	{
		ImGui::OpenPopup("LoadTool");
	}

	if (ImGui::BeginPopup("SaveTool"))
	{
		ImGui::InputText("FileName", &fileName);

		if (ImGui::Button("Save"))
		{
			if (!fileName.empty())
			{
				SerializeSettings(SerializeArchiveType::SAVE_DATA, fileName);
				ImGui::CloseCurrentPopup();
			}
		}
		ImGui::EndPopup();
	}

	if (ImGui::BeginPopup("LoadTool"))
	{
		auto path = FileIO::GetPath(FileIO::ToolPreset) + "/" + m_Name;

		if (ImGui::ListBoxHeader("Saves"))
		{
			if (std::filesystem::exists(path))
				for (const auto& entry : std::filesystem::directory_iterator(path))
				{
					auto filename = entry.path().filename().string();
					filename = filename.substr(0, filename.find('.'));
					if (ImGui::Selectable(filename.c_str()))
					{
						SerializeSettings(SerializeArchiveType::LOAD_DATA, filename);
						// handle selection
						ImGui::CloseCurrentPopup();
					}
				}

			ImGui::ListBoxFooter();
		}
		ImGui::EndPopup();
	}
}

void ToolBase::SerializeSettings(const SerializeArchiveType& serializationType, const std::string& fileName)
{
	nlohmann::ordered_json data = {};
	if (serializationType == SerializeArchiveType::LOAD_DATA)
	{
		auto stringData = FileIO::Read(FileIO::ToolPreset, m_Name + "/" + fileName + ".json");
		data = nlohmann::ordered_json::parse(stringData);
	}

	SerializeArchive archive = SerializeArchive(serializationType, &data);
	Serialize(archive);

	if (serializationType == SerializeArchiveType::SAVE_DATA)
	{
		FileIO::Write(FileIO::ToolPreset, m_Name + "/" + fileName + ".json", data.dump(4));
	}
}