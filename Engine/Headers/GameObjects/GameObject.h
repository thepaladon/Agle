#pragma once
#include "IObject.h"
#include "Transform.h"

#include <glm/gtx/quaternion.hpp>
#include <string>

#include "Serialization/ObjectFactory.h"

#define REFLECT(TYPE)                                          \
public:                                                        \
	const char* GetTypeName() override                         \
	{                                                          \
		return TYPE_NAME;                                      \
	}                                                          \
	static inline const char* TYPE_NAME = #TYPE;               \
                                                               \
private:                                                       \
	__attribute__((constructor)) static void ReflectInit(void) \
	{                                                          \
		if (!Ball::ObjectFactory::Contains(TYPE_NAME))         \
			Ball::ObjectFactory::RegisterType<TYPE>();         \
	};

namespace Ball
{
	struct PrefabData;
}
namespace Ball
{
	class Shape;
	class Model;
	class PhysicsObject;
	class SerializeArchive;
	class AnimationController;
	struct TlasInstanceData;
	struct PhysicsProperties;
	struct PhysicsMaterial;

	class GameObject : public IObject
	{
		friend class ObjectManager;
		friend class ObjectSerializer;
		friend class ObjectFactory;

	public:
		GameObject() = default;
		virtual ~GameObject() override = default;

		virtual void Update(float deltaTime) override {}

		void SetModel(const std::string& modelPath);
		void RemoveModel();
		const std::string& GetModelPath() const { return m_ModelPath; }

		// Outdated parenting stuff // TODO check whether can be safely removed if outdated
		void SetParent(GameObject* parent);
		void RemoveFromParent();
		GameObject* GetParentedObject() const { return m_ParentedObject; }

		Transform& GetTransform() { return m_Transform; }
		void SetTransform(const Transform& transform) { m_Transform = transform; }

		AnimationController* GetAnimationControllerPtr() const { return m_AnimationController; }
		void SetAnimationControllerPtr(AnimationController* newController) { m_AnimationController = newController; }
		// The following 4 functions need to be verified. This can only be done once we have proper entity rendering.
		glm::mat4 MakeLocalToWorldTransform();
		glm::mat4 MakeWorldToLocalTransform();

		/// <summary>
		/// This function will return the type name of the game object.
		///	THIS ONLY WORKS IF YOU HAVE DEFINED THE REFLECT macro inside the gameobject.
		/// </summary>
		virtual const char* GetTypeName();

		/// <summary>
		/// Gives the name of the prefab used to make this gameobject
		/// </summary>
		const std::string& GetPrefabName() const;
		const PrefabData* GetPrefab() const;

	protected:
		// Iterate through parents to prevent circulair dependencies in parenting.
		bool DoesHierarchyContainObject(GameObject* newParent, GameObject* currentChild);

		glm::mat4 MakeLocalToParentTransform();
		glm::mat4 MakeParentToLocalTransform();

		Transform m_Transform;
		bool m_CanBeSaved = true;

	private:
		void SerializeBase(SerializeArchive& archive);

		GameObject* m_ParentedObject = nullptr;
		GameObject* m_LastChildObject = nullptr;
		GameObject* m_PreviousSiblingObject = nullptr;
		GameObject* m_NextSiblingObject = nullptr;

		std::string m_PrefabTitle;
		std::string m_ModelPath;
		AnimationController* m_AnimationController = nullptr;

		// We do not register baseclass.. but required in inherited classes.
		// Copy the macro below but replace GameObject with your object type
		// REFLECT(GameObject);
	};
} // namespace Ball