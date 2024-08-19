#pragma once

#include "BEAR/Buffer.h"

namespace Ball
{
	class CommandList;
	class BufferVisualizer;
} // namespace Ball

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

		// For Cleaning GPU Upload Buffers once they've completed their job
		static void CleanupHelperResources();

	private:
		BufferManager() = delete;
		BufferManager(const BufferManager&) = delete;
		BufferManager& operator=(const BufferManager&) = delete;

		;

		friend BufferVisualizer; // Uses this data to display info about Buffers
		static inline std::list<Buffer*> m_Buffers;
		static inline size_t m_BufferCount = 0;
	};
} // namespace Ball
