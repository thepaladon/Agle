#include "Rendering/BEAR/ResourceDescriptorHeap.h"
#include "Helpers/DXHelperFunctions.h"
#include "Rendering/BEAR/Buffer.h"
#include "Rendering/BEAR/Texture.h"
#include "DX12GlobalVariables.h"
#include <cassert>
#include "Engine.h"
#include "Rendering/Renderer.h"

#include "Log.h"

namespace Ball
{

	ResourceDescriptorHeap::ResourceDescriptorHeap(uint32_t maxNumberResources)
	{
		// For now all of the descriptors will be shader visible
		m_DescriptorHeapHandle.m_Heap =
			Helpers::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, maxNumberResources, true);
		m_DescriptorHeapHandle.m_Handle = m_DescriptorHeapHandle.m_Heap->GetCPUDescriptorHandleForHeapStart();
		m_TextureNames.clear();
	}

	ResourceDescriptorHeap::~ResourceDescriptorHeap()
	{
	}

	int ResourceDescriptorHeap::ReserveSpace(uint32_t numSpacesToReserve)
	{
		ReserveSpaceCommonLogic(numSpacesToReserve);

		m_DescriptorHeapHandle.m_Handle.ptr += GlobalDX12::g_CBV_SRV_UAVDescSize * numSpacesToReserve;
		return m_NumElements - 1;
	}

	int ResourceDescriptorHeap::Add(Buffer& buffer)
	{
		if ((buffer.GetFlags() & BufferFlags::CBV) != BufferFlags::NONE)
		{
			// Describe and create a constant buffer view
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = buffer.GetGPUHandleRef().m_Buffer.Get()->GetGPUVirtualAddress();
			cbvDesc.SizeInBytes = buffer.GetSizeBytes();
			GlobalDX12::g_Device->CreateConstantBufferView(&cbvDesc, m_DescriptorHeapHandle.m_Handle);
			assert((buffer.GetFlags() & BufferFlags::SRV) == BufferFlags::NONE &&
				   "Buffer can't have CBV and SRV flags");
		}
		else if ((buffer.GetFlags() & BufferFlags::UAV) != BufferFlags::NONE)
		{
			// Describe and create a unordered buffer view
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
			uavDesc.Format = DXGI_FORMAT_UNKNOWN;
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			uavDesc.Buffer.NumElements = buffer.GetNumElements();
			uavDesc.Buffer.StructureByteStride = buffer.GetStride();
			GlobalDX12::g_Device->CreateUnorderedAccessView(
				buffer.GetGPUHandleRef().m_Buffer.Get(), nullptr, &uavDesc, m_DescriptorHeapHandle.m_Handle);
			assert((buffer.GetFlags() & BufferFlags::CBV) == BufferFlags::NONE &&
				   "Buffer can't have CBV and UAV flags");
			assert((buffer.GetFlags() & BufferFlags::SRV) == BufferFlags::NONE &&
				   "Buffer can't have SRV and UAV flags");
		}
		else
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
			srvDesc.Format = DXGI_FORMAT_UNKNOWN;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Buffer.NumElements = buffer.GetNumElements();
			srvDesc.Buffer.FirstElement = 0;
			srvDesc.Buffer.StructureByteStride = buffer.GetStride();
			srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
			GlobalDX12::g_Device->CreateShaderResourceView(
				buffer.GetGPUHandleRef().m_Buffer.Get(), &srvDesc, m_DescriptorHeapHandle.m_Handle);
		}
		m_DescriptorHeapHandle.m_Handle.ptr += GlobalDX12::g_CBV_SRV_UAVDescSize;
		m_NumElements++;

		// Add buffer with placeholder text so the heapID is alligned
		m_TextureNames.push_back(std::string("buffer"));

		if ((buffer.GetFlags() & BufferFlags::SCREENSIZE) != BufferFlags::NONE)
		{
			GetEngine().GetRenderer().MakeScreensizeHeapLink({&buffer, this, m_NumElements - 1});
		}

		return m_NumElements - 1;
	}

	void ResourceDescriptorHeap::Switch(Buffer& newBuffer, int heapID)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE handle = m_DescriptorHeapHandle.m_Heap->GetCPUDescriptorHandleForHeapStart();
		handle.ptr += heapID * GlobalDX12::g_CBV_SRV_UAVDescSize;
		if ((newBuffer.GetFlags() & BufferFlags::CBV) != BufferFlags::NONE)
		{
			// Describe and create a constant buffer view
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = newBuffer.GetGPUHandleRef().m_Buffer.Get()->GetGPUVirtualAddress();
			cbvDesc.SizeInBytes = newBuffer.GetSizeBytes();
			GlobalDX12::g_Device->CreateConstantBufferView(&cbvDesc, handle);
			assert((newBuffer.GetFlags() & BufferFlags::SRV) == BufferFlags::NONE &&
				   "Buffer can't have CBV and SRV flags");
		}
		else if ((newBuffer.GetFlags() & BufferFlags::SRV) != BufferFlags::NONE)
		{
			// Describe and create a structured buffer view
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
			srvDesc.Format = DXGI_FORMAT_UNKNOWN;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Buffer.NumElements = newBuffer.GetNumElements();
			srvDesc.Buffer.FirstElement = 0;
			srvDesc.Buffer.StructureByteStride = newBuffer.GetStride();
			srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
			GlobalDX12::g_Device->CreateShaderResourceView(
				newBuffer.GetGPUHandleRef().m_Buffer.Get(), &srvDesc, handle);
			assert((newBuffer.GetFlags() & BufferFlags::CBV) == BufferFlags::NONE &&
				   "Buffer can't have CBV and SRV flags");
		}
		else
		{
			// Describe and create a unordered buffer view
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
			uavDesc.Format = DXGI_FORMAT_UNKNOWN;
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			uavDesc.Buffer.NumElements = newBuffer.GetNumElements();
			uavDesc.Buffer.StructureByteStride = newBuffer.GetStride();
			GlobalDX12::g_Device->CreateUnorderedAccessView(
				newBuffer.GetGPUHandleRef().m_Buffer.Get(), nullptr, &uavDesc, handle);
			assert((newBuffer.GetFlags() & BufferFlags::CBV) == BufferFlags::NONE &&
				   "Buffer can't have UAV and CBV flags");
			assert((newBuffer.GetFlags() & BufferFlags::SRV) == BufferFlags::NONE &&
				   "Buffer can't have UAV and SRV flags");
		}

		// Remove the resizing link from the previous texture, add to the new one
		if ((newBuffer.GetFlags() & BufferFlags::SCREENSIZE) != BufferFlags::NONE)
		{
			GetEngine().GetRenderer().RemoveScreensizeHeapLink({&newBuffer, this, heapID});
			GetEngine().GetRenderer().MakeScreensizeHeapLink({&newBuffer, this, heapID});
		}
	}

	int ResourceDescriptorHeap::Add(Texture& texture)
	{
		if (texture.GetType() == TextureType::R_TEXTURE)
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
			srvDesc.Format = texture.GetGPUHandleRef().m_Texture.Get()->GetDesc().Format;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Texture2D.MipLevels = texture.GetGPUHandleRef().m_Texture->GetDesc().MipLevels;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.PlaneSlice = 0;
			srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
			GlobalDX12::g_Device->CreateShaderResourceView(
				texture.GetGPUHandleRef().m_Texture.Get(), &srvDesc, m_DescriptorHeapHandle.m_Handle);
		}
		if (texture.GetType() == TextureType::RW_TEXTURE)
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			GlobalDX12::g_Device->CreateUnorderedAccessView(
				texture.GetGPUHandleRef().m_Texture.Get(), nullptr, &uavDesc, m_DescriptorHeapHandle.m_Handle);
		}
		m_DescriptorHeapHandle.m_Handle.ptr += GlobalDX12::g_CBV_SRV_UAVDescSize;
		m_NumElements++;

		// Add texture name to dynamic array
		m_TextureNames.push_back(texture.GetName());

		if ((texture.GetFlags() & TextureFlags::SCREENSIZE) != TextureFlags::NONE)
		{
			GetEngine().GetRenderer().MakeScreensizeHeapLink({&texture, this, m_NumElements - 1});
		}

		return m_NumElements - 1;
	}

	void ResourceDescriptorHeap::Switch(Texture& newTexture, int heapID)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE handle = m_DescriptorHeapHandle.m_Heap->GetCPUDescriptorHandleForHeapStart();
		handle.ptr += heapID * GlobalDX12::g_CBV_SRV_UAVDescSize;
		if (newTexture.GetType() == TextureType::R_TEXTURE)
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
			srvDesc.Format = newTexture.GetGPUHandleRef().m_Texture.Get()->GetDesc().Format;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Texture2D.MipLevels = newTexture.GetGPUHandleRef().m_Texture->GetDesc().MipLevels;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.PlaneSlice = 0;
			srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
			GlobalDX12::g_Device->CreateShaderResourceView(
				newTexture.GetGPUHandleRef().m_Texture.Get(), &srvDesc, handle);
		}
		if (newTexture.GetType() == TextureType::RW_TEXTURE)
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			GlobalDX12::g_Device->CreateUnorderedAccessView(
				newTexture.GetGPUHandleRef().m_Texture.Get(), nullptr, &uavDesc, handle);
		}

		// Replace name of texture at heapID index
		// Do I need to cast the int to size_t?
		m_TextureNames.at(heapID) = newTexture.GetName();

		// Remove the resizing link from the previous texture, add to the new one
		if ((newTexture.GetFlags() & TextureFlags::SCREENSIZE) != TextureFlags::NONE)
		{
			GetEngine().GetRenderer().RemoveScreensizeHeapLink({&newTexture, this, heapID});
			GetEngine().GetRenderer().MakeScreensizeHeapLink({&newTexture, this, heapID});
		}
	}

} // namespace Ball
