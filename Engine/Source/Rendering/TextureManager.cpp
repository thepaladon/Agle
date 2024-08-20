#include "Rendering/TextureManager.h"

#include <stb/stb_image.h>

#include "FileIO.h"
#include "Log.h"

#include "Rendering/BEAR/Texture.h"

using namespace Ball;

Texture* TextureManager::Create(const void* data, TextureSpec spec, const std::string& name)
{
	Texture* texture = new Texture(data, spec, name);
	m_Textures.push_back(texture);
	m_TextureCount++;
	return texture;
}

Texture* TextureManager::CreateFromFilepath(const std::string& path, TextureSpec spec, const std::string& name)
{
	ASSERT_MSG(LOG_GRAPHICS, FileIO::Exist(FileIO::Engine, path), "Texture path doesn't exist: '%s'", path.c_str());
	std::string texturepath = FileIO::GetPath(FileIO::Engine, path);

	int width, height, channels;
	auto* data = stbi_loadf(texturepath.c_str(), &width, &height, &channels, 4);

	// HACK : I dislike how the width and the height get overwritten here for the texture spec
	// and that you're able to pass a texture spec whenever loading an image from a path
	// This needs to get thought out more - Angel [ 16/08/24 ]
	spec.m_Width = width;
	spec.m_Height = height;

	if (!data)
		ASSERT_MSG(LOG_GRAPHICS, false, "stbi_load() failed in Texture constructor for %s", path.c_str());

	Texture* texture = Create(data, spec, name);
	stbi_image_free(data);
	return texture;
}

void TextureManager::Destroy(Texture* texture)
{
	m_Textures.remove(texture);
	delete texture;
	m_TextureCount--;
}

void TextureManager::DestroyAll()
{
	m_TextureCount = 0;

	for (auto& tex : m_Textures)
		delete tex;

	m_Textures.clear();
}

std::string TextureManager::GetFormatAsString(const TextureFormat format)
{
	switch (format)
	{
	case TextureFormat::R8G8B8A8_UNORM:
		return "R8G8B8A8_UNORM";
	case TextureFormat::R8G8B8A8_SNORM:
		return "R8G8B8A8_SNORM";
	case TextureFormat::R16G16B16A16_UNORM:
		return "R16G16B16A16_UNORM";
	case TextureFormat::R32_FLOAT:
		return "R32_FLOAT";
	case TextureFormat::R32_G32_FLOAT:
		return "R32_G32_FLOAT";
	case TextureFormat::R32_G32_B32_A32_FLOAT:
		return "R32_G32_B32_A32_FLOAT";
	default:
		ASSERT_MSG(LOG_GRAPHICS, false, "Unknown Format of Texture");
		return "Unknown Format";
	}
}
std::string TextureManager::GetTypeAsString(const TextureType type)
{
	switch (type)
	{
	case TextureType::RENDER_TARGET:
		return "RENDER_TARGET";
	case TextureType::R_TEXTURE:
		return "R_TEXTURE";
	case TextureType::RW_TEXTURE:
		return "RW_TEXTURE";
	default:
		ASSERT_MSG(LOG_GRAPHICS, false, "Unknown Type of Texture");
		return "Unknown Type";
	}
}

void TextureManager::CleanupHelperResources()
{
	for (const auto& tex : m_Textures)
		tex->CleanupHelperResources();
}
