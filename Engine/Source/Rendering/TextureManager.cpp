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

void TextureManager::Destroy(Texture* buffer)
{
}

void TextureManager::DestroyAll()
{
}
