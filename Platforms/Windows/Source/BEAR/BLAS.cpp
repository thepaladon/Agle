#include "Rendering/BEAR/BLAS.h"
#include "Helpers/BottomLevelASGenerator.h"
#include "Helpers/DXHelperFunctions.h"
#include "DX12GlobalVariables.h"
#include <Helpers/CommandQueue.h>

namespace Ball
{
	BLAS::BLAS(const std::vector<BLASPrimitive*>& data, BlasQuality quality, const std::string& name)
	{
		m_ModelData = data;

		// Transform Buffer creation
		m_BLASHandle.m_TransformBuffer = Helpers::CreateBuffer(sizeof(glm::mat4) * data.size(),
															   D3D12_RESOURCE_FLAG_NONE,
															   D3D12_RESOURCE_STATE_COMMON,
															   Helpers::kUploadHeapProps);
		// Fill in the transforms buffer
		CD3DX12_RANGE readRange(0, 0);
		UINT8* pUploadBegin;
		ThrowIfFailed(m_BLASHandle.m_TransformBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pUploadBegin)));

		for (size_t i = 0; i < data.size(); i++)
		{
			size_t offset = i * sizeof(glm::mat4);
			glm::mat4 mat = glm::transpose(data[i]->m_ModelMatrix);
			memcpy(pUploadBegin + offset, &mat, sizeof(glm::mat4));
		}
		m_BLASHandle.m_TransformBuffer->Unmap(0, nullptr);

		// Add blas primitives
		for (size_t i = 0; i < data.size(); i++)
		{
			size_t offset = i * sizeof(glm::mat4);
			// Add a new primitive
			m_BLASHandle.m_BottomLevelASGenerator.AddVertexBuffer(
				data[i]->m_VertexBuffer->GetGPUHandleRef().m_Buffer.Get(),
				0,
				data[i]->m_VertexBuffer->GetNumElements(),
				data[i]->m_VertexBuffer->GetStride(),
				data[i]->m_IndexBuffer->GetGPUHandleRef().m_Buffer.Get(),
				0,
				data[i]->m_IndexBuffer->GetNumElements(),
				m_BLASHandle.m_TransformBuffer.Get(),
				offset,
				true);
		}

		// Create BLAS
		UINT64 scratchSizeInBytes = 0;
		UINT64 resultSizeInBytes = 0;
		m_BLASHandle.m_BottomLevelASGenerator.ComputeASBufferSizes(
			GlobalDX12::g_Device.Get(), false, &scratchSizeInBytes, &resultSizeInBytes);
		m_BLASHandle.m_Scratch = Helpers::CreateBuffer(scratchSizeInBytes,
													   D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
													   D3D12_RESOURCE_STATE_COMMON,
													   Helpers::kDefaultHeapProps);
		m_BLASHandle.m_Result = Helpers::CreateBuffer(resultSizeInBytes,
													  D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
													  D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
													  Helpers::kDefaultHeapProps);

		m_BLASHandle.m_BottomLevelASGenerator.Generate(
			GlobalDX12::g_DirectCommandList.Get(), m_BLASHandle.m_Scratch.Get(), m_BLASHandle.m_Result.Get());
	}

	BLAS::~BLAS()
	{
		for (int i = 0; i < m_ModelData.size(); i++)
		{
			delete m_ModelData[i];
		}
	}
	void BLAS::Update()
	{
		// Fill in the transforms buffer
		CD3DX12_RANGE readRange(0, 0);
		UINT8* pUploadBegin;
		ThrowIfFailed(m_BLASHandle.m_TransformBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pUploadBegin)));

		for (size_t i = 0; i < m_ModelData.size(); i++)
		{
			size_t offset = i * sizeof(glm::mat4);
			glm::mat4 mat = glm::transpose(m_ModelData[i]->m_ModelMatrix);
			memcpy(pUploadBegin + offset, &mat, sizeof(glm::mat4));
		}

		m_BLASHandle.m_TransformBuffer->Unmap(0, nullptr);

		// Add blas primitives
		for (size_t i = 0; i < m_ModelData.size(); i++)
		{
			size_t offset = i * sizeof(glm::mat4);
			// Add a new primitive
			m_BLASHandle.m_BottomLevelASGenerator.UpdateTransform(i, m_BLASHandle.m_TransformBuffer.Get(), offset);
		}

		m_BLASHandle.m_BottomLevelASGenerator.Generate(
			GlobalDX12::g_DirectCommandList.Get(), m_BLASHandle.m_Scratch.Get(), m_BLASHandle.m_Result.Get());
	}
} // namespace Ball