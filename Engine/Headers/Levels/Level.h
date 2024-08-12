#pragma once
#include <string>

#include "GameObjects/ObjectManager.h"
#include "FileIO.h"

namespace Modio
{
	struct ModInfo;
}

namespace Ball
{
	enum class LevelSaveType
	{
		None,
		Campaign = FileIO::DirectoryType::CampaignSave,
		LocalSave = FileIO::DirectoryType::LocalLevel,
		Community = FileIO::DirectoryType::CommunitySave,
		Temporary = FileIO::DirectoryType::TempData
	};

	class Level
	{
	public:
		// Load an empty level
		Level() = delete;
		Level(const std::string& filePath, const LevelSaveType& levelType);
		virtual ~Level();

		// Initialize child scene
		virtual void Init();

		// Shudown child scene
		virtual void Shutdown();

		// Update all GameObjects in the level
		// - deltaTime: The delta time, which can be used by all game objects.
		virtual void Update(float deltaTime);

		// Draw ImGui ui that is required for the level
		virtual void OnImGui() {}

		/// Calculates the bounds of a level and stores it. if bounds have been calculated already it just returns them
		/// as references
		///
		/// @param min reference that will be set to minimun of the bounds
		/// @param max reference that will be set to maximum of the bounds
		void GetLevelBounds(glm::vec3& min, glm::vec3& max, bool recalculate = false);

		// Add an object of type T to the object manager.
		// Only registered objects can be added to the object manager.
		// - prefabName: Name of the prefab that you want to apply to the object. Leave empty for no prefab to be
		// loaded.
		// - Return value: A pointer to the created object.
		template<typename T>
		T* AddObject(const std::string& prefabName = "");
		void RemoveObject(GameObject* gameObject);

		std::string GetLevelPath() const { return m_CurrentLevelPath; }
		LevelSaveType GetLevelType() const { return m_CurrentLevelType; }
		void LoadLevel(const std::string& filePath, const LevelSaveType& type);
		void ReloadLevel(const std::string& filePath, const LevelSaveType& type);

		ObjectManager& GetObjectManager() const { return *m_ObjectManager; }

		/// <summary>
		/// Get the current time playing, Not all levels need/support this
		/// </summary>
		/// <returns></returns>
		virtual float GetPlayTime() const { return 0; }

		virtual void Restart();

		void SaveLevel(bool Published = false);

	protected:
		ObjectManager* m_ObjectManager = nullptr;

	private:
		std::string m_CurrentLevelPath;
		LevelSaveType m_CurrentLevelType;

		// Level bounds
		glm::vec3 m_BoundsMin = glm::vec3(0.0f);
		glm::vec3 m_BoundsMax = glm::vec3(0.0f);
		bool m_ValidBounds = false;
	};

	template<typename T>
	inline T* Level::AddObject(const std::string& prefabName)
	{
		return m_ObjectManager->AddObject<T>(prefabName);
	}

	inline void Level::RemoveObject(GameObject* gameObject)
	{
		m_ObjectManager->RemoveObject(gameObject);
	}
} // namespace Ball