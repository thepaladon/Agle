#include "Tools/BindlessHeapViewer.h"
#include "Tools/ToolManager.h"
#include "Rendering/Renderer.h"
#include "Rendering/BEAR/ResourceDescriptorHeap.h"
#include "Rendering/BEAR/SamplerDescriptorHeap.h"
#include "ImGui/imgui.h"
#include "Engine.h"

using namespace Ball;

void BindlessHeapViewer::Draw()
{
	ImGui::Begin(m_Name.c_str(), &m_Open);

	RenderAPI& renderer = GetEngine().GetRenderer();

	uint32_t textureCount = renderer.GetResourceDescriptorHeap()->GetTextureElementCount();
	ImGui::Text("Resource descriptor element count: %i", textureCount);
	ImGui::Checkbox("Hide buffer entries", &m_HideBufferEntires);

	ImGui::Text("Textures:");
	for (uint32_t i = 0; i < textureCount; i++)
	{
		std::string name = renderer.GetResourceDescriptorHeap()->GetTextureElementName(i).c_str();
		if (name.compare("null") != 0)
		{
			// Only print entry with name when its a texture
			// But when hideBuffer entries is false also show buffer entries
			if (name.compare("buffer") != 0 || !m_HideBufferEntires)
			{
				ImGui::Text("	[%i] %s", i, name.c_str());
			}
		}
		else
		{
			// Print invalid text when name is null
			ImGui::Text("	[%i] invalid", i);
		}
	}

	// Add spacing between lists
	float spacing = ImGui::GetCursorPosY();
	spacing += 20.0f;
	ImGui::SetCursorPosY(spacing);

	uint32_t samplerCount = renderer.GetSamplerDescriptorHeap()->GetNumElements();
	ImGui::Text("Sampler descriptor element count: %i", samplerCount);

	ImGui::Text("Samplers: ");
	for (uint32_t i = 0; i < samplerCount; i++)
	{
		SamplerState samplerState = renderer.GetSamplerDescriptorHeap()->GetSamplerState(i);
		if (samplerState.valid)
		{
			ImGui::Text("	[%i] MinFiler=%s, MagFilter=%s, WrapUV=%s",
						i,
						samplerState.GetMinFilterName().c_str(),
						samplerState.GetMapFilterName().c_str(),
						samplerState.GetWrapUVName().c_str());
		}
		else
		{
			ImGui::Text("	[%i] invalid sampler", i);
		}
	}

	ImGui::End();
}
