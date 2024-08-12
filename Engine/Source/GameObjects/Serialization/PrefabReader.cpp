#include "Headers/GameObjects/Serialization/PrefabReader.h"
#include "FileIO.h"

#include "Log.h"
#include "GameObjects/Serialization/ObjectSerializer.h"

namespace Ball
{
	void PrefabReader::RegisterPrefab(const std::string& prefabFilePath)
	{
		if (m_Prefabs.find(prefabFilePath) != m_Prefabs.end())
			return;

		std::string prefabFileName = prefabFilePath + ".json";
		if (!FileIO::Exist(FileIO::DirectoryType::Prefabs, prefabFileName))
		{
			ERROR("Serializer", "Prefab file %s not found", prefabFileName.c_str());
			return;
		}

		nlohmann::ordered_json prefabFileData =
			nlohmann::json::parse(FileIO::Read(FileIO::DirectoryType::Prefabs, prefabFileName));
		if (prefabFileData.size() <= 0)
		{
			ERROR("Serializer", "Failed to read prefab file %s. Is this file empty?", prefabFileName.c_str());
			return;
		}

		std::unique_ptr<PrefabData> prefabData = std::make_unique<PrefabData>();
		prefabData->m_PrefabPath = prefabFilePath;
		prefabData->m_DisplayName = prefabFileData["DisplayName"];
		prefabData->m_Description = prefabFileData["Description"];
		prefabData->m_ThumbnailPath = prefabFileData["ThumbnailImage"];
		prefabData->m_ObjectType = prefabFileData["Type"];
		prefabData->m_LevelCost = prefabFileData["Cost"];
		prefabData->m_IncludeInPrefabBrowser = prefabFileData["IncludeInPrefabBrowser"];
		prefabData->m_ObjectData = prefabFileData["ObjectData"];

		if (prefabFileData.contains("ObjectData"))
		{
			const auto& objectJsonData = prefabFileData["ObjectData"];
			// Set model path
			if (objectJsonData.contains("ModelPath"))
				prefabData->m_ModelPath = objectJsonData["ModelPath"];
			else
				INFO(LOG_SERIALIZER, "Prefab %s does not have a model path.", prefabFilePath.c_str());
		}

		// Inject path
		prefabData->m_ObjectData.push_back({"m_PrefabPath", prefabFilePath});

		m_Prefabs.emplace(prefabFilePath, std::move(prefabData));
	}

	void PrefabData::SerializePrefabInfo(SerializeArchive& archive)
	{
		archive.Add(ARCHIVE_VAR(m_DisplayName));
		archive.Add(ARCHIVE_VAR(m_Description));
		archive.Add(ARCHIVE_VAR(m_ThumbnailPath));
		archive.Add(ARCHIVE_VAR(m_LevelCost));
		archive.Add(ARCHIVE_VAR(m_DisplayName));
	}

	std::vector<PrefabData> PrefabReader::GetListOfPrefabs()
	{
		if (m_Prefabs.empty())
			Initialize();

		std::vector<PrefabData> prefabsList = std::vector<PrefabData>(m_Prefabs.size());

		size_t index = 0;
		for (const auto& data : m_Prefabs)
		{
			prefabsList[index] = *data.second;
			index++;
		}

		return prefabsList;
	}
	std::vector<const PrefabData*> PrefabReader::GetPrefabs()
	{
		if (m_Prefabs.empty())
			Initialize();

		std::vector<const PrefabData*> prefabsList = std::vector<const PrefabData*>(m_Prefabs.size());

		size_t index = 0;
		for (const auto& data : m_Prefabs)
		{
			prefabsList[index] = data.second.get();
			index++;
		}

		return prefabsList;
	}

	std::vector<const PrefabData*> PrefabReader::GetEditorPrefabs()
	{
		// TODO this function is slow and should be optimized
		if (m_Prefabs.empty())
			Initialize();

		std::vector<const PrefabData*> prefabsList = std::vector<const PrefabData*>();
		prefabsList.reserve(m_Prefabs.size());

		for (const auto& data : m_Prefabs)
		{
			if (!data.second->CanIncludeInPrefabBrowser())
				continue;
			prefabsList.emplace_back(data.second.get());
		}

		return prefabsList;
	}

	PrefabData* PrefabReader::GetPrefabData(const std::string& prefabName)
	{
		if (m_Prefabs.empty())
			Initialize();

		auto data = m_Prefabs.find(prefabName);

		ASSERT_MSG(LOG_SERIALIZER,
				   data != m_Prefabs.end(),
				   "Attempted to load a prefab that doesn't exist: %s",
				   prefabName.c_str());

		if (data == m_Prefabs.end())
			return nullptr;

		return data->second.get();
	}

	void PrefabReader::Initialize()
	{
		for (std::filesystem::recursive_directory_iterator i(FileIO::GetPath(FileIO::Prefabs)), end; i != end; ++i)
			if (i->path().extension().string() == ".json")
			{
				auto p = i->path().lexically_relative(FileIO::GetPath(FileIO::Prefabs)).replace_extension("").string();
				RegisterPrefab(p);
			}
	}
} // namespace Ball