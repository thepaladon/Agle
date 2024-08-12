#pragma once
#include "Buffer.h"
#include "TypeDefs.h"

#include <vector>
#include <glm/glm.hpp>

namespace Ball
{
	enum class BlasQuality
	{
		FAST_BUILD,
		FAST_TRAVERSE,
		REFIT_FAST_BUILD,
		REFIT_FAST_TRAVERSE,
	};

	struct BLASPrimitive
	{
		Buffer* m_VertexBuffer; // Vertext POSITION buffer
		Buffer* m_IndexBuffer; // UINT32 index buffer
		glm::mat4 m_ModelMatrix; // Ptimitive-to-model-space matrix
	};

	class BLAS
	{
	public:
		BLAS(const std::vector<BLASPrimitive*>& data, BlasQuality quality = BlasQuality::FAST_TRAVERSE,
			 const std::string& name = "default_blas_name");
		~BLAS();
		void Update();
		// Getter
		const GPUBlasHandle& GetBLASRef() const { return m_BLASHandle; }

	private:
		std::string m_Name;
		std::vector<BLASPrimitive*> m_ModelData;
		GPUBlasHandle m_BLASHandle;
	};
} // namespace Ball