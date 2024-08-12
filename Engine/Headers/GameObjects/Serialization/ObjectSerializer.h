#pragma once
#include <string>
#include <nlohmann/json.hpp>

#include "GameObjects/Serialization/SerializerFields.h"
#include "GameObjects/Serialization/PrefabReader.h"

#include "Utilities/StringUtilities.h"

using json = nlohmann::ordered_json;

namespace Ball
{
	class GameObject;

#define ARCHIVE_VAR(x) x, #x, __FUNCTION__

	enum class SerializeArchiveType
	{
		SAVE_DATA = 0,
		LOAD_DATA = 1
	};

	enum class LevelSaveType;
	struct PrefabData;
	class SerializeArchive
	{
		friend class ObjectManager;
		friend class ObjectSerializer;

	public:
		SerializeArchive(SerializeArchiveType type, nlohmann::ordered_json* dataSource) :
			m_Data(dataSource), m_Type(type)
		{
		}

		template<typename T>
		void Add(T& data, const std::string& varName, const std::string& funcSignature);

		SerializeArchiveType GetArchiveType() const { return m_Type; }
		nlohmann::ordered_json& GetData() const { return *m_Data; }

	private:
		// Data required for saving:
		nlohmann::ordered_json* m_Data = nullptr;

		// Data required for both saving and loading
		SerializeArchiveType m_Type;
		PrefabData* m_AttachedPrefabData = nullptr;
	};

	class ObjectSerializer
	{
	public:
		ObjectSerializer() = delete;

		static void SaveLevel(const std::string& filePath, const LevelSaveType& type);
		static void LoadLevel(const std::string& filePath, const LevelSaveType& type);

		/// <summary>
		/// Loads prefab and returns a gameObject.
		///	Note that this gameobject is not attached to a level...
		///	As such user should not call this, only ObjectManager should
		/// </summary>
		/// <param name="prefabPath"></param>
		/// <returns>gameObject pointer not attached to a level !</returns>
		static GameObject* LoadPrefab(const std::string& prefabPath);
	};

	template<typename, typename = std::void_t<>>
	struct has_serialize : std::false_type
	{
	};
	template<typename T>
	struct has_serialize<T, std::void_t<decltype(std::declval<T>().Serialize(std::declval<SerializeArchive&>()))>>
		: std::true_type
	{
	};

	template<typename T>
	inline void SerializeArchive::Add(T& data, const std::string& varName, const std::string& funcSignature)
	{
		std::string varNameNoPrefix = Utilities::RemoveStringMemberPrefix(varName);

		// In case the inner object has a serialize function, we become recursive
		if constexpr (has_serialize<T>::value)
		{
			// Create new JsonObject, For SubObject
			nlohmann::ordered_json object = {};
			auto* root = this->m_Data;
			this->m_Data = &object;

			data.Serialize(*this);

			(*root)[varName] = object;
			this->m_Data = root;

			return;
		}

		// Serialization
		if (m_Type == SerializeArchiveType::SAVE_DATA)
		{
			Serializer::SerializeValue(*m_Data, data, varName, funcSignature);
			// Check if there is a prefab attached. If that's the case, check for overrides.
			if (m_AttachedPrefabData != nullptr)
			{
				// A prefab has been attached to the archive! Now we need to check if the data is the same.
				// If it is, we ignore it. If it isn't, we save it to the override section.

				nlohmann::ordered_json& saveDataCompare = (*m_Data)[varName];
				nlohmann::ordered_json& prefabDataCompare = m_AttachedPrefabData->m_ObjectData[varNameNoPrefix];
				if (prefabDataCompare == saveDataCompare)
					(*m_Data).erase(varName);
			}
		}
		// Deserialization
		else
		{
			if (m_AttachedPrefabData != nullptr)
			{
				std::string varNameNoPrefix = Utilities::RemoveStringMemberPrefix(varName);

				// Check for overrides in the save file
				if (m_Data->contains(varName))
				{
					Deserializer::DeserializeValue(*m_Data, data, varName, funcSignature);
				}
				// Load from prefab file if one is attached
				else if (m_AttachedPrefabData->m_ObjectData.contains(varNameNoPrefix))
				{
					std::string lookupKey = Utilities::RemoveStringMemberPrefix(varName);

					// Don't handle null data. This will only cause problems...
					if (m_AttachedPrefabData->m_ObjectData[lookupKey].type() == nlohmann::json::value_t::null)
						return;
					Deserializer::DeserializeValue(m_AttachedPrefabData->m_ObjectData, data, lookupKey, funcSignature);
				}
			}
			// If no prefab is attached, we just try to read it from the save file. This is risky, and needs to be
			// tested...
			else
			{
				if (m_Data->contains(varName))
					Deserializer::DeserializeValue(*m_Data, data, varName, funcSignature);
			}
		}
	}
} // namespace Ball