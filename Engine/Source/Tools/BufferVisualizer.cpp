#include "Tools/BufferVisualizer.h"

#include "imgui.h"
#include "Rendering/BufferManager.h"

using namespace Ball;

void BufferVisualizer::Init()
{
	m_Open = false;
	m_Name = "Buffer Visualizer";
	m_ToolCatagory = ToolCatagory::RESOURCES;
	m_ToolInterfaceType = ToolInterfaceType::WINDOW;
}

void BufferVisualizer::Draw()
{
	ImGui::Begin(m_Name.c_str(), &m_Open);
	static char searchBuffer[128] = "";
	ImGui::InputText("Search", searchBuffer, IM_ARRAYSIZE(searchBuffer));

	// Sorting options
	static int sortOption = 0; // 0 = None, 1 = Ascending, 2 = Descending
	ImGui::Combo("Sort by", &sortOption, "None\0Name (Ascending)\0Name (Descending)\0");

	// Copy unordered_set to vector for sorting and filtering
	std::vector<Buffer*> filteredBuffers;
	for (auto& buffer : BufferManager::m_Buffers)
	{
		if (strlen(searchBuffer) == 0 || buffer->GetName().find(searchBuffer) != std::string::npos)
		{
			filteredBuffers.push_back(buffer);
		}
	}

	// Sort the vector based on the selected option
	if (sortOption == 1)
	{
		std::sort(filteredBuffers.begin(),
				  filteredBuffers.end(),
				  [](Buffer* a, Buffer* b) { return a->GetName() < b->GetName(); });
	}
	else if (sortOption == 2)
	{
		std::sort(filteredBuffers.begin(),
				  filteredBuffers.end(),
				  [](Buffer* a, Buffer* b) { return a->GetName() > b->GetName(); });
	}

	static Buffer* selectedBuffer = nullptr;
	int index = 0;

	for (auto& buffer : filteredBuffers)
	{
		std::string label = std::to_string(index) + ": " + buffer->GetName();
		if (ImGui::Selectable(label.c_str(), buffer == selectedBuffer))
		{
			selectedBuffer = buffer;
		}
		index++;
	}

	if (selectedBuffer)
	{
		ImGui::Separator();
		ImGui::Text("Buffer Details:");
		ImGui::Text("Name: %s", selectedBuffer->GetName().c_str());
		ImGui::Text("Number of Elements: %u", selectedBuffer->GetNumElements());
		ImGui::Text("Stride: %u Bytes", selectedBuffer->GetStride());
		ImGui::Text("Size: %u Bytes", selectedBuffer->GetSizeBytes());

		// Displaying flags and handle might require custom code depending on their structures.
	}

	ImGui::End();
}
