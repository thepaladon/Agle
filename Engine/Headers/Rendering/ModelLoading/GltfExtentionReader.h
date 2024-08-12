#pragma once
#include <TinyglTF/tiny_gltf.h>
#include <glm/glm.hpp>
#include <vector>
// Tiny gltf value reading, inspired by https://github.com/nvpro-samples/nvpro_core/blob/master/nvh/gltfscene.hpp

// Return a vector of data for a tinygltf::Value
template<typename T>
static inline std::vector<T> GetVector(const tinygltf::Value& value)
{
	std::vector<T> result{0};
	if (!value.IsArray())
		return result;
	result.resize(value.ArrayLen());
	for (int i = 0; i < value.ArrayLen(); i++)
	{
		result[i] = static_cast<T>(value.Get(i).IsNumber() ? value.Get(i).Get<double>() : value.Get(i).Get<int>());
	}
	return result;
}

static inline void GetFloat(const tinygltf::Value& value, const std::string& name, float& val)
{
	if (value.Has(name))
	{
		val = static_cast<float>(value.Get(name).Get<double>());
	}
}

static inline void GetInt(const tinygltf::Value& value, const std::string& name, int& val)
{
	if (value.Has(name))
	{
		val = value.Get(name).Get<int>();
	}
}

static inline void GetVec2(const tinygltf::Value& value, const std::string& name, glm::vec2& val)
{
	if (value.Has(name))
	{
		auto s = GetVector<float>(value.Get(name));
		val = glm::vec2{s[0], s[1]};
	}
}

static inline void GetVec3(const tinygltf::Value& value, const std::string& name, glm::vec3& val)
{
	if (value.Has(name))
	{
		auto s = GetVector<float>(value.Get(name));
		val = glm::vec3{s[0], s[1], s[2]};
	}
}

static inline void GetVec4(const tinygltf::Value& value, const std::string& name, glm::vec4& val)
{
	if (value.Has(name))
	{
		auto s = GetVector<float>(value.Get(name));
		val = glm::vec4{s[0], s[1], s[2], s[3]};
	}
}

static inline void GetTexAndSampId(const tinygltf::Value& value, const std::vector<tinygltf::Texture>& textures,
								   const std::string& name, int& valt, int& vals)
{
	if (value.Has(name))
	{
		int index = value.Get(name).Get("index").Get<int>();
		if (index >= 0)
		{
			valt = textures[index].source;
			vals = textures[index].sampler;
		}
	}
}
