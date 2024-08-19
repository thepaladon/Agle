#pragma once
#include <string>
#include "TypeDefs.h"

namespace Ball
{
	class BufferManager;

	enum class BufferFlags
	{
		NONE = 0,
		CBV = 1 << 1, // 256 byte aligned structs (b0)
		SRV = 1 << 2, // An array of data (t0)
		UAV = 1 << 3, // A writable array of data (u0)
		ALLOW_UA = 1 << 4, // Can be writable
		DEFAULT_HEAP = 1 << 5, // Default heap is hard to rewrite - fast to access in shaders, used for data which is
							   // rarely updated
		UPLOAD_HEAP = 1 << 6, // Upload heap is fast to update - slower to access in shader, used for data which is
							  // regularly updated
		VERTEX_BUFFER = 1 << 7, // PS5 specific, as requires the unique struct
		SCREENSIZE = 1 << 8
	};

	// Enable bitwise operations on the BufferFlags enum
	inline BufferFlags operator|(BufferFlags a, BufferFlags b)
	{
		return static_cast<BufferFlags>(static_cast<int>(a) | static_cast<int>(b));
	}

	inline BufferFlags operator&(BufferFlags a, BufferFlags b)
	{
		return static_cast<BufferFlags>(static_cast<int>(a) & static_cast<int>(b));
	}

	class Buffer
	{
	public:
		uint32_t GetNumElements() const { return m_Count; }
		uint32_t GetStride() const { return m_Stride; } // in Bytes
		uint32_t GetSizeBytes() const { return m_Count * m_Stride; } // in Bytes
		std::string GetName() const { return m_Name; }
		BufferFlags GetFlags() const { return m_Flags; }
		GPUBufferHandle& GetGPUHandleRef() { return m_BufferHandle; }
		void UpdateData(const void* data, uint32_t dataSizeInBytes);
		void Resize(uint32_t newCount);

	private:
		// Use BufferManager for BufferCreation and deletion
		friend BufferManager;
		Buffer() = delete;
		Buffer(const void* data, const uint32_t stride, const uint32_t count, BufferFlags flags = BufferFlags::NONE,
			   const std::string& name = "default_name");
		~Buffer();

		void CleanupHelperResources();

		GPUBufferHandle m_BufferHandle;
		std::string m_Name = "DEFAULT_NAME_FOR_BUFFER";
		uint32_t m_Stride = 0;
		uint32_t m_Count = 0;
		BufferFlags m_Flags = BufferFlags::NONE;
	};

} // namespace Ball
