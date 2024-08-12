#pragma once
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <string>

// This class is used for loading the data from a prefab file into a map.
// The data in this map can be retrieved at any time.
//
// The lookup key in the map is the name of the prefab file.
// If you're looking for the prefab data of the moving platform, the lookup key is most likely "MovingPlatform"
//
// The prefab contains information like its display name, description, thumbnail path and level cost.
// It also contains data required for deserialization.

namespace Ball
{
	struct PrefabData
	{
		friend class PrefabReader;
		friend class SerializeArchive;
		friend class ObjectManager;
		friend class ObjectSerializer;

	public:
		const std::string& GetModelPath() const { return m_ModelPath; }
		const std::string& GetPrefabPath() const { return m_PrefabPath; }
		const std::string& GetDescription() const { return m_Description; }
		const std::string& GetDisplayName() const { return m_DisplayName; }
		const std::string& GetThumbnailPath() const { return m_ThumbnailPath; }
		const std::string& GetObjectType() const { return m_ObjectType; }
		int GetLevelCost() const { return m_LevelCost; }
		bool CanIncludeInPrefabBrowser() const { return m_IncludeInPrefabBrowser; } // This getter name sucks

		/// <summary>
		/// This function serializes the generic info used for UI
		/// </summary>
		/// <param name="archive"></param>
		void SerializePrefabInfo(class SerializeArchive& archive);

	private:
		std::string m_DisplayName = "";
		std::string m_Description = "";
		std::string m_PrefabPath = "";
		std::string m_ThumbnailPath = "";
		std::string m_ObjectType = "";
		int m_LevelCost = 0;
		bool m_IncludeInPrefabBrowser = false;

		std::string m_ModelPath = "";

		// Data required for deserialization
		nlohmann::ordered_json m_ObjectData;
	};

	// TODO: Come up with a better name for this class. Kinda sucks atm.
	class PrefabReader
	{
	public:
		PrefabReader() = delete;
		~PrefabReader() = default;

		// Get a list of all the prefabs that have been added to the prefab reader.
		// This returns a list with all of the names of the added prefabs.
		static std::vector<PrefabData> GetListOfPrefabs();

		// Get a list of all the prefabs that have been added to the prefab reader.
		// This returns a list with all of the names of the added prefabs.
		static std::vector<const PrefabData*> GetPrefabs();

		static std::vector<const PrefabData*> GetEditorPrefabs();

		// Get the prefab data from a specific prefab.
		// - prefabName: Lookup key to access the prefab data. This is the name of the prefab (the same name that was
		// used when using the AddPrefab function).
		// - Return value: A pointer to the PrefabData struct. Returns nullptr when no data has been found.
		static PrefabData* GetPrefabData(const std::string& prefabName);

	private:
		/// <summary>
		/// Add a prefab file to the prefab reader.
		/// Doing this will load all data about the prefab into the prefab reader.
		/// This data can be accessed using the "GetPrefabData(...)" function.
		/// </summary>
		/// <param name="prefabFilePath">The name of the prefab that needs to be loaded</param>
		static void RegisterPrefab(const std::string& prefabFilePath);

		static void Initialize();

		inline static std::unordered_map<std::string, std::unique_ptr<PrefabData>> m_Prefabs;
	};
} // namespace Ball