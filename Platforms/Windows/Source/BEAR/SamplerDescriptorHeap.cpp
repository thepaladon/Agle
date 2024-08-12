#include "Rendering/BEAR/SamplerDescriptorHeap.h"
#include "Helpers/DXHelperFunctions.h"
#include "DX12GlobalVariables.h"
#include <cassert>

namespace Ball
{
	void SamplerDescriptorHeap::Initialize(uint32_t maxNumberResources, bool fillAllPossible)
	{
		m_MaxSize = maxNumberResources;
		// For now all of the descriptors will be shader visible
		m_DescriptorHeapHandle.m_Heap =
			Helpers::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, maxNumberResources, true);
		m_DescriptorHeapHandle.m_Handle = m_DescriptorHeapHandle.m_Heap->GetCPUDescriptorHandleForHeapStart();
		m_SamplerStates.clear();
		if (fillAllPossible)
		{
			assert((36 <= m_MaxSize) && "For the default samplers to be filled in, you need at least 36 spots");
			FillDefaultSamplers();
		}
	}

	int SamplerDescriptorHeap::AddSampler(Sampler& sampler)
	{
		assert((m_NumElements < m_MaxSize) && "Sampler heap is full");
		D3D12_SAMPLER_DESC buildSamplerDesc = sampler.GetGPUHandle().m_SamplerDesc;
		GlobalDX12::g_Device->CreateSampler(&buildSamplerDesc, m_DescriptorHeapHandle.m_Handle);
		m_DescriptorHeapHandle.m_Handle.ptr += GlobalDX12::g_SamplerDescSize;
		m_NumElements++;
		m_SamplerStates.push_back(sampler.GetSamplerState());
		return m_NumElements - 1;
	}

	void SamplerDescriptorHeap::SwitchSampler(Sampler& newSampler, uint32_t heapID)
	{
		assert((heapID < m_MaxSize) && "This heapID is out of bounds");
		D3D12_CPU_DESCRIPTOR_HANDLE handle = m_DescriptorHeapHandle.m_Heap->GetCPUDescriptorHandleForHeapStart();
		handle.ptr += heapID * GlobalDX12::g_SamplerDescSize;
		D3D12_SAMPLER_DESC buildSamplerDesc = newSampler.GetGPUHandle().m_SamplerDesc;
		GlobalDX12::g_Device->CreateSampler(&buildSamplerDesc, handle);
		m_SamplerStates.at(heapID) = newSampler.GetSamplerState();
	}
} // namespace Ball
