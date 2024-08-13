#pragma once
#include <memory>
#include <unordered_set>
#include "BEAR/Buffer.h"

namespace Ball
{
	class BufferManager
	{
	public:
		static Buffer* CreateBuffer(const void* data, uint32_t stride, uint32_t count,
									BufferFlags flags = BufferFlags::NONE, const std::string& name = "default_name");

		static void DestroyBuffer(Buffer* buffer);
		static void DestroyAllBuffers();

		static size_t GetBufferCount() { return m_BufferCount; }

	private:
		BufferManager() = delete;
		BufferManager(const BufferManager&) = delete;
		BufferManager& operator=(const BufferManager&) = delete;

		static inline std::unordered_set<Buffer*> m_Buffers;
		static inline size_t m_BufferCount = 0;
	};
} // namespace Ball
