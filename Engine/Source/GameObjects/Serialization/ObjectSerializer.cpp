#include "Headers/GameObjects/Serialization/ObjectSerializer.h"

#include "Engine.h"
#include "Levels/Level.h"
#include "FileIO.h"
#include "GameObjects/ObjectManager.h"
#include "GameObjects/Serialization/ObjectFactory.h"

#define SERIALIZER_VERSION "0.3"

namespace Ball
{

	void ObjectSerializer::SaveLevel(const std::string& filePath, const LevelSaveType& type)
	{
		nlohmann::ordered_json jsonData;
		jsonData["Metadata"]["SerializationVersion"] = SERIALIZER_VERSION;
		jsonData["Metadata"]["ObjectCount"] = GetLevel().GetObjectManager().m_Objects.size();

		for (auto& it : GetLevel().GetObjectManager().m_Objects)
		{
			// Cast to GameObject so that we can use its serialization functions.
			// It also doubles as a way to check if it's actually a game object and not some other object.
			// Additionally, we also check if the object needs to be saved. If this is false, we skip the object.
			GameObject* gameObject = it.get();
			if (gameObject == nullptr || gameObject->m_CanBeSaved == false)
				continue;

			nlohmann::ordered_json baseData{};
			SerializeArchive baseArchive(SerializeArchiveType::SAVE_DATA, &baseData);
			gameObject->SerializeBase(baseArchive);

			nlohmann::ordered_json propertyData{};
			SerializeArchive propertyArchive(SerializeArchiveType::SAVE_DATA, &propertyData);
			if (!gameObject->m_PrefabTitle.empty())
				propertyArchive.m_AttachedPrefabData = PrefabReader::GetPrefabData(gameObject->m_PrefabTitle);

			gameObject->Serialize(propertyArchive);

			nlohmann::ordered_json saveData;
			saveData["Base"] = baseData;
			saveData["Properties"] = propertyData;
			jsonData["Objects"][it->GetTypeName()].push_back(saveData);
		}

		Ball::FileIO::Write(static_cast<FileIO::DirectoryType>(type), filePath, jsonData.dump(4), false);
	}

	void ObjectSerializer::LoadLevel(const std::string& filePath, const LevelSaveType& type)
	{
		// Check if save file exists
		std::string completeFilePath = FileIO::GetPath(static_cast<FileIO::DirectoryType>(type), filePath);
		ASSERT_MSG(LOG_SERIALIZER,
				   FileIO::Exist(static_cast<FileIO::DirectoryType>(type), filePath),
				   "Failed to open save file %s: File does not exist.",
				   completeFilePath.c_str());

		nlohmann::ordered_json jsonData;
		std::string fileContent = FileIO::Read(static_cast<FileIO::DirectoryType>(type), filePath);
		if (fileContent.length() <= 0)
		{
			ERROR(LOG_SERIALIZER, "Cannot load save file %s: file is empty.", completeFilePath.c_str());
			return;
		}

		jsonData = json::parse(fileContent);

		for (auto& el : jsonData["Metadata"].items())
		{
			// Check serialization version. If the save file version and engine serialization version match, we may
			// proceed.
			if (el.key() == "SerializationVersion")
			{
				auto& versionObject = jsonData["Metadata"]["SerializationVersion"];
				if (versionObject.is_string())
				{
					if (versionObject != SERIALIZER_VERSION)
					{
						ERROR(
							LOG_SERIALIZER,
							"Cannot load level file: save file uses a serialization version number that does not match "
							"with the engine serialization version number.");

						return;
					}
				}
				else
				{
					ERROR(LOG_SERIALIZER, "Cannot read \"SerializationVersion\": incorrect value.");
					return;
				}
			}

			// Reserve space in the vector. This might save some time.
			if (el.key() == "ObjectCount")
			{
				if (el.value().is_number())
				{
					GetLevel().GetObjectManager().m_Objects.reserve(el.value());
				}
				else
				{
					ERROR(LOG_SERIALIZER, "Cannot reserve object container: \"ObjectCount\" is NAN.");
				}
			}
		}

		// This container stores all objects that are loaded, we will initialize them after we are done loading.
		std::vector<GameObject*> loadedObjects;

		// This is where we load all objects into the object container.
		// Loop over all object types.
		for (auto& objectTypesIt : jsonData["Objects"].items())
		{
			// Loop over all objects of a specific type
			for (auto& objectIt : jsonData["Objects"][objectTypesIt.key()].items())
			{
				if (objectIt.value().is_object())
				{
					// Create the object
					GameObject* newObject = ObjectFactory::CreateObject(objectTypesIt.key());
					if (newObject == nullptr)
					{
						ERROR(LOG_SERIALIZER,
							  "Received a nullptr from ObjectFactory, using \"%s\" as a lookup. Discarding...",
							  objectTypesIt.key().c_str());
						continue;
					}

					// Deserialize base variables
					SerializeArchive archive(SerializeArchiveType::LOAD_DATA, &objectIt.value()["Base"]);
					newObject->SerializeBase(archive);

					PrefabData* prefabData = nullptr;

					if (!newObject->m_PrefabTitle.empty())
						prefabData = PrefabReader::GetPrefabData(newObject->m_PrefabTitle);

					if (prefabData == nullptr)
					{
						INFO(LOG_SERIALIZER, "Failed to find prefab file: no prefab file was attached to GameObject.");
					}

					// Deserialize property variables
					archive.m_AttachedPrefabData = prefabData;
					archive.m_Data = &objectIt.value()["Properties"];
					newObject->Serialize(archive);

					// Move the object into the object container
					GetLevel().GetObjectManager().m_Objects.emplace_back(newObject);
					loadedObjects.push_back(newObject);
				}
			}
		}

		// Now that all objects are loaded, we initialize them.
		for (auto object : loadedObjects)
		{
			object->Init();
		}
	}

	GameObject* ObjectSerializer::LoadPrefab(const std::string& prefabPath)
	{
		nlohmann::ordered_json data = {};
		SerializeArchive prefabArchive(SerializeArchiveType::LOAD_DATA, &data);
		prefabArchive.m_AttachedPrefabData = PrefabReader::GetPrefabData(prefabPath);

		ASSERT_MSG(LOG_SERIALIZER,
				   prefabArchive.m_AttachedPrefabData,
				   "Attempted to load a prefab hat doesn't exist: %s",
				   prefabPath.c_str());

		prefabArchive.m_Data = &prefabArchive.m_AttachedPrefabData->m_ObjectData;

		GameObject* object = ObjectFactory::CreateObject(prefabArchive.m_AttachedPrefabData->m_ObjectType);

		object->m_PrefabTitle = prefabPath;

		object->SerializeBase(prefabArchive);
		object->Serialize(prefabArchive);

		return object;
	}
} // namespace Ball
