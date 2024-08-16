#include "Tools/TextureVisualizer.h"

#include "imgui.h"
#include "Rendering/TextureManager.h"

using namespace Ball;

void RenderMultiSelectComboA(const char* label, int& selectedFlags, const std::vector<std::string>& items);

void TextureVisualizer::Init()
{
	m_Open = false;
	m_Name = "Texture Visualizer";
	m_ToolCatagory = ToolCatagory::RESOURCES;
	m_ToolInterfaceType = ToolInterfaceType::WINDOW;
}

void TextureVisualizer::Draw()
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

	ImGui::Combo("Sort by Flags", &sortOption, sortItems, IM_ARRAYSIZE(sortItems));
	const std::vector<std::string> comboFlags = {"NONE", "MIPMAP_GENERATE", "ALLOW_UA", "SCREENSIZE"};
	RenderMultiSelectComboA("Filter", m_SelectedFlags, comboFlags);

	// Copy unordered_set to vector for sorting and filtering
	std::vector<Texture*> nameFilteredTextures;
	nameFilteredTextures.reserve(TextureManager::m_Textures.size());
	for (auto& Texture : TextureManager::m_Textures)
	{
		// Add the Textures the correspond with the search field
		if (strlen(searchField) == 0 || Texture->GetName().find(searchField) != std::string::npos)
		{
			nameFilteredTextures.push_back(Texture);
		}
	}

	// Copy unordered_set to vector for sorting and filtering
	std::vector<Texture*> flagFilteredTextures;
	flagFilteredTextures.reserve(nameFilteredTextures.size());
	for (auto& Texture : nameFilteredTextures)
	{
		if (((int)Texture->GetFlags() & m_SelectedFlags) == m_SelectedFlags)
		{
			flagFilteredTextures.push_back(Texture);
		}
	}

	// Sort the vector based on the selected option
	if (sortOption == 1)
	{
		std::sort(flagFilteredTextures.begin(),
				  flagFilteredTextures.end(),
				  [](Texture* a, Texture* b) { return a->GetName() < b->GetName(); });
	}
	else if (sortOption == 2)
	{
		std::sort(flagFilteredTextures.begin(),
				  flagFilteredTextures.end(),
				  [](Texture* a, Texture* b) { return a->GetName() > b->GetName(); });
	}

	// List all Textures as selectable items
	int index = 0;
	std::unordered_set<Texture*> toDelete;
	for (auto& Texture : flagFilteredTextures)
	{
		std::string label = std::to_string(index) + ": " + Texture->GetName();
		bool isSelected = m_SelectedTextures.find(Texture) != m_SelectedTextures.end();
		if (ImGui::Selectable(label.c_str(), &isSelected))
		{
			m_SelectedTextures.insert(Texture);

			// Clear if you've deselected a Texture
			if (!isSelected)
				toDelete.insert(Texture);
		}
		index++;
	}

	for (auto selectedTexture : m_SelectedTextures)
	{
		std::string name = "Name: " + selectedTexture->GetName();

		bool open = true;
		ImGui::Begin(name.c_str(), &open);
		ImGui::Text("%s", name.c_str());
		ImGui::Text("Type: %s", TextureManager::GetTypeAsString(selectedTexture->GetType()).c_str());
		ImGui::Text("Format: %s", TextureManager::GetFormatAsString(selectedTexture->GetFormat()).c_str());
		ImGui::Text("Width: %u", selectedTexture->GetWidth());
		ImGui::Text("Height: %u", selectedTexture->GetHeight());
		ImGui::Text("Aligned Width: %u", selectedTexture->GetAlignedWidth());
		ImGui::Text("Number of Channels: %u", selectedTexture->GetNumChannels());
		ImGui::Text("Bytes per Channel: %u", selectedTexture->GetBytesPerChannel());
		ImGui::Text("Size: %u Bytes",
					selectedTexture->GetAlignedWidth() * selectedTexture->GetHeight() *
						selectedTexture->GetBytesPerChannel() * selectedTexture->GetNumChannels());

		// Display each flag if it is set
		TextureFlags flags = selectedTexture->GetFlags();

		if (flags == TextureFlags::NONE)
		{
			ImGui::Text("Flags: NONE");
		}
		else
		{
			ImGui::Text("Flags:");
			if ((flags & TextureFlags::MIPMAP_GENERATE) != TextureFlags::NONE)
				ImGui::BulletText("MIPMAP_GENERATE");
			if ((flags & TextureFlags::ALLOW_UA) != TextureFlags::NONE)
				ImGui::BulletText("ALLOW_UA");
			if ((flags & TextureFlags::SCREENSIZE) != TextureFlags::NONE)
				ImGui::BulletText("SCREENSIZE");
		}

		if (!open)
			toDelete.insert(selectedTexture);

		ImGui::End();
	}

	// Cleanup
	for (const auto& Texture : toDelete)
	{
		m_SelectedTextures.erase(Texture);
	}

	ImGui::End();
}

// Utility function to check if a flag is set
bool IsFlagSetA(int flags, int flag)
{
	return (flags & flag) == flag;
}

// Function to render the multi-select combo box
void RenderMultiSelectComboA(const char* label, int& selectedFlags, const std::vector<std::string>& items)
{
	if (ImGui::BeginCombo(label, "Select..."))
	{
		for (int i = 0; i < items.size(); ++i)
		{
			bool isSelected = IsFlagSetA(selectedFlags, 1 << i);
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
