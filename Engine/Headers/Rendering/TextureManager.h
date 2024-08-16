#pragma once

#include "BEAR/Texture.h"

namespace Ball
{
	class TextureVisualizer;
	class TextureManager
	{
	public:
		static Texture* Create(const void* data, TextureSpec spec, const std::string& name = "default_name");

		static Texture* CreateFromFilepath(const std::string& path, TextureSpec spec,
										   const std::string& name = "default_name");

		static void Destroy(Texture* buffer);
		static void DestroyAll();

		static size_t GetCount() { return m_TextureCount; }

	private:
		TextureManager() = delete;
		TextureManager(const TextureManager&) = delete;
		TextureManager& operator=(const TextureManager&) = delete;

		friend TextureVisualizer; // Uses this data to display info about Textures
		static inline std::list<Texture*> m_Textures;
		static inline size_t m_TextureCount = 0;
	};
} // namespace Ball
