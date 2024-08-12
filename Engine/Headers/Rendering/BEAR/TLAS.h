#pragma once
#include "BLAS.h"
#include "TypeDefs.h"
#include <glm/glm.hpp>
#include <unordered_map>

namespace Ball
{
	struct TlasInstanceData
	{
		BLAS* m_Blas;
		glm::mat4 m_Transform;
		uint32_t m_ModelId;
	};

	class TLAS
	{
	public:
		TLAS(const std::vector<TlasInstanceData*>& levelData);
		~TLAS();
		void Update();

		// Getters
		const GPUTlasDescHandle& GetTLASRef() const { return m_TLAS; }
		glm::mat4& GetInstanceTransformRef(const uint32_t id) const;

		const size_t GetNumInstances() const { return m_LevelData.size(); }
		const uint32_t GetInstanceModelId(const uint32_t id) const { return m_LevelData[id]->m_ModelId; }
		// Setters
		void SetInstanceTransform(const glm::mat4& newTransform, const uint32_t id);

	private:
		// Reference to the Models in our world so we can always
		// update the TLAS with the most relevant data.
		std::vector<TlasInstanceData*> m_LevelData;
		GPUTlasDescHandle m_TLAS;
	};
} // namespace Ball