#include "Rendering/ModelLoading/Primitive.h"

#include <glm/ext/matrix_transform.hpp>
#include <TinyglTF/tiny_gltf.h>

namespace Ball
{

	PrimitiveGPU GetDefaultPrimitive()
	{
		PrimitiveGPU defaultPrim;

		defaultPrim.m_Model = glm::identity<glm::mat4>();
		defaultPrim.m_MaterialIndex = -1;
		defaultPrim.m_IndexBufferId = -1;
		defaultPrim.m_PositionIndex = -1;
		defaultPrim.m_TexCoordIndex = -1;
		defaultPrim.m_TangentIndex = -1;
		defaultPrim.m_NormalIndex = -1;
		defaultPrim.m_ColorIndex = -1;
		defaultPrim.padding = -1;

		return defaultPrim;
	}

	Primitive::Primitive(const tinygltf::Primitive& primitive)
	{
		// Note: This gets set during the BLAS creation step from
		// multiplying all Nodes to get into world space from vertex space

		m_Data = GetDefaultPrimitive();
		m_Data.m_MaterialIndex = primitive.material;
		m_Data.m_IndexBufferId = primitive.indices;

		[[maybe_unused]] int i = 0;
		// Buffers relevant to Material
		for (auto& attribute : primitive.attributes)
		{
			std::string attributeName = attribute.first;
			if (attributeName == "POSITION")
			{
				m_Data.m_PositionIndex = attribute.second;
			}
			else if (attributeName == "NORMAL")
			{
				m_Data.m_NormalIndex = attribute.second;
			}
			else if (attributeName == "TANGENT")
			{
				m_Data.m_TangentIndex = attribute.second;
			}
			else if (attributeName == "TEXCOORD_0")
			{
				m_Data.m_TexCoordIndex = attribute.second;
			}
			else if (attributeName == "COLOR_0")
			{
				m_Data.m_ColorIndex = attribute.second;
			}
			else
			{
				printf("LOG: [WARNING] No support for %s yet. \n", attribute.first.c_str());
			}
		}
	}
} // namespace Ball