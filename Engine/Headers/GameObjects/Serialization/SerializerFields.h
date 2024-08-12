#pragma once
#include <type_traits>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Engine.h"
#include "Log.h"

namespace Ball::Serializer
{

	template<typename T>
	std::enable_if_t<std::is_same<T, int>::value || std::is_same<T, float>::value || std::is_same<T, int64_t>::value ||
					 std::is_same<T, bool>::value || std::is_same<T, std::string>::value ||
					 std::is_same<T, const std::string&>::value || std::is_same<T, double>::value>
	SerializeValue(nlohmann::ordered_json& json, T& data, const std::string& varName, const std::string& funcSignature)
	{
		json[varName] = data;
	}

	template<typename T>
	std::enable_if_t<!std::is_same<T, int>::value && !std::is_same<T, float>::value &&
					 !std::is_same<T, int64_t>::value && !std::is_same<T, bool>::value &&
					 !std::is_same<T, std::string>::value && !std::is_same<T, const std::string&>::value &&
					 !std::is_same<T, double>::value>
	SerializeValue(nlohmann::ordered_json& json, T& data, const std::string& varName, const std::string& funcSignature)
	{
		ERROR("Serializer", "Invalid serialization type! %s", varName.c_str());
	}

	// Serialization for glm::vec3
	template<>
	inline void SerializeValue(nlohmann::ordered_json& json, glm::vec3& data, const std::string& varName,
							   const std::string& funcSignature)
	{
		json[varName] = {data.x, data.y, data.z};
	}

	// Serialization for glm::quat
	template<>
	inline void SerializeValue(nlohmann::ordered_json& json, glm::quat& data, const std::string& varName,
							   const std::string& funcSignature)
	{
		json[varName] = {data.w, data.x, data.y, data.z};
	}

	// Serialization for vectors of any (supported) type
	template<typename T>
	inline void SerializeValue(nlohmann::ordered_json& json, std::vector<T>& data, const std::string& varName,
							   const std::string& funcSignature)
	{
		nlohmann::ordered_json arrayJson;
		for (int i = 0; i < data.size(); i++)
		{
			SerializeValue(arrayJson, data.at(i), "", funcSignature);
			json[varName].push_back(arrayJson[""]);
		}
	}
} // namespace Ball::Serializer

namespace Ball::Deserializer
{
	template<typename T>
	std::enable_if_t<std::is_same<T, int>::value || std::is_same<T, float>::value || std::is_same<T, bool>::value ||
					 std::is_same<T, std::string>::value || std::is_same<T, double>::value>
	DeserializeValue(nlohmann::ordered_json& json, T& data, const std::string& varName,
					 const std::string& funcSignature)
	{
		data = json[varName];
	}

	template<typename T>
	std::enable_if_t<!std::is_same<T, int>::value && !std::is_same<T, float>::value && !std::is_same<T, bool>::value &&
					 !std::is_same<T, std::string>::value && !std::is_same<T, double>::value>
	DeserializeValue(nlohmann::ordered_json& json, T& data, const std::string& varName,
					 const std::string& funcSignature)
	{
		ERROR("Serializer", "Invalid deserialization type!");
	}

	// Deserialization for glm::vec3
	template<>
	inline void DeserializeValue(nlohmann::ordered_json& json, glm::vec3& data, const std::string& varName,
								 const std::string& funcSignature)
	{
		data = {json[varName].at(0), json[varName].at(1), json[varName].at(2)};
	}

	// Deserialization for glm::quat
	template<>
	inline void DeserializeValue(nlohmann::ordered_json& json, glm::quat& data, const std::string& varName,
								 const std::string& funcSignature)
	{
		data = {json[varName].at(0), json[varName].at(1), json[varName].at(2), json[varName].at(3)};
	}

	// Deserialization for vectors of any (supported) type
	template<typename T>
	inline void DeserializeValue(nlohmann::ordered_json& json, std::vector<T>& data, const std::string& varName,
								 const std::string& funcSignature)
	{
		nlohmann::ordered_json& jsonArray = json[varName];

		data.clear();
		data.resize(jsonArray.size());
		nlohmann::ordered_json arrayData;
		for (int i = 0; i < jsonArray.size(); i++)
		{
			arrayData[""] = jsonArray.at(i);
			DeserializeValue(arrayData, data[i], "", funcSignature);
		}
	}
} // namespace Ball::Deserializer