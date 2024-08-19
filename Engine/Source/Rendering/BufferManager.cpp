#include "Rendering/BufferManager.h"

namespace Ball
{
	Buffer* BufferManager::Create(const void* data, uint32_t stride, uint32_t count, BufferFlags flags,
								  const std::string& name)
	{
		const auto buffer = new Buffer(data, stride, count, flags, name);
		m_Buffers.push_back(buffer);
		m_BufferCount++;
		return buffer;
	}

	void BufferManager::Destroy(Buffer* buffer)
	{
		m_Buffers.remove(buffer);
		delete buffer;
		m_BufferCount--;
	}

	void BufferManager::DestroyAll()
	{
		m_BufferCount = 0;

		for (const auto& buf : m_Buffers)
			delete buf;

		m_Buffers.clear();
	}

	void BufferManager::CleanupHelperResources()
	{
		// HACK : DirectX 12 specific
		for (const auto& buf : m_Buffers)
		{
			if (buf->GetGPUHandleRef().m_Uploader.Get() != nullptr)
				buf->GetGPUHandleRef().m_Uploader.Reset();
		}
	}

} // namespace Ball
