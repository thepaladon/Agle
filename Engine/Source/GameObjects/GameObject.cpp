#include "Headers/GameObjects/GameObject.h"

#include "Engine.h"
#include "Levels/Level.h"
#include "GameObjects/ObjectManager.h"
#include "GameObjects/Serialization/ObjectSerializer.h"

#include "Rendering/Renderer.h"
#include "Rendering/ModelLoading/Model.h"
#include "Rendering/ModelLoading/ModelManager.h"
#include "Rendering/AnimationController.h"
#include "Rendering/BEAR/TLAS.h"
#include "FileIO.h"

using namespace Ball;

void GameObject::SetParent(GameObject* parent)
{
	// Leave room for error handling.There should be an extra check that checks whether the pointers are valid.

	// Assert here on iterate through parents
	if (DoesHierarchyContainObject(parent, this))
	{
		printf("Invalid Parenting!!! \n");
		return;
	}

	RemoveFromParent();
	m_ParentedObject = parent;

	if (m_ParentedObject != nullptr)
	{
		m_PreviousSiblingObject = m_ParentedObject->m_LastChildObject;
		m_ParentedObject->m_LastChildObject = this;

		// Extra links if the parent already had a child
		if (m_PreviousSiblingObject != nullptr)
		{
			m_PreviousSiblingObject->m_NextSiblingObject = this;
		}
	}
}

void GameObject::RemoveFromParent()
{
	if (m_ParentedObject != nullptr)
	{
		// Remove from previous parent
		// Rearrange sibling linked list
		if (m_PreviousSiblingObject != nullptr)
			m_PreviousSiblingObject->m_NextSiblingObject = m_NextSiblingObject;

		if (m_NextSiblingObject != nullptr)
			m_NextSiblingObject->m_PreviousSiblingObject = m_PreviousSiblingObject;
		else
			m_ParentedObject->m_LastChildObject = m_PreviousSiblingObject;

		m_NextSiblingObject = m_PreviousSiblingObject = nullptr;
	}
}

void GameObject::SetModel(const std::string& modelPath)
{
	// First, check if model path is valid
	if (!FileIO::Exist(FileIO::DirectoryType::Engine, modelPath) || modelPath == "")
	{
		ASSERT_MSG(
			LOG_RESOURCE, false, "Invalid model path passed into SetModel() function: \"%s\"", modelPath.c_str());
		return;
	}
	// Check if this model path hasn't already been set
	if (modelPath == m_ModelPath)
		return;

	// Set the model
	m_ModelPath = modelPath;
	GetEngine().GetRenderer().GetModelManager()->RequestReloadModels();
}

void GameObject::RemoveModel()
{
	// Check if the model is not already null
	if (m_ModelPath != "")
	{
		m_ModelPath = "";
		GetEngine().GetRenderer().GetModelManager()->RequestReloadModels();
	}
	if (m_AnimationController != nullptr)
	{
		delete m_AnimationController;
	}
}

bool GameObject::DoesHierarchyContainObject(GameObject* newParent, GameObject* currentChild)
{
	if (newParent == nullptr)
		return false;
	else
	{
		if (newParent == currentChild)
			return true;
		else
		{
			if (newParent->m_ParentedObject == nullptr)
				return false;
			else
				return DoesHierarchyContainObject(newParent->m_ParentedObject, currentChild);
		}
	}
}

glm::mat4 GameObject::MakeLocalToWorldTransform()
{
	if (m_ParentedObject != nullptr)
		return m_ParentedObject->MakeLocalToWorldTransform() * MakeLocalToParentTransform();

	else
		return MakeLocalToParentTransform();
}

glm::mat4 GameObject::MakeWorldToLocalTransform()
{
	if (m_ParentedObject != nullptr)
		return MakeParentToLocalTransform() * m_ParentedObject->MakeWorldToLocalTransform();

	else
		return MakeParentToLocalTransform();
}

const char* GameObject::GetTypeName()
{
	ASSERT_MSG(LOG_GAMEOBJECTS, false, "Attempted to Get type name but Gameobject type is not reflected !");
	return "NOT_IMPLEMENTED";
}

const std::string& GameObject::GetPrefabName() const
{
	return m_PrefabTitle;
}
const PrefabData* GameObject::GetPrefab() const
{
	return PrefabReader::GetPrefabData(m_PrefabTitle);
}

glm::mat4 GameObject::MakeLocalToParentTransform()
{
	return glm::mat4( // translate
			   glm::vec4(1.0f, 0.0f, 0.0f, 0.0f),
			   glm::vec4(0.0f, 1.0f, 0.0f, 0.0f),
			   glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
			   glm::vec4(m_Transform.GetPosition(), 1.0f)) *
		glm::mat4_cast(m_Transform.GetRotation()) // rotate
		* glm::mat4( // scale
			   glm::vec4(m_Transform.GetScale().x, 0.0f, 0.0f, 0.0f),
			   glm::vec4(0.0f, m_Transform.GetScale().y, 0.0f, 0.0f),
			   glm::vec4(0.0f, 0.0f, m_Transform.GetScale().z, 0.0f),
			   glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
}

glm::mat4 GameObject::MakeParentToLocalTransform()
{
	glm::vec3 InverseScale;
	InverseScale.x = (m_Transform.GetScale().x == 0.0f ? 0.0f : 1.0f / m_Transform.GetScale().x);
	InverseScale.y = (m_Transform.GetScale().y == 0.0f ? 0.0f : 1.0f / m_Transform.GetScale().y);
	InverseScale.z = (m_Transform.GetScale().z == 0.0f ? 0.0f : 1.0f / m_Transform.GetScale().z);
	return glm::mat4( // translate
			   glm::vec4(1.0f, 0.0f, 0.0f, 0.0f),
			   glm::vec4(0.0f, 1.0f, 0.0f, 0.0f),
			   glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
			   glm::vec4(-m_Transform.GetPosition(), 1.0f)) *
		glm::mat4_cast(glm::inverse(m_Transform.GetRotation())) // rotate
		* glm::mat4( // scale
			   glm::vec4(InverseScale.x, 0.0f, 0.0f, 0.0f),
			   glm::vec4(0.0f, InverseScale.y, 0.0f, 0.0f),
			   glm::vec4(0.0f, 0.0f, InverseScale.z, 0.0f),
			   glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
}

void GameObject::SerializeBase(SerializeArchive& archive)
{
	archive.Add(ARCHIVE_VAR(m_PrefabTitle));

	m_Transform.Serialize(archive);

	archive.Add(ARCHIVE_VAR(m_ModelPath));
	if (archive.GetArchiveType() == SerializeArchiveType::LOAD_DATA)
	{
		if (!m_ModelPath.empty())
		{
			SetModel(m_ModelPath);
			GetEngine().GetRenderer().GetModelManager()->RequestReloadModels();
		}
	}
}