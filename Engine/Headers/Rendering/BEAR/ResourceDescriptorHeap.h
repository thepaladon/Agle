#pragma once

#include "Log.h"
#include "TypeDefs.h"
#include "Rendering/ModelLoading/Model.h"

namespace Ball
{
	class Texture;
	class Buffer;

	class ResourceDescriptorHeap
	{
	public:
		ResourceDescriptorHeap() = delete;
		ResourceDescriptorHeap(uint32_t maxNumberResources);
		~ResourceDescriptorHeap();

		// Reserves Spot without pushing anything
		int ReserveSpace(uint32_t numSpacesToReserve = 1);

		// Pushes to the heap
		int Add(Buffer& buffer);

		// Switches a resource in the heap to the new one
		void Switch(Buffer& newBuffer, int heapID);

		// Pushes to the heap
		int Add(Texture& texture);

		// Switches a resource in the heap to the new one
		void Switch(Texture& newTexture, int heapID);

		// Get size of texture element string vector array
		int GetTextureElementCount() { return m_TextureNames.size(); }

		// Function is inside header because then we dont need two implementations for the two platforms
		// Get string of the texture by id
		std::string GetTextureElementName(int index) const
		{
			std::string textureName = std::string("null");

			// Check if index is valid
			if (index >= 0 && index < m_TextureNames.size())
			{
				textureName = m_TextureNames.at(index);
			}

			return textureName;
		}

		GPUDescriptorHeapHandle& GetDescriptorHeapHandleRef() { return m_DescriptorHeapHandle; }
		int GetNumElements() const { return m_NumElements; }

	private:
		GPUDescriptorHeapHandle m_DescriptorHeapHandle;
		int m_NumElements = 0;
		[[maybe_unused]] int m_MaxSize = 0;

		// Array that stores the names of the textures added to the heap
		std::vector<std::string> m_TextureNames;

		void ReserveSpaceCommonLogic(int i)
		{
			WARN(LOG_GRAPHICS,
				 "Resource Descriptor Entry [%i - %i] : RESERVED SPACE(s), Currently Empty",
				 m_NumElements,
				 (m_NumElements + i - 1));
			m_NumElements += i;
			m_TextureNames.insert(m_TextureNames.end(), i, std::string("Reserved Spot, Currently Empty"));
		}
	};
} // namespace Ball