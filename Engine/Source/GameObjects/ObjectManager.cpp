#include "Headers/GameObjects/ObjectManager.h"

#include "Engine.h"
#include "GameObjects/Types/Light.h"
#include "Rendering/Renderer.h"

#include "Headers/Levels/Level.h"
#include "Headers/GameObjects/Serialization/ObjectSerializer.h"

#include "Rendering/BEAR/TLAS.h"
#include "Rendering/ModelLoading/ModelManager.h"

namespace Ball
{
	ObjectManager::ObjectManager()
	{
	}

	ObjectManager::~ObjectManager()
	{
	}

	void ObjectManager::ReturnOwnership(std::unique_ptr<GameObject>& targetObject)
	{
		m_Objects.emplace_back(std::move(targetObject));
		m_Objects[m_Objects.size() - 1]->Init();

		GetEngine().GetRenderer().GetModelManager()->RequestReloadModels();
	}

	std::unique_ptr<GameObject> ObjectManager::RemoveOwnership(GameObject* targetObject)
	{
		for (int i = 0; i < m_Objects.size(); i++)
		{
			if (m_Objects[i].get() == targetObject)
			{
				std::string modelPath = m_Objects[i]->GetModelPath();

				m_Objects[i]->RemoveModel();
				m_Objects[i]->Shutdown();

				std::unique_ptr<GameObject> obj = std::move(m_Objects[i]);
				obj->SetModel(modelPath);

				m_Objects.erase(m_Objects.begin() + i);
				return obj;
			}
		}
		return std::unique_ptr<GameObject>();
	}

	void ObjectManager::RemoveObject(GameObject* object)
	{
		for (int i = 0; i < m_Objects.size(); i++)
		{
			if (m_Objects[i].get() == object)
			{
				m_Objects[i]->Shutdown();
				m_Objects[i]->RemoveModel();
				m_Objects.erase(m_Objects.begin() + i);
				return;
			}
		}
	}

	void ObjectManager::Clear()
	{
		for (int i = m_Objects.size() - 1; i >= 0; i--)
		{
			m_Objects[i]->Shutdown();
			m_Objects[i]->RemoveModel();
			m_Objects.erase(m_Objects.begin() + i);
		}

		m_Objects.clear();
		m_Objects.shrink_to_fit();
	}

	void ObjectManager::Update(float deltaTime)
	{
		// Update animations
		GetEngine().GetRenderer().GetModelManager()->UpdateAnimations();

		for (int i = 0; i < m_Objects.size(); i++)
		{
			m_Objects[i]->Update(deltaTime);
		}
	}

	GameObject* ObjectManager::operator[](int i) const
	{
		if (i >= 0 && i <= Size())
			return m_Objects[i].get();
		else
			return nullptr;
	}
} // namespace Ball