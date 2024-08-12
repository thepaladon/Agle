#pragma once
#include "Log.h"
namespace Ball
{
	/// <summary>
	/// Wrapper around resources
	///	When dealing with ResourceManager this is the type it will return.
	///	Can contain null or unloaded resources.
	/// </summary>
	/// <typeparam name="T">Type of the Resource we contain</typeparam>
	template<typename T>
	class Resource
	{
	public:
		Resource() = default;
		~Resource();
		Resource(const Resource &baseResource);
		Resource &operator=(const Resource &baseResource);
		Resource(Resource &&other) noexcept;
		Resource &operator=(Resource &&other);

		T *operator->() const;
		bool operator!() const;

		/// <summary>
		/// Checks if the underlying Resource is loaded
		/// </summary>
		/// <returns>true if its loaded</returns>
		bool IsLoaded() const;
		/// <summary>
		/// Get the object that the underlying Resource is storing
		/// </summary>
		/// <returns>Returns a raw pointer to T</returns>
		T *Get() const;

	private:
		template<typename TT>
		friend class ResourceManager;

		explicit Resource(T *instance) : m_Instance(instance) {}
		T *m_Instance = nullptr;
	};

	template<typename T>
	Resource<T>::~Resource()
	{
		m_Instance = nullptr;
	}

	template<typename T>
	T *Resource<T>::operator->() const
	{
		return Get();
	}

	template<typename T>
	bool Resource<T>::operator!() const
	{
		return m_Instance == nullptr;
	}

	template<typename T>
	Resource<T>::Resource(const Resource &baseResource)
	{
		m_Instance = baseResource.m_Instance;
	}

	template<typename T>
	Resource<T> &Resource<T>::operator=(const Resource &baseResource)
	{
		m_Instance = baseResource.m_Instance;
		return this;
	}

	template<typename T>
	Resource<T>::Resource(Resource &&other) noexcept : m_Instance(other.m_Instance)
	{
	}

	template<typename T>
	Resource<T> &Resource<T>::operator=(Resource &&other)
	{
		m_Instance = other.m_Instance;
		return *this;
	}

	template<typename T>
	bool Resource<T>::IsLoaded() const
	{
		if (m_Instance == nullptr)
			return false;

		return m_Instance->m_Loaded;
	}

	template<typename T>
	T *Resource<T>::Get() const
	{
		ASSERT_MSG(LOG_RESOURCE, m_Instance, "Instance is nullptr");

		if (!m_Instance->IsLoaded())
			return nullptr;

		return m_Instance;
	}
} // namespace Ball