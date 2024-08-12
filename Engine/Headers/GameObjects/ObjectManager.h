#pragma once

#include <vector>

#include <functional>
#include <type_traits>

#include "GameObject.h"

#include "GameObjects/Serialization/ObjectFactory.h"
#include "Serialization/ObjectSerializer.h"

struct ModelHeapLocation;
namespace Ball
{
	struct TlasInstanceData;
	class ObjectManager
	{
		friend class Level;
		friend class ObjectSerializer;

	public:
		// The constructor for the object manager.
		ObjectManager();
		ObjectManager(const ObjectManager& manager) = delete;
		~ObjectManager();

		// Add an object to the object manager.
		// prefabName: Name of the prefab file that you want to apply to the object. Leave empty for no prefab to be
		// loaded.
		template<typename T>
		T* AddObject(const std::string& prefabName);

		void ReturnOwnership(std::unique_ptr<GameObject>& targetObject);
		std::unique_ptr<GameObject> RemoveOwnership(GameObject* targetObject);

		void RemoveObject(GameObject* object);
		void Clear();

		void Update(float deltaTime);

		GameObject* operator[](int i) const;
		int Size() const { return m_Objects.size(); }

		struct Iterator
		{
			// First, set up the iterator traits
			// We assume that the iterator will be used both forward and backwards.
			using iterator_category = std::bidirectional_iterator_tag;
			// Represent the distance between the two iterators
			using difference_type = std::ptrdiff_t;

			Iterator(std::unique_ptr<GameObject>* ptr) : m_Ptr(ptr) {}

			GameObject* operator*() const { return m_Ptr->get(); }
			std::unique_ptr<GameObject>* operator->() { return m_Ptr; }

			std::unique_ptr<GameObject>& operator[](int index) { return (m_Ptr[index]); }

			Iterator& operator++()
			{
				m_Ptr++;
				return *this;
			}

			Iterator operator++(int)
			{
				Iterator tempIt = *this;
				++(*this);
				return tempIt;
			}

			Iterator& operator--()
			{
				m_Ptr--;
				return *this;
			}

			Iterator operator--(int)
			{
				Iterator tempIt = *this;
				--(*this);
				return tempIt;
			}

			friend bool operator==(const Iterator& a, const Iterator& b) { return a.m_Ptr == b.m_Ptr; }
			friend bool operator!=(const Iterator& a, const Iterator& b) { return a.m_Ptr != b.m_Ptr; }

		private:
			std::unique_ptr<GameObject>* m_Ptr;
		};

		Iterator begin() { return m_Objects.empty() ? end() : Iterator(&m_Objects[0]); }
		Iterator end() { return Iterator(m_Objects.data() + m_Objects.size()); }

	private:
		bool m_FixedSize = false;

		std::vector<std::unique_ptr<GameObject>> m_Objects;
	};

	template<typename T>
	inline T* ObjectManager::AddObject(const std::string& prefabName)
	{
		// Check if template type is derived from GameObject
		static_assert(std::is_base_of_v<GameObject, T>,
					  "Tried to add an game object that is not derived from the GameObject class");

		if (!prefabName.empty())
		{
			auto* object = ObjectSerializer::LoadPrefab(prefabName);
			m_Objects.emplace_back(std::unique_ptr<GameObject>(object));
			object->Init();
			return static_cast<T*>(object);
		}

		// There is no serialization around this object, it doesn't exist.
		auto* obj = new T();
		m_Objects.emplace_back(std::unique_ptr<GameObject>(obj));
		static_cast<GameObject*>(obj)->Init();
		return obj;
	}
} // namespace Ball