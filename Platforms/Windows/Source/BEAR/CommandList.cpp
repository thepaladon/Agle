#include "Rendering/BEAR/CommandList.h"
#include "Rendering/BEAR/ResourceDescriptorHeap.h"
#include "Rendering/BEAR/SamplerDescriptorHeap.h"
#include "Rendering/BEAR/ComputePipelineDescription.h"
#include "Rendering/BEAR/Buffer.h"
#include "Rendering/BEAR/Texture.h"
#include "Rendering/BEAR/TLAS.h"
#include "DX12GlobalVariables.h"
#include "Helpers/DXHelperFunctions.h"
#include "Utilities/RenderUtilities.h"
#include <cassert>
#include <Helpers/TempAssert.h>
#include <Helpers/CommandQueue.h>

namespace Ball
{
	CommandList::~CommandList()
	{
	}

	void CommandList::Initialize(Texture* refIntermediateRt)
	{
		m_CmdListHandle.m_CommandList = GlobalDX12::g_DirectCommandQueue.get()->GetCommandList();
		GlobalDX12::g_DirectCommandList = m_CmdListHandle.m_CommandList;
		m_RefIntermediateRt = refIntermediateRt;
	}

	void CommandList::Destroy()
	{
		assert(false);
	}

	void CommandList::SetDescriptorHeaps(ResourceDescriptorHeap* heapR, SamplerDescriptorHeap* heapS)
	{
		std::vector<ID3D12DescriptorHeap*> heaps;
		if (heapR != nullptr)
			heaps.push_back(heapR->GetDescriptorHeapHandleRef().m_Heap.Get());
		if (heapS != nullptr)
			heaps.push_back(heapS->GetDescriptorHeapHandleRef().m_Heap.Get());
		if (heaps.size() > 0)
			m_CmdListHandle.m_CommandList->SetDescriptorHeaps(static_cast<UINT>(heaps.size()), heaps.data());
	}

	void CommandList::BindResource32BitConstants(const uint32_t layoutLocation, const void* data, const uint32_t num)
	{
		m_CmdListHandle.m_CommandList->SetComputeRoot32BitConstants(layoutLocation, num, data, 0);
	}
	void CommandList::BindResourceCBV(const uint32_t layoutLocation, Buffer& buffer)
	{
		Helpers::TransitionResourceState(&buffer, D3D12_RESOURCE_STATE_GENERIC_READ);
		m_CmdListHandle.m_CommandList->SetComputeRootConstantBufferView(
			layoutLocation, buffer.GetGPUHandleRef().m_Buffer.Get()->GetGPUVirtualAddress());
	}

	void CommandList::BindResourceSRV(const uint32_t layoutLocation, Buffer& buffer)
	{
		Helpers::TransitionResourceState(&buffer, D3D12_RESOURCE_STATE_GENERIC_READ);
		m_CmdListHandle.m_CommandList->SetComputeRootShaderResourceView(
			layoutLocation, buffer.GetGPUHandleRef().m_Buffer.Get()->GetGPUVirtualAddress());
	}
	void CommandList::BindResourceSRV(const uint32_t layoutLocation, Texture& texture)
	{
		Helpers::TransitionResourceState(&texture, D3D12_RESOURCE_STATE_GENERIC_READ);
		m_CmdListHandle.m_CommandList->SetComputeRootShaderResourceView(
			layoutLocation, texture.GetGPUHandleRef().m_Texture.Get()->GetGPUVirtualAddress());
	}

	void CommandList::BindResourceSRV(const uint32_t layoutLocation, TLAS& tlas)
	{
		m_CmdListHandle.m_CommandList->SetComputeRootShaderResourceView(
			layoutLocation, tlas.GetTLASRef().m_Result.Get()->GetGPUVirtualAddress());
	}
	void CommandList::CopyResource(Buffer& bufferDst, Buffer& bufferSrc)
	{
		assert(bufferDst.GetSizeBytes() == bufferSrc.GetSizeBytes() &&
			   "Copy resources need to have the same size in bytes!");
		Helpers::TransitionResourceState(&bufferDst, D3D12_RESOURCE_STATE_COPY_DEST);
		Helpers::TransitionResourceState(&bufferSrc, D3D12_RESOURCE_STATE_COPY_SOURCE);
		m_CmdListHandle.m_CommandList->CopyResource(bufferDst.GetGPUHandleRef().m_Buffer.Get(),
													bufferSrc.GetGPUHandleRef().m_Buffer.Get());
	}
	void CommandList::CopyResource(Texture& textureDst, Texture& textureSrc)
	{
		assert((textureDst.GetHeight() == textureSrc.GetHeight() && textureDst.GetWidth() == textureSrc.GetWidth() &&
				textureDst.GetFormat() == textureSrc.GetFormat()) &&
			   "Copy resources need to have the same dimensions and formats!");
		Helpers::TransitionResourceState(&textureDst, D3D12_RESOURCE_STATE_COPY_DEST);
		Helpers::TransitionResourceState(&textureSrc, D3D12_RESOURCE_STATE_COPY_SOURCE);
		m_CmdListHandle.m_CommandList->CopyResource(textureDst.GetGPUHandleRef().m_Texture.Get(),
													textureSrc.GetGPUHandleRef().m_Texture.Get());
	}
	void CommandList::BindResourceUAV(const uint32_t layoutLocation, Buffer& buffer)
	{
		Helpers::TransitionResourceState(&buffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		m_CmdListHandle.m_CommandList->SetComputeRootUnorderedAccessView(
			layoutLocation, buffer.GetGPUHandleRef().m_Buffer.Get()->GetGPUVirtualAddress());
	}

	void CommandList::BindResourceUAV(const uint32_t layoutLocation, Texture& texture)
	{
		Helpers::TransitionResourceState(&texture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		m_CmdListHandle.m_CommandList->SetComputeRootUnorderedAccessView(
			layoutLocation, texture.GetGPUHandleRef().m_Texture.Get()->GetGPUVirtualAddress());
	}

	void CommandList::SetComputePipeline(ComputePipelineDescription& cpd)
	{
		Utilities::PushGPUMarker(this, cpd.GetShaderName());
		m_CmdListHandle.m_CommandList->SetComputeRootSignature(
			cpd.GetShaderLayout().GetShaderLayoutHandle().m_RootSignature.Get());
		m_CmdListHandle.m_CommandList->SetPipelineState(cpd.GetPipelineHandleRef().m_ComputePSO.Get());
	}

	void CommandList::Reset()
	{
		// Reset Cmd List
		ThrowIfFailed(GlobalDX12::g_CommandAllocator->Reset());
		ThrowIfFailed(GlobalDX12::g_DirectCommandList->Reset(GlobalDX12::g_CommandAllocator.Get(), nullptr));
	}

	void CommandList::Dispatch(const uint32_t numThreadGroupsX, const uint32_t numThreadGroupsY,
							   const uint32_t numThreadGroupsZ, bool syncBeforeDispatch)
	{
		if (syncBeforeDispatch)
		{
			// Simulate a Resource Barrier for the Output Texture to do a force sync
			D3D12_RESOURCE_BARRIER uavBarrier = {};
			uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
			uavBarrier.UAV.pResource = m_RefIntermediateRt->GetGPUHandleRef().m_Texture.Get(); // The UAV being shared
			m_CmdListHandle.m_CommandList->ResourceBarrier(1, &uavBarrier);
		}

		m_CmdListHandle.m_CommandList->Dispatch(numThreadGroupsX, numThreadGroupsY, numThreadGroupsZ);
		Utilities::PopGPUMarker(this);
	}

	void CommandList::Execute()
	{
		ThrowIfFailed(m_CmdListHandle.m_CommandList->Close());
		ID3D12CommandList* ppCommandLists[] = {m_CmdListHandle.m_CommandList.Get()};
		GlobalDX12::g_DirectCommandQueue->GetD3D12CommandQueue()->ExecuteCommandLists(_countof(ppCommandLists),
																					  ppCommandLists);
	}
} // namespace Ball
