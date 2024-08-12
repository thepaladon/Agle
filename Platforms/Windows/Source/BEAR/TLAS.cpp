#include "Rendering/BEAR/TLAS.h"

#include "Helpers/TopLevelASGenerator.h"
#include "Helpers/DXHelperFunctions.h"
#include "DX12GlobalVariables.h"

namespace Ball
{
	TLAS::TLAS(const std::vector<TlasInstanceData*>& levelData)
	{
		m_LevelData = levelData;
		m_TLAS.m_TopLevelASGenerator.ClearInstances();
		// Gather all the instances into the builder helper
		for (size_t i = 0; i < m_LevelData.size(); i++)
		{
			m_TLAS.m_TopLevelASGenerator.AddInstance(
				m_LevelData[i]->m_Blas->GetBLASRef().m_Result.Get(),
				m_LevelData[i]->m_Transform,
				static_cast<UINT>(m_LevelData[i]->m_ModelId),
				// Hit group id refers to the order in which we added Hit Groups to SBT
				static_cast<UINT>(i));
		}
		UINT64 scratchSize, resultSize, instanceDescsSize;
		m_TLAS.m_TopLevelASGenerator.ComputeASBufferSizes(
			GlobalDX12::g_Device.Get(), true, &scratchSize, &resultSize, &instanceDescsSize);
		m_TLAS.m_Scratch = Helpers::CreateBuffer(scratchSize,
												 D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
												 D3D12_RESOURCE_STATE_COMMON,
												 Helpers::kDefaultHeapProps);
		m_TLAS.m_Result = Helpers::CreateBuffer(resultSize,
												D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
												D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
												Helpers::kDefaultHeapProps);
		m_TLAS.m_InstanceDesc = Helpers::CreateBuffer(
			instanceDescsSize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, Helpers::kUploadHeapProps);

		// After all the buffers are allocated, or if only an update is required, we
		// can build the acceleration structure. Note that in the case of the update
		// we also pass the existing AS as the 'previous' AS, so that it can be
		// refitted in place.
		m_TLAS.m_TopLevelASGenerator.Generate(GlobalDX12::g_DirectCommandList,
											  m_TLAS.m_Scratch.Get(),
											  m_TLAS.m_Result.Get(),
											  m_TLAS.m_InstanceDesc.Get());
	}

	TLAS::~TLAS()
	{
		for (int i = 0; i < m_LevelData.size(); i++)
		{
			delete m_LevelData[i];
		}
		m_LevelData.clear();
	}

	void TLAS::SetInstanceTransform(const glm::mat4& newTransform, const uint32_t id)
	{
		assert(id < m_LevelData.size() && "ID out of bounds");
		m_LevelData[id]->m_Transform = newTransform;
	}

	glm::mat4& TLAS::GetInstanceTransformRef(const uint32_t id) const
	{
		assert(id < m_LevelData.size() && "ID out of bounds");
		return m_LevelData[id]->m_Transform;
	}

	void TLAS::Update()
	{
		m_TLAS.m_TopLevelASGenerator.Generate(GlobalDX12::g_DirectCommandList,
											  m_TLAS.m_Scratch.Get(),
											  m_TLAS.m_Result.Get(),
											  m_TLAS.m_InstanceDesc.Get());
	}
} // namespace Ball