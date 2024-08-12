#pragma once
#include "Shaders/ShaderHeaders/GpuModelStruct.h"

namespace tinygltf
{
	class Model;
}

namespace Ball
{
	// Struct is Used to upload data to GPU Material Buffer as well
	struct Material
	{
		Material(const tinygltf::Model& model, int index);
		Material() = delete;
		~Material() = default;

		// TODO (Would): Support more than 1 Tex Coord
		// TODO (Would): Support Alpha Modes and Cutoff
		// TODO (Would): Support "Double Sided"

		MaterialGPU m_Data;
	};

	static_assert(sizeof(Material) == sizeof(MaterialGPU),
				  "The size of Material must be equal to MaterialGPU struct on GPU");
} // namespace Ball