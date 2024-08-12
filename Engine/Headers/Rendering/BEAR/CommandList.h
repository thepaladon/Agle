#pragma once
#include <TypeDefs.h>

namespace Ball
{
	class Buffer;
	class Texture;
	class ComputePipelineDescription;
	class SamplerDescriptorHeap;
	class ResourceDescriptorHeap;
	class TLAS;

	class CommandList
	{
	public:
		CommandList() = default;
		~CommandList();

		void Initialize(Texture* refIntermediateRt);
		void Destroy();

		// Binds shader with a shader layout
		void SetComputePipeline(ComputePipelineDescription& cpd);

		// Set Bindless Descriptor Heaps - these heaps will be accessible in the shader
		void SetDescriptorHeaps(ResourceDescriptorHeap* heapR,
								SamplerDescriptorHeap* heapS = nullptr); // they can be nullptr if we don't use them

		// Binders - bind data to registers, using id in the shader layout
		void BindResource32BitConstants(const uint32_t layoutLocation, const void* data, const uint32_t num = 1);
		void BindResourceCBV(const uint32_t layoutLocation, Buffer& buffer);
		void BindResourceSRV(const uint32_t layoutLocation, Buffer& buffer);
		void BindResourceSRV(const uint32_t layoutLocation, Texture& texture);
		void BindResourceSRV(const uint32_t layoutLocation, TLAS& tlas);
		void BindResourceUAV(const uint32_t layoutLocation, Buffer& buffer);
		void BindResourceUAV(const uint32_t layoutLocation, Texture& texture);

		// Copy of Resources
		void CopyResource(Buffer& bufferDst, Buffer& bufferSrc); // Buffers should have the same size
		void CopyResource(Texture& textureDst, Texture& textureSrc); // Textures should have same size and format

		// Sync Functions
		void Execute();
		void Reset();
		// Dispatches a selected number of threads to execute the binded shader (can sync all the GPU-CPU resources for
		// PS5 in prior to launch)
		void Dispatch(const uint32_t x, const uint32_t y = 1, const uint32_t z = 1, bool syncBeforeDispatch = false);

		// Getters
		GPUCommandListHandle& GetCommandListHandleRef() { return m_CmdListHandle; }

	private:
		Texture* m_RefIntermediateRt = nullptr;
		GPUCommandListHandle m_CmdListHandle;
	};

} // namespace Ball
