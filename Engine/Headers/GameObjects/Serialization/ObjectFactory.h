#pragma once
#include <unordered_map>
#include "Log.h"

namespace Ball
{
	class GameObject;

	class ObjectFactory
	{
	public:
		/// <summary>
		/// Register object type to the factory.. Should only be called by gameObjects
		/// </summary>
		/// <typeparam name="T">ObjectType that inherents from GameObject</typeparam>
		template<typename T>
		static void RegisterType();

		/// <summary>
		/// Create a new GameObject from a typename
		/// </summary>
		/// <param name="TypeName">The Object type as string, can get this from T::TYPE_NAME</param>
		/// <returns>A game object, not added to any level</returns>
		static GameObject* CreateObject(const std::string& TypeName);

		/// <summary>
		/// Checks if the given typename is registered to the ObjectFactory.
		/// </summary>
		/// <param name="TypeName">The Object type as string, can get this from T::TYPE_NAME</param>
		/// <returns>True if the type exist</returns>
		static bool Contains(const char* TypeName);

	private:
		ObjectFactory() = default;
		~ObjectFactory() = default;

		/// <summary>
		/// Get the Unique instance of the object factory.
		/// </summary>
		/// <returns></returns>
		static ObjectFactory& GetInstance();

		typedef GameObject* (*CreateObjectFunc)();
		std::unordered_map<std::string, CreateObjectFunc> m_ObjectCreationFunctions{};

		/// <summary>
		/// This function creates the unique object instance
		/// </summary>
		template<typename T>
		static GameObject* CreateObjectPtr()
		{
			return new T();
		}
	};

	template<typename T>
	void ObjectFactory::RegisterType()
	{
		static_assert(std::is_base_of_v<GameObject, T>, "T must inherit from GameObject");

		const char* objectName = T::TYPE_NAME;

		auto& registeredTypes = GetInstance().m_ObjectCreationFunctions;

		ASSERT_MSG(LOG_GAMEOBJECTS,
				   registeredTypes.find(objectName) == registeredTypes.end(),
				   "Failed to register type \"%s\" in object factory: Object is already registered.",
				   objectName);

		registeredTypes.insert({objectName, CreateObjectPtr<T>});
	}

	inline ObjectFactory& ObjectFactory::GetInstance()
	{
		static ObjectFactory instance = {};

		return instance;
	}
} // namespace Ball