#pragma once

#include "TypeDefs.h"

namespace Ball
{

	enum class ShaderParameter
	{
		UAV, // register (u...) - read write buffers and textures
		CBV, // register (b...) - constant struct 256 byte aligned
		SRV // register (t...) - read buffers and textures
	};

	class ShaderLayout
	{
	public:
		ShaderLayout() = default;

		void Initialize();

		// Adds a UAV / CBV / SRV to the layout
		void AddParameter(ShaderParameter type);
		// Adds N constants - in a shader they are represented by a single constant buffer (b...)
		void Add32bitConstParameter(int num32bit = 1);

		// Add multiple at the same time, for instance [UAV, SRV, SRV]
		// If adding more than one make sure the Shader Parameter is a valid array
		// ToDo, remake it using std::vector
		void AddParameters(ShaderParameter* type, int num);

		GPUShaderLayoutHandle GetShaderLayoutHandle() const { return m_ShaderLayout; }
		GPUShaderLayoutHandle& GetShaderLayoutHandleRef() { return m_ShaderLayout; }

	private:
		GPUShaderLayoutHandle m_ShaderLayout;
		uint32_t m_NumCBV = 0;
		uint32_t m_NumSRV = 0;
		uint32_t m_NumUAV = 0;
	};

} // namespace Ball
