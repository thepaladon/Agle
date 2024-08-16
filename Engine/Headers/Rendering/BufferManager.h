#pragma once
#include <memory>
#include <set>

#include "BEAR/Buffer.h"

namespace Ball
{
	class BufferVisualizer;
}

namespace Ball
{
	class BufferManager
	{
	public:
		static Buffer* Create(const void* data, uint32_t stride, uint32_t count, BufferFlags flags = BufferFlags::NONE,
							  const std::string& name = "default_name");

		static void Destroy(Buffer* buffer);
		static void DestroyAll();

		static size_t GetCount() { return m_BufferCount; }

	private:
		BufferManager() = delete;
		BufferManager(const BufferManager&) = delete;
		BufferManager& operator=(const BufferManager&) = delete;

		friend BufferVisualizer; // Uses this data to display info about Buffers
		static inline std::list<Buffer*> m_Buffers;
		static inline size_t m_BufferCount = 0;
	};
} // namespace Ball
