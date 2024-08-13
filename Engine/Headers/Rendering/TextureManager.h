#pragma once
#include <cstdint>

#include "Shaders/ShaderHeaders/GpuModelStruct.h"

namespace tinygltf
{
	struct Primitive;
}

namespace Ball
{
	class Primitive
	{
	public:
		Primitive(const tinygltf::Primitive& primitive);

		uint32_t GetPositionIndex() const { return m_Data.m_PositionIndex; }
		uint32_t GetIndexBufferIndex() const { return m_Data.m_IndexBufferId; }
		uint32_t GetMaterialIndex() const { return m_Data.m_MaterialIndex; }

		void SetMatrix(glm::mat4 mat) { m_Data.m_Model = mat; }
		glm::mat4 GetMatrix() const { return m_Data.m_Model; }

	private:
		PrimitiveGPU m_Data;
	};

} // namespace Ball