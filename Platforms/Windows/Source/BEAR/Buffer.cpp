#include "Rendering/BEAR/Buffer.h"
#include "Helpers/DXHelperFunctions.h"
#include "DX12GlobalVariables.h"
#include <cassert>
#include <codecvt>

#include "Engine.h"
#include "Rendering/Renderer.h"
#include <mutex>

namespace Ball
{
	std::mutex bufMut;
	Buffer::Buffer(const void* data, const uint32_t stride, const uint32_t count, BufferFlags flags,
				   const std::string& name)
	{
		ASSERT_MSG(LOG_GRAPHICS, count > 0, "Buffer must have at least 1 element");

		// Fill in variables
		m_Name = name;
		m_Stride = stride;
		m_Count = count;
		m_Flags = flags;
		uint32_t bufferSize = m_Stride * m_Count;
		// Allocate space
		m_BufferHandle.m_State = D3D12_RESOURCE_STATE_COMMON;
		D3D12_RESOURCE_FLAGS createFlags = D3D12_RESOURCE_FLAG_NONE;
		D3D12_HEAP_PROPERTIES heapProps = Helpers::kUploadHeapProps;

		if ((m_Flags & BufferFlags::ALLOW_UA) != BufferFlags::NONE)
		{
			createFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}

		if ((m_Flags & BufferFlags::DEFAULT_HEAP) != BufferFlags::NONE)
		{
			heapProps = Helpers::kDefaultHeapProps;
			assert((m_Flags & BufferFlags::UPLOAD_HEAP) == BufferFlags::NONE &&
				   "Buffer can't have default and upload flags");
		}

		m_BufferHandle.m_Buffer = Helpers::CreateBuffer(bufferSize, createFlags, m_BufferHandle.m_State, heapProps);
		m_BufferHandle.m_Buffer->SetPrivateData(WKPDID_D3DDebugObjectName, name.size(), name.c_str());
		m_BufferHandle.m_Buffer->SetName(Helpers::ConvertToWString(name).c_str());

		if (data != nullptr)
		{
			// Fill in data
			if ((m_Flags & BufferFlags::DEFAULT_HEAP) != BufferFlags::NONE)
			{
				// Note: this might be unnecessary
				if (m_BufferHandle.m_Uploader.Get() != nullptr)
					m_BufferHandle.m_Uploader.Get()->Release();

				// For default heap we need to use a staging resource as we can't write straight to it
				CD3DX12_RANGE readRange(0, 0);
				m_BufferHandle.m_Uploader = Helpers::CreateBuffer(
					bufferSize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, Helpers::kUploadHeapProps);

				UINT8* pUploadBegin;
				ThrowIfFailed(m_BufferHandle.m_Uploader->Map(0, &readRange, reinterpret_cast<void**>(&pUploadBegin)));
				memcpy(pUploadBegin, data, bufferSize);
				m_BufferHandle.m_Uploader->Unmap(0, nullptr);

				{
					std::lock_guard<std::mutex> lg(bufMut);
					Helpers::TransitionResourceState(this, D3D12_RESOURCE_STATE_COPY_DEST);
					GlobalDX12::g_DirectCommandList->CopyResource(m_BufferHandle.m_Buffer.Get(),
																  m_BufferHandle.m_Uploader.Get());
					Helpers::TransitionResourceState(this, D3D12_RESOURCE_STATE_COMMON);
				}
			}
			else if ((m_Flags & BufferFlags::UPLOAD_HEAP) != BufferFlags::NONE)
			{
				CD3DX12_RANGE readRange(0, 0);
				UINT8* pUploadBegin;
				ThrowIfFailed(m_BufferHandle.m_Buffer->Map(0, &readRange, reinterpret_cast<void**>(&pUploadBegin)));
				memcpy(pUploadBegin, data, bufferSize);
				m_BufferHandle.m_Buffer->Unmap(0, nullptr);
			}
		}

		if ((m_Flags & BufferFlags::SCREENSIZE) != BufferFlags::NONE)
		{
			GetEngine().GetRenderer().AddScreensize(this);
		}
	}

	void Buffer::UpdateData(const void* data, uint32_t dataSizeInBytes)
	{
		assert((m_Flags & BufferFlags::DEFAULT_HEAP) == BufferFlags::NONE &&
			   "Buffer in default heap shouldn't be updated");
		size_t sizeInBytes = m_Stride * m_Count;
		assert((dataSizeInBytes <= sizeInBytes) && "Update data size doesn't match buffer size");

		CD3DX12_RANGE readRange(0, 0);
		UINT8* pUploadBegin;
		ThrowIfFailed(m_BufferHandle.m_Buffer->Map(0, &readRange, reinterpret_cast<void**>(&pUploadBegin)));
		memcpy(pUploadBegin, data, sizeInBytes);
		m_BufferHandle.m_Buffer->Unmap(0, nullptr);
	}

	void Buffer::Resize(uint32_t newCount)
	{
		m_BufferHandle.m_Buffer.Reset();

		if (m_BufferHandle.m_Uploader.Get() != nullptr)
			m_BufferHandle.m_Uploader.Reset();

		m_Count = newCount;
		uint32_t bufferSize = m_Stride * m_Count;

		// Allocate space
		m_BufferHandle.m_State = D3D12_RESOURCE_STATE_COMMON;
		D3D12_RESOURCE_FLAGS createFlags = D3D12_RESOURCE_FLAG_NONE;
		D3D12_HEAP_PROPERTIES heapProps = Helpers::kUploadHeapProps;

		if ((m_Flags & BufferFlags::ALLOW_UA) != BufferFlags::NONE)
		{
			createFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}

		if ((m_Flags & BufferFlags::DEFAULT_HEAP) != BufferFlags::NONE)
		{
			heapProps = Helpers::kDefaultHeapProps;
			assert((m_Flags & BufferFlags::UPLOAD_HEAP) == BufferFlags::NONE &&
				   "Buffer can't have default and upload flags");
		}

		m_BufferHandle.m_Buffer = Helpers::CreateBuffer(bufferSize, createFlags, m_BufferHandle.m_State, heapProps);
		m_BufferHandle.m_Buffer->SetPrivateData(WKPDID_D3DDebugObjectName, m_Name.size(), m_Name.c_str());

		if ((m_Flags & BufferFlags::DEFAULT_HEAP) != BufferFlags::NONE)
		{
			m_BufferHandle.m_Uploader = Helpers::CreateBuffer(
				bufferSize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, Helpers::kUploadHeapProps);
		}
	}

	Buffer::~Buffer()
	{
		if ((m_Flags & BufferFlags::SCREENSIZE) != BufferFlags::NONE)
		{
			GetEngine().GetRenderer().RemoveScreensize(this);
		}

		m_BufferHandle.m_Buffer.ReleaseAndGetAddressOf();
		m_BufferHandle.m_Uploader.ReleaseAndGetAddressOf();
	}

} // namespace Ball
