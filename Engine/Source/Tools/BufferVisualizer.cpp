#include "Tools/BufferVisualizer.h"

#include "imgui.h"
#include "Rendering/BufferManager.h"

using namespace Ball;

bool IsFlagSet(int flags, int flag);
void RenderMultiSelectCombo(const char* label, int& selectedFlags, const std::vector<std::string>& items);

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
	static char searchField[128] = "";
	ImGui::InputText("Search", searchField, IM_ARRAYSIZE(searchField));

	// Sorting options
	static int sortOption = 0; // 0 = None, 1 = Ascending, 2 = Descending
	const char* sortItems[] = {
		"None",
		"Name (Ascending)",
		"Name (Descending)",
	};

	ImGui::Combo("Sort by", &sortOption, sortItems, IM_ARRAYSIZE(sortItems));

	const std::vector<std::string> items = {
		"NONE", "CBV", "SRV", "UAV", "ALLOW_UA", "DEFAULT_HEAP", "UPLOAD_HEAP", "VERTEX_BUFFER", "SCREENSIZE"};

	if (ImGui::BeginCombo("Filter", "Select..."))
	{
		for (int i = 0; i < items.size(); ++i)
		{
			bool isSelected = IsFlagSet(m_SelectedFlags, 1 << i);
			if (ImGui::Checkbox(items[i].c_str(), &isSelected))
			{
				if (isSelected)
					m_SelectedFlags |= (1 << i); // Set the flag
				else
					m_SelectedFlags &= ~(1 << i); // Clear the flag
			}
		}
		ImGui::EndCombo();
	}

	// Copy unordered_set to vector for sorting and filtering
	std::vector<Buffer*> nameFilteredBuffers;
	nameFilteredBuffers.reserve(BufferManager::m_Buffers.size());
	for (auto& buffer : BufferManager::m_Buffers)
	{
		// Add the buffers the correspond with the search field
		if (strlen(searchField) == 0 || buffer->GetName().find(searchField) != std::string::npos)
		{
			nameFilteredBuffers.push_back(buffer);
		}
	}

	// Copy unordered_set to vector for sorting and filtering
	std::vector<Buffer*> flagFilteredBuffers;
	flagFilteredBuffers.reserve(nameFilteredBuffers.size());
	for (auto& buffer : nameFilteredBuffers)
	{
		if (((int)buffer->GetFlags() & m_SelectedFlags) == m_SelectedFlags)
		{
			flagFilteredBuffers.push_back(buffer);
		}
	}

	// Sort the vector based on the selected option
	if (sortOption == 1)
	{
		std::sort(flagFilteredBuffers.begin(),
				  flagFilteredBuffers.end(),
				  [](Buffer* a, Buffer* b) { return a->GetName() < b->GetName(); });
	}
	else if (sortOption == 2)
	{
		std::sort(flagFilteredBuffers.begin(),
				  flagFilteredBuffers.end(),
				  [](Buffer* a, Buffer* b) { return a->GetName() > b->GetName(); });
	}

	int index = 0;

	std::unordered_set<Buffer*> toDelete;
	for (auto& buffer : flagFilteredBuffers)
	{
		std::string label = std::to_string(index) + ": " + buffer->GetName();
		bool isSelected = m_SelectedBuffers.find(buffer) != m_SelectedBuffers.end();
		if (ImGui::Selectable(label.c_str(), &isSelected))
		{
			m_SelectedBuffers.insert(buffer);

			// Clear if you've deselected a buffer
			if (!isSelected)
				toDelete.insert(buffer);
		}

		index++;
	}

	for (auto selectedBuffer : m_SelectedBuffers)
	{
		std::string name = "Name: " + selectedBuffer->GetName();

		bool open = true;
		ImGui::Begin(name.c_str(), &open);
		ImGui::Text("%s", name.c_str());
		ImGui::Text("Number of Elements: %u", selectedBuffer->GetNumElements());
		ImGui::Text("Stride: %u Bytes", selectedBuffer->GetStride());
		ImGui::Text("Size: %u Bytes", selectedBuffer->GetSizeBytes());

		// Display each flag if it is set
		BufferFlags flags = selectedBuffer->GetFlags();

		if (flags == BufferFlags::NONE)
		{
			ImGui::Text("Flags: NONE");
		}
		else
		{
			ImGui::Text("Flags:");
			if ((flags & BufferFlags::CBV) != BufferFlags::NONE)
				ImGui::BulletText("CBV");
			if ((flags & BufferFlags::SRV) != BufferFlags::NONE)
				ImGui::BulletText("SRV");
			if ((flags & BufferFlags::UAV) != BufferFlags::NONE)
				ImGui::BulletText("UAV");
			if ((flags & BufferFlags::ALLOW_UA) != BufferFlags::NONE)
				ImGui::BulletText("ALLOW_UA");
			if ((flags & BufferFlags::DEFAULT_HEAP) != BufferFlags::NONE)
				ImGui::BulletText("DEFAULT_HEAP");
			if ((flags & BufferFlags::UPLOAD_HEAP) != BufferFlags::NONE)
				ImGui::BulletText("UPLOAD_HEAP");
			if ((flags & BufferFlags::VERTEX_BUFFER) != BufferFlags::NONE)
				ImGui::BulletText("VERTEX_BUFFER");
			if ((flags & BufferFlags::SCREENSIZE) != BufferFlags::NONE)
				ImGui::BulletText("SCREENSIZE");
		}

		if (!open)
			toDelete.insert(selectedBuffer);

		ImGui::End();
	}

	// Cleanup
	for (const auto& buffer : toDelete)
	{
		m_SelectedBuffers.erase(buffer);
	}

	ImGui::End();
}

// Utility function to check if a flag is set
bool IsFlagSet(int flags, int flag)
{
	return (flags & flag) == flag;
}

// Function to render the multi-select combo box
void RenderMultiSelectCombo(const char* label, int& selectedFlags, const std::vector<std::string>& items)
{
	if (ImGui::BeginCombo(label, "Select..."))
	{
		for (int i = 0; i < items.size(); ++i)
		{
			bool isSelected = IsFlagSet(selectedFlags, 1 << i);
			if (ImGui::Checkbox(items[i].c_str(), &isSelected))
			{
				if (isSelected)
					selectedFlags |= (1 << i); // Set the flag
				else
					selectedFlags &= ~(1 << i); // Clear the flag
			}
		}
		ImGui::EndCombo();
	}
}
