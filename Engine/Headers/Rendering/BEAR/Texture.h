#pragma once
#include <string>

#include "TypeDefs.h"
#include "Utilities/MathUtilities.h"

namespace Ball
{
	class TextureManager;

	enum class TextureFormat
	{
		R8G8B8A8_UNORM,
		R8G8B8A8_SNORM,
		R16G16B16A16_UNORM,
		R32_FLOAT,
		R32_G32_FLOAT,
		R32_G32_B32_A32_FLOAT,
		// New types will be added when needed
	};

	enum class TextureFlags
	{
		NONE = 0,
		MIPMAP_GENERATE = 1 << 1,
		ALLOW_UA = 1 << 2, // Will allow a DX12 texture to be writable in a shader
		SCREENSIZE = 1 << 3
	};

	enum class TextureType
	{
		RENDER_TARGET = 0,
		R_TEXTURE = 1,
		RW_TEXTURE = 2,
	};

	// Enable bitwise operations on the TextureFlags enum
	inline TextureFlags operator|(TextureFlags a, TextureFlags b)
	{
		return static_cast<TextureFlags>(static_cast<int>(a) | static_cast<int>(b));
	}

	inline TextureFlags operator&(TextureFlags a, TextureFlags b)
	{
		return static_cast<TextureFlags>(static_cast<int>(a) & static_cast<int>(b));
	}

	struct TextureSpec
	{
		uint32_t m_Width;
		uint32_t m_Height;
		TextureFormat m_Format;
		TextureType m_Type;
		TextureFlags m_Flags;
	};

	class Texture
	{
	public:
		// Only works for RenderTarget - Windows
		// Workaround for `ResizeFrameBuffers`
		// ToDo : For PS5 (all) and Windows (R and RW Tex).
		void Resize(int newWidth, int newHeight);

		void UpdateData(const void* data);

		// Requests Texture data to be transferred to CPU
		void RequestDataOnCPU();

		// Gets requested texture data
		// Note: Call this next frame after "RequestDataOnCPU"
		void* GetDataOnCPU();

		// Call this after you're done using the data from GetDataOnCPU
		void ReleaseDataOnCPU();

		// Getters
		std::string GetName() const { return m_Name; }
		TextureSpec GetSpec() const { return m_Spec; }
		TextureFlags GetFlags() const { return m_Spec.m_Flags; }
		TextureFormat GetFormat() const { return m_Spec.m_Format; }
		TextureType GetType() const { return m_Spec.m_Type; }
		uint32_t GetBytesPerChannel() const { return m_BytesPerChannel; }
		uint32_t GetNumChannels() const { return m_Channels; }
		uint32_t GetWidth() const { return m_Spec.m_Width; }
		uint32_t GetHeight() const { return m_Spec.m_Height; }

		uint32_t GetAlignedWidth() const
		{
			const uint32_t rowPitch = m_Spec.m_Width * m_Channels * m_BytesPerChannel;
			const uint32_t alignedRowPitch = Utilities::AlignToClosestUpper(rowPitch, 256);
			return alignedRowPitch / m_BytesPerChannel / m_Channels;
		}

		GPUTextureHandle& GetGPUHandleRef() { return m_TextureHandle; }

		uint32_t CalculateMipsNum() const
		{
			return uint32_t(fmax(1.0, log2(fmax(float(m_Spec.m_Width), float(m_Spec.m_Height))) + 1.f));
		}

		// Note: Should be private, but because of MT Model Loading it's not - [ Angel 16/08/24 ]
		void GenerateMips();

	private:
		friend TextureManager;
		Texture() = default;
		Texture(const void* data, TextureSpec spec, const std::string& name = "default_name");
		~Texture();

		uint32_t GetAlignedSize() const
		{
			const uint32_t rowPitch = m_Spec.m_Width * m_Channels * m_BytesPerChannel;
			const uint32_t alignedRowPitch = Utilities::AlignToClosestUpper(rowPitch, 256);
			return alignedRowPitch * m_Spec.m_Height;
		}

		TextureSpec m_Spec;
		GPUTextureHandle m_TextureHandle = {};
		uint32_t m_Channels = 0;
		[[maybe_unused]] uint32_t m_SizeInBytes = 0;
		[[maybe_unused]] uint32_t m_BytesPerChannel = 1;
		[[maybe_unused]] bool m_MipsGenerated = false;
		std::string m_Name;
	};
} // namespace Ball