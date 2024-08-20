#pragma once

#include "BEAR/Texture.h"

namespace Ball
{
	class TextureVisualizer;
	class BackEndRenderer;
} // namespace Ball

namespace Ball
{
	class TextureManager
	{
	public:
		static Texture* Create(const void* data, TextureSpec spec, const std::string& name = "default_name");

		static Texture* CreateFromFilepath(const std::string& path, TextureSpec spec,
										   const std::string& name = "default_name");

		static void Destroy(Texture* buffer);
		static void DestroyAll();

		static size_t GetCount() { return m_TextureCount; }

		// Function to get TextureFormat as string
		static std::string GetFormatAsString(const TextureFormat format);

		// Function to get TextureType as string
		static std::string GetTypeAsString(const TextureType type);

	private:
		TextureManager() = delete;
		TextureManager(const TextureManager&) = delete;
		TextureManager& operator=(const TextureManager&) = delete;

		// For Cleaning GPU Upload Buffers once they've completed their job
		friend BackEndRenderer; // DirectX Specific (only this API is used rn)
		static void CleanupHelperResources();

		friend TextureVisualizer; // Uses this data to display info about Textures
		static inline std::list<Texture*> m_Textures;
		static inline size_t m_TextureCount = 0;
	};
} // namespace Ball
