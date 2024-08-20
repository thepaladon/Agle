#include "Headers/Levels/Level.h"

#include <random>

#include "GameObjects/Serialization/ObjectSerializer.h"
#include "GameObjects/Serialization/PrefabReader.h"

#include "Log.h"

#include "GameObjects/Types/Camera.h"
#include "GameObjects/Types/Light.h"
#include "Rendering/Renderer.h"

namespace Ball
{
	Level::Level(const std::string& filePath, const LevelSaveType& levelType) :
		m_CurrentLevelPath(filePath), m_CurrentLevelType(levelType)
	{
		if (m_ObjectManager == nullptr)
			m_ObjectManager = new ObjectManager();

		constexpr auto path = "Images/HDRIs/";
		std::vector<std::string> hdrFiles = FileIO::GetDirectoryContent(FileIO::Engine, "Images/HDRIs/");

		// Initialize random number generator
		std::random_device rd; // Obtain a random number from hardware
		std::mt19937 gen(rd()); // Seed the generator
		std::uniform_int_distribution<> distr(0, hdrFiles.size() - 1); // Define the range

		// Select a random .hdr file
		const std::string randomFile = hdrFiles[distr(gen)];
		const std::string finalfile = path + randomFile;

		Ball::GetEngine().GetRenderer().LoadSkybox(finalfile);
	}

	Level::~Level()
	{
		if (m_ObjectManager != nullptr)
		{
			m_ObjectManager->Clear();
			delete m_ObjectManager;
		}
		m_ObjectManager = nullptr;
	}

	void Level::Init()
	{
		if (!m_CurrentLevelPath.empty())
			LoadLevel(m_CurrentLevelPath, m_CurrentLevelType);
	}

	void Level::Shutdown()
	{
		// Most code is deleted in the destructor
		// This is here for inherited classes to be able to
		// destroy custom stuff
	}

	void Level::Update(float deltaTime)
	{
		m_ObjectManager->Update(deltaTime);
	}

	void Level::GetLevelBounds(glm::vec3& min, glm::vec3& max, bool recalculate)
	{
		if (recalculate)
			m_ValidBounds = false;

		if (m_ValidBounds)
		{
			min = m_BoundsMin;
			max = m_BoundsMax;
			return;
		}

		// Get bounds of the entire map
		for (Ball::GameObject* obj : Ball::GetLevel().GetObjectManager())
		{
			glm::vec3 pos = obj->GetTransform().GetPosition();

			// Filter out particles as they are not part of the level bounds
			if (dynamic_cast<Light*>(obj))
				continue;
			if (dynamic_cast<Camera*>(obj))
				continue;

			if (pos.x > m_BoundsMax.x)
				m_BoundsMax.x = pos.x;
			if (pos.y > m_BoundsMax.y)
				m_BoundsMax.y = pos.y;
			if (pos.z > m_BoundsMax.z)
				m_BoundsMax.z = pos.z;

			if (pos.x < m_BoundsMin.x)
				m_BoundsMin.x = pos.x;
			if (pos.y < m_BoundsMin.y)
				m_BoundsMin.y = pos.y;
			if (pos.z < m_BoundsMin.z)
				m_BoundsMin.z = pos.z;
		}

		min = m_BoundsMin;
		max = m_BoundsMax;
		m_ValidBounds = true;
	}

	void Level::LoadLevel(const std::string& filePath, const LevelSaveType& type)
	{
		LOG(LOG_SERIALIZER, "Loading level data from save file: %s", m_CurrentLevelPath.c_str());
		ObjectSerializer::LoadLevel(filePath, type);
	}

	void Level::ReloadLevel(const std::string& filePath, const LevelSaveType& type)
	{
		if (m_ObjectManager != nullptr)
		{
			m_ObjectManager->Clear();
			delete m_ObjectManager;
		}

		m_ObjectManager = nullptr;
		m_ObjectManager = new ObjectManager();

		LoadLevel(filePath, type);
	}

	void Level::Restart()
	{
	}

	void Level::SaveLevel(bool published)
	{
		GetEngine().SaveLevel("ModPublish/level.json", LevelSaveType::Temporary);
	}
} // namespace Ball
