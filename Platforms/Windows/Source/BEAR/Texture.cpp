#include "Rendering/BEAR/Texture.h"

#include "Helpers/DXHelperFunctions.h"
#include "Helpers/CommandQueue.h"
#include "DX12GlobalVariables.h"
#include <D3D12/d3dx12.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cassert>
#include <codecvt>
#include <cstdint>
#include "Engine.h"
#include "Rendering/Renderer.h"
#include "Log.h"
#include <stb/stb_image.h>
#include <mutex>

namespace Ball
{
	std::mutex texMut;
	DXGI_FORMAT GetDXFormat(TextureFormat format);

	uint32_t GetBytesPerChannelFromFormat(TextureFormat format);

	uint32_t GetNumChannelsFromFormat(TextureFormat format);

	Texture::Texture(const void* data, TextureSpec spec, const std::string& name) : m_Spec(spec), m_Name(name)
	{
		LoadTextureData(data, name);
	}

	Texture::Texture(const std::string& path, TextureSpec spec, const std::string& name) : m_Spec(spec), m_Name(name)
	{
		ASSERT_MSG(LOG_GRAPHICS, FileIO::Exist(FileIO::Engine, path), "Texture path doesn't exist: '%s'", path.c_str());
		std::string texturepath = FileIO::GetPath(FileIO::Engine, path);

		int width, height, channels;
		auto* data = stbi_loadf(texturepath.c_str(), &width, &height, &channels, 4);

		m_Spec.m_Width = width;
		m_Spec.m_Height = height;

		if (!data)
			ASSERT_MSG(LOG_GRAPHICS, false, "stbi_load() failed in Texture constructor for %s", path.c_str());

		LoadTextureData(data, name);
		stbi_image_free(data);
	}

	void Texture::LoadTextureData(const void* data, const std::string& name)
	{
		DXGI_FORMAT createFormat = GetDXFormat(m_Spec.m_Format);
		m_BytesPerChannel = GetBytesPerChannelFromFormat(m_Spec.m_Format);
		m_Channels = GetNumChannelsFromFormat(m_Spec.m_Format);

		m_TextureHandle.m_State = D3D12_RESOURCE_STATE_COMMON;
		D3D12_RESOURCE_FLAGS createFlags = D3D12_RESOURCE_FLAG_NONE;
		D3D12_HEAP_PROPERTIES heapProps = Helpers::kDefaultHeapProps;

		if (((m_Spec.m_Flags & TextureFlags::ALLOW_UA) != TextureFlags::NONE) ||
			(m_Spec.m_Flags & TextureFlags::MIPMAP_GENERATE) != TextureFlags::NONE)
		{
			createFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}

		uint32_t MipsNum = 1;
		if ((m_Spec.m_Flags & TextureFlags::MIPMAP_GENERATE) != TextureFlags::NONE)
		{
			MipsNum = CalculateMipsNum();
		}

		// Allocates memory
		m_TextureHandle.m_Texture = Helpers::CreateTextureBuffer(
			m_Spec.m_Width, m_Spec.m_Height, MipsNum, createFormat, createFlags, m_TextureHandle.m_State, heapProps);
		const uint32_t rowPitch = m_Spec.m_Width * m_Channels * m_BytesPerChannel;
		const uint32_t alignedRowPitch = Utilities::AlignToClosestUpper(rowPitch, 256);
		m_SizeInBytes = rowPitch * m_Spec.m_Height;
		const uint32_t alignedSize = alignedRowPitch * m_Spec.m_Height;
		m_TextureHandle.m_Uploader = Helpers::CreateBuffer(
			alignedSize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, Helpers::kUploadHeapProps);

		if (data != nullptr)
		{
			// Fill in data
			CD3DX12_RANGE readRange(0, 0);
			UINT8* pUploadBegin;
			ThrowIfFailed(m_TextureHandle.m_Uploader->Map(0, &readRange, reinterpret_cast<void**>(&pUploadBegin)));
			memcpy(pUploadBegin, data, m_SizeInBytes);
			m_TextureHandle.m_Uploader->Unmap(0, nullptr);

			{
				std::lock_guard<std::mutex> lg(texMut);
				Helpers::TransitionResourceState(this, D3D12_RESOURCE_STATE_COPY_DEST);
				D3D12_SUBRESOURCE_DATA textureData = {};

				textureData.pData = data;
				textureData.RowPitch = alignedRowPitch;
				textureData.SlicePitch = m_SizeInBytes;
				UpdateSubresources(GlobalDX12::g_DirectCommandList.Get(),
								   m_TextureHandle.m_Texture.Get(),
								   m_TextureHandle.m_Uploader.Get(),
								   0, // Intermediate upload heap offset
								   0, // First subresource
								   1, // Number of subresources
								   &textureData);
				Helpers::TransitionResourceState(this, D3D12_RESOURCE_STATE_COMMON);
			}
		}

		m_TextureHandle.m_Texture->SetPrivateData(WKPDID_D3DDebugObjectName, name.size(), name.c_str());
		m_TextureHandle.m_Texture->SetName(Helpers::ConvertToWString(name).c_str());

		if ((m_Spec.m_Flags & TextureFlags::SCREENSIZE) != TextureFlags::NONE)
		{
			GetEngine().GetRenderer().AddScreensize(this);
		}
	}

	void Texture::UpdateData(const void* data)
	{
		// Fill in data
		if (data != nullptr)
		{
			// Fill in data
			CD3DX12_RANGE readRange(0, 0);
			UINT8* pUploadBegin;
			ThrowIfFailed(m_TextureHandle.m_Uploader->Map(0, &readRange, reinterpret_cast<void**>(&pUploadBegin)));

			const uint32_t alignedSize = GetAlignedSize();
			const uint32_t alignedRowPitch = alignedSize / m_Spec.m_Height;
			// Allocate a buffer for aligned data
			std::vector<uint8_t> alignedBuffer(alignedSize);
			const uint8_t* srcData = static_cast<const uint8_t*>(data);
			// Copy data from pixelData to alignedBuffer with padding
			for (uint32_t row = 0; row < m_Spec.m_Height; ++row)
			{
				std::memcpy(&alignedBuffer[row * alignedRowPitch],
							&srcData[row * m_Spec.m_Width * m_Channels * m_BytesPerChannel],
							m_Spec.m_Width * m_Channels * m_BytesPerChannel);
			}
			std::memcpy(pUploadBegin, alignedBuffer.data(), alignedBuffer.size());

			m_TextureHandle.m_Uploader->Unmap(0, nullptr);
			Helpers::TransitionResourceState(this, D3D12_RESOURCE_STATE_COPY_DEST);

			D3D12_SUBRESOURCE_DATA textureData = {};
			textureData.pData = alignedBuffer.data();
			textureData.RowPitch = alignedRowPitch;
			textureData.SlicePitch = m_SizeInBytes;
			// Perform the update
			UpdateSubresources(GlobalDX12::g_DirectCommandList.Get(),
							   m_TextureHandle.m_Texture.Get(),
							   m_TextureHandle.m_Uploader.Get(),
							   0, // Intermediate upload heap offset
							   0, // First subresource
							   1, // Number of subresources
							   &textureData);

			Helpers::TransitionResourceState(this, D3D12_RESOURCE_STATE_COMMON);
		}
	}

	void Texture::RequestDataOnCPU()
	{
		const auto& desc = m_TextureHandle.m_Texture->GetDesc();
		D3D12_RESOURCE_DESC bufferDesc = {};
		bufferDesc.Alignment = 0;
		bufferDesc.DepthOrArraySize = 1;
		bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
		bufferDesc.Height = 1;
		bufferDesc.Width = GetAlignedSize();

		bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		bufferDesc.MipLevels = 1;
		bufferDesc.SampleDesc.Count = 1;
		bufferDesc.SampleDesc.Quality = 0;

		const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_READBACK);

		// Setup readbackBuffer similar to gpuTexture but with D3D12_HEAP_TYPE_READBACK
		GlobalDX12::g_Device->CreateCommittedResource(&heapProperties,
													  D3D12_HEAP_FLAG_NONE,
													  &bufferDesc,
													  D3D12_RESOURCE_STATE_COPY_DEST,
													  nullptr,
													  IID_PPV_ARGS(&GlobalDX12::g_ScreenshotReadbackBuffer));

		Helpers::TransitionResourceState(this, D3D12_RESOURCE_STATE_COPY_SOURCE);

		D3D12_TEXTURE_COPY_LOCATION srcLocation;
		srcLocation.pResource = m_TextureHandle.m_Texture.Get();
		srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		srcLocation.SubresourceIndex = 0;

		D3D12_TEXTURE_COPY_LOCATION dstLocation;
		dstLocation.pResource = GlobalDX12::g_ScreenshotReadbackBuffer.Get();
		dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		GlobalDX12::g_Device->GetCopyableFootprints(
			&desc, 0, 1, 0, &dstLocation.PlacedFootprint, nullptr, nullptr, nullptr);

		GlobalDX12::g_DirectCommandList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);

		Helpers::TransitionResourceState(this, D3D12_RESOURCE_STATE_COMMON);
	}

	void* Texture::GetDataOnCPU()
	{
		const D3D12_RANGE readRange = {0, GetAlignedSize()}; // Assuming a 4 byte per pixel format
		BYTE* pData;
		ThrowIfFailed(GlobalDX12::g_ScreenshotReadbackBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pData)));

		// Now pData points to the texture data
		return pData;
	}

	void Texture::ReleaseDataOnCPU()
	{
		GlobalDX12::g_ScreenshotReadbackBuffer->Unmap(0, nullptr);
		GlobalDX12::g_ScreenshotReadbackBuffer.ReleaseAndGetAddressOf();
	}

	void Texture::Resize(int newWidth, int newHeight)
	{
		m_TextureHandle.m_Texture.Reset();
		m_TextureHandle.m_Uploader.Reset();
		m_Spec.m_Width = newWidth;
		m_Spec.m_Height = newHeight;
		const uint32_t rowPitch = m_Spec.m_Width * m_Channels * m_BytesPerChannel;
		m_SizeInBytes = rowPitch * m_Spec.m_Height;

		// Render Targets are rectreated by a Swap Chain
		if (m_Spec.m_Type != TextureType::RENDER_TARGET)
		{
			DXGI_FORMAT createFormat = GetDXFormat(m_Spec.m_Format);

			m_TextureHandle.m_State = D3D12_RESOURCE_STATE_COMMON;
			D3D12_RESOURCE_FLAGS createFlags = D3D12_RESOURCE_FLAG_NONE;
			D3D12_HEAP_PROPERTIES heapProps = Helpers::kDefaultHeapProps;
			if ((m_Spec.m_Flags & TextureFlags::ALLOW_UA) != TextureFlags::NONE)
			{
				createFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
			}

			// Allocates memory
			m_TextureHandle.m_Texture = Helpers::CreateTextureBuffer(
				m_Spec.m_Width, m_Spec.m_Height, 1, createFormat, createFlags, m_TextureHandle.m_State, heapProps);
			m_TextureHandle.m_Uploader = Helpers::CreateBuffer(GetAlignedSize(),
															   D3D12_RESOURCE_FLAG_NONE,
															   D3D12_RESOURCE_STATE_GENERIC_READ,
															   Helpers::kUploadHeapProps);
		}
	}

	DXGI_FORMAT GetDXFormat(TextureFormat format)
	{
		DXGI_FORMAT createFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		switch (format)
		{
		case (TextureFormat::R8G8B8A8_UNORM):
			createFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
			break;
		case (TextureFormat::R8G8B8A8_SNORM):
			createFormat = DXGI_FORMAT_R8G8B8A8_SNORM;
			break;
		case (TextureFormat::R32_FLOAT):
			createFormat = DXGI_FORMAT_R32_FLOAT;
			break;
		case (TextureFormat::R32_G32_FLOAT):
			createFormat = DXGI_FORMAT_R32G32_FLOAT;
			break;
		case (TextureFormat::R32_G32_B32_A32_FLOAT):
			createFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
			break;
		case TextureFormat::R16G16B16A16_UNORM:
			createFormat = DXGI_FORMAT_R16G16B16A16_UNORM;
			break;
		default:
			ASSERT_MSG(LOG_GRAPHICS, false, "Unexpected TextureFormat");
			break;
		}
		return createFormat;
	}

	uint32_t GetBytesPerChannelFromFormat(TextureFormat format)
	{
		uint32_t bytes = 1;
		switch (format)
		{
		case (TextureFormat::R8G8B8A8_UNORM):
			bytes = 1;
			break;
		case (TextureFormat::R8G8B8A8_SNORM):
			bytes = 1;
			break;
		case (TextureFormat::R32_FLOAT):
			bytes = 4;
			break;
		case (TextureFormat::R32_G32_FLOAT):
			bytes = 4;
			break;
		case (TextureFormat::R32_G32_B32_A32_FLOAT):
			bytes = 4;
			break;
		case TextureFormat::R16G16B16A16_UNORM:
			bytes = 2;
			break;
		default:
			ASSERT_MSG(LOG_GRAPHICS, false, "Unexpected TextureFormat");
			break;
		}
		return bytes;
	}

	uint32_t GetNumChannelsFromFormat(TextureFormat format)
	{
		uint32_t numChannels = 4;
		switch (format)
		{
		case (TextureFormat::R8G8B8A8_UNORM):
			numChannels = 4;
			break;
		case (TextureFormat::R8G8B8A8_SNORM):
			numChannels = 4;
			break;
		case (TextureFormat::R32_FLOAT):
			numChannels = 1;
			break;
		case (TextureFormat::R32_G32_FLOAT):
			numChannels = 2;
			break;
		case (TextureFormat::R32_G32_B32_A32_FLOAT):
			numChannels = 4;
			break;
		case TextureFormat::R16G16B16A16_UNORM:
			numChannels = 4;
			break;

		default:
			ASSERT_MSG(LOG_GRAPHICS, false, "Unexpected TextureFormat");
			break;
		}
		return numChannels;
	}

	void Texture::GenerateMips()
	{
		if (!m_MipsGenerated)
		{
			m_MipsGenerated = true;
			// Texture to the proper state
			Helpers::TransitionResourceState(this, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

			uint32_t numMips = m_TextureHandle.m_Texture->GetDesc().MipLevels;

			// Set handles to the heap start
			D3D12_GPU_DESCRIPTOR_HANDLE uavHandleGPU =
				GlobalDX12::g_MipMapHeap.Get()->GetGPUDescriptorHandleForHeapStart();
			D3D12_CPU_DESCRIPTOR_HANDLE uavHandleCPU =
				GlobalDX12::g_MipMapHeap.Get()->GetCPUDescriptorHandleForHeapStart();

			// Create UAVs for each Mip
			std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> mipUAVs;
			for (int i = 0; i < numMips; i++)
			{
				D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
				uavDesc.Format = m_TextureHandle.m_Texture->GetDesc().Format;
				uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
				uavDesc.Texture2D.MipSlice = i;
				GlobalDX12::g_Device->CreateUnorderedAccessView(
					m_TextureHandle.m_Texture.Get(), nullptr, &uavDesc, uavHandleCPU);

				uavHandleCPU.ptr += GlobalDX12::g_CBV_SRV_UAVDescSize;
				mipUAVs.push_back(uavHandleGPU);
				uavHandleGPU.ptr += GlobalDX12::g_CBV_SRV_UAVDescSize;
			}

			// Setup Mip Map pipeline
			GlobalDX12::g_DirectCommandList->SetComputeRootSignature(GlobalDX12::g_MipMapRootSignature.Get());
			GlobalDX12::g_DirectCommandList->SetPipelineState(GlobalDX12::g_MipMapPSO.Get());
			std::vector<ID3D12DescriptorHeap*> heaps = {GlobalDX12::g_MipMapHeap.Get()};
			GlobalDX12::g_DirectCommandList->SetDescriptorHeaps(static_cast<UINT>(heaps.size()), heaps.data());

			for (int i = 0; i < numMips - 1; i++)
			{
				// Source
				GlobalDX12::g_DirectCommandList->SetComputeRootDescriptorTable(0, mipUAVs[i]);
				// Destination
				GlobalDX12::g_DirectCommandList->SetComputeRootDescriptorTable(1, mipUAVs[i + 1]);
				// Calculate the size of the mip we will write to (original size / 2 ^ (i + 1))
				int mipWidth = glm::max(1, int(m_Spec.m_Width >> (i + 1)));
				int mipHeight = glm::max(1, int(m_Spec.m_Height >> (i + 1)));
				const float threadGroupSize = 16.0;
				// Calculate the number of thread groups needed to cover the texture
				int numGroupsX = int(ceil(float(mipWidth) / threadGroupSize));
				int numGroupsY = int(ceil(float(mipHeight) / threadGroupSize));

				// Downsample into next mip
				GlobalDX12::g_DirectCommandList->Dispatch(numGroupsX, numGroupsY, 1);
				// Gradually transition all levels Generic Read
				// Helpers::TransitionResourceState(this, D3D12_RESOURCE_STATE_GENERIC_READ, i);
				Helpers::TransitionSubResourceState(
					this, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ, i);
			}

			// Transition the last level to Generic Read
			Helpers::TransitionSubResourceState(
				this, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ, numMips - 1);
			// Texture to the read state, so the whole texture is with the same one
			m_TextureHandle.m_State = D3D12_RESOURCE_STATE_GENERIC_READ;

			// For textures which don't need UA, we copy the data to a new texture with no UA
			bool needToChangeUA = ((m_Spec.m_Flags & TextureFlags::ALLOW_UA) == TextureFlags::NONE);
			Microsoft::WRL::ComPtr<ID3D12Resource> newTextureResource;
			if (needToChangeUA)
			{
				// Copy data to the resource without unordered access
				DXGI_FORMAT createFormat = GetDXFormat(m_Spec.m_Format);
				D3D12_RESOURCE_FLAGS createFlags = D3D12_RESOURCE_FLAG_NONE;
				D3D12_HEAP_PROPERTIES heapProps = Helpers::kDefaultHeapProps;
				newTextureResource = Helpers::CreateTextureBuffer(m_Spec.m_Width,
																  m_Spec.m_Height,
																  numMips,
																  createFormat,
																  createFlags,
																  D3D12_RESOURCE_STATE_COPY_DEST,
																  heapProps);
				Helpers::TransitionResourceState(this, D3D12_RESOURCE_STATE_COPY_SOURCE);
				GlobalDX12::g_DirectCommandList->CopyResource(newTextureResource.Get(),
															  m_TextureHandle.m_Texture.Get());
			}

			// Execute the command list and
			ThrowIfFailed(GlobalDX12::g_DirectCommandList->Close());
			ID3D12CommandList* ppCommandLists[] = {GlobalDX12::g_DirectCommandList.Get()};
			GlobalDX12::g_DirectCommandQueue->GetD3D12CommandQueue()->ExecuteCommandLists(_countof(ppCommandLists),
																						  ppCommandLists);

			// Wait for other commands to execute
			GlobalDX12::g_DirectCommandQueue->WaitForExecution();
			ThrowIfFailed(GlobalDX12::g_CommandAllocator->Reset());
			ThrowIfFailed(GlobalDX12::g_DirectCommandList->Reset(GlobalDX12::g_CommandAllocator.Get(), nullptr));

			if (needToChangeUA)
			{
				// We release the texture which had UA and use the one which doesn't
				// We don't wnat model textures to bewritable
				m_TextureHandle.m_Texture.ReleaseAndGetAddressOf();
				m_TextureHandle.m_Texture = newTextureResource;
				m_TextureHandle.m_State = D3D12_RESOURCE_STATE_COPY_DEST;
				Helpers::TransitionResourceState(this, D3D12_RESOURCE_STATE_GENERIC_READ);
			}
		}
	}

	Texture::~Texture()
	{
		if ((m_Spec.m_Flags & TextureFlags::SCREENSIZE) != TextureFlags::NONE)
		{
			GetEngine().GetRenderer().RemoveScreensize(this);
		}

		m_TextureHandle.m_Texture.ReleaseAndGetAddressOf();
		m_TextureHandle.m_Uploader.ReleaseAndGetAddressOf();
	}
} // namespace Ball
