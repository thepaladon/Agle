#include "Rendering/BufferManager.h"

namespace Ball
{
	Buffer* BufferManager::CreateBuffer(const void* data, uint32_t stride, uint32_t count, BufferFlags flags,
										const std::string& name)
	{
		const auto buffer = new Buffer(data, stride, count, flags, name);
		m_Buffers.push_back(buffer);
		m_BufferCount++;
		return buffer;
	}

	void BufferManager::DestroyBuffer(Buffer* buffer)
	{
		m_Buffers.remove(buffer);
		delete buffer;
		m_BufferCount--;
	}

	void BufferManager::DestroyAllBuffers()
	{
		for (auto& buf : m_Buffers)
		{
			delete buf;
		}
	}
} // namespace Ball
