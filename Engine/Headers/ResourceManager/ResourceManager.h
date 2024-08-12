#pragma once

#include <memory>
#include <string>
#include <vector>
#include "FileIO.h"
#include "IResourceType.h"
#include "Log.h"
#include "Resource.h"

namespace Ball
{
	template<typename T>
	class ResourceManager
	{
	public:
		static std::vector<std::string> GetAllPaths();

		/// <summary>
		/// Returns the Resource, even if its not loaded (But it has to exist !)
		/// </summary>
		/// <param name="filePath">Path to the resource</param>
		/// <returns>A valid resource if its exists, but nullptr Resource if it doesn't</returns>
		static Resource<T> Get(const std::string& filePath);
		/// <summary>
		/// Loads the current resource, use this to call constructor ect
		/// </summary>
		/// <param name="path">Path to resource to be loaded</param>
		/// <param name="...args">Additional constructor parameters for T</param>
		/// <returns>The resource that has been loaded</returns>
		template<typename... Args>
		static Resource<T> Load(const std::string& path, Args&&... args);
		/// <summary>
		/// Unloads the given resource
		/// </summary>
		/// <param name="path">resource to unload</param>
		static void Unload(const std::string& path);
		/// <summary>
		/// Checks if an item is loaded (an item can exists in the cache but still be unloaded)
		/// </summary>
		/// <param name="path">The path to item you want to check</param>
		/// <returns>True if item is loaded</returns>
		static bool IsLoaded(const std::string& path);
		/// <summary>
		/// Remove all items from this resource manager
		///	Compared to unload, this invalidates all pointers !
		/// </summary>
		static void Clear();
		/// <summary>
		/// Unloads and removes all items from this resource manager
		/// </summary>
		static void UnloadAndClearAll();
		/// <summary>
		/// Get the amount of Items stored in this resourceManager
		/// </summary>
		/// <returns> number of items</returns>
		static int Size();

	private:
		static inline std::unordered_map<std::string, std::unique_ptr<T>> m_Cache{};
	};

	template<typename T>
	std::vector<std::string> ResourceManager<T>::GetAllPaths()
	{
		std::vector<std::string> paths;
		paths.reserve(m_Cache.size());

		for (const auto& cachePair : m_Cache)
			paths.emplace_back(cachePair.first);

		return paths;
	}

	template<typename T>
	Resource<T> ResourceManager<T>::Get(const std::string& filePath)
	{
		// Check if the resource path has already been prefixed
		std::string tempPath;
		if (filePath.find(FileIO::GetPath(FileIO::DirectoryType::Engine)) == std::string::npos)
			tempPath = FileIO::GetPath(FileIO::DirectoryType::Engine) + filePath;
		else
			tempPath = filePath;

		const auto& cachePair = m_Cache.find(tempPath);
		if (cachePair != m_Cache.end())
		{
			return Resource(cachePair->second.get());
		}

		ERROR(LOG_RESOURCE, "Attempted to get '%s' but Resource doesn't exist", tempPath.c_str());
		return Resource<T>(nullptr);
	}

	template<typename T>
	template<typename... Args>
	Resource<T> ResourceManager<T>::Load(const std::string& path, Args&&... args)
	{
		// Check if the resource path has already been prefixed
		std::string tempPath;
		if (path.find(FileIO::GetPath(FileIO::DirectoryType::Engine)) == std::string::npos)
			tempPath = FileIO::GetPath(FileIO::DirectoryType::Engine) + path;
		else
			tempPath = path;

		const auto& cachePair = m_Cache.find(tempPath);

		if (cachePair == m_Cache.end())
		{
			// Its not found create it
			auto asset = std::make_unique<T>(tempPath, std::forward<Args>(args)...);
			IResourceType* resource = static_cast<IResourceType*>(asset.get());
			resource->Load();
			resource->m_Loaded = true;

			m_Cache.emplace(tempPath, std::move(asset));

			return Resource(m_Cache.find(tempPath)->second.get());
		}
		else
		{
			IResourceType* resource = static_cast<IResourceType*>(cachePair->second.get());
			if (resource->IsLoaded())
			{
				WARN(LOG_RESOURCE, "Attempted to load an already loaded asset '%s'", tempPath.c_str());

				return Resource((T*)resource);
			}

			// We have a smart pointer and want to reset it without it changing the underlying T* address.
			// I achieve this by using the placement new operator.

			resource->~IResourceType();
			new (resource) T(tempPath, std::forward<Args>(args)...);

			resource->Load();
			resource->m_Loaded = true;

			return Resource(static_cast<T*>(resource));
		}
	}

	template<typename T>
	void ResourceManager<T>::Unload(const std::string& path)
	{
		// Check if the resource path has already been prefixed
		std::string tempPath;
		if (path.find(FileIO::GetPath(FileIO::DirectoryType::Engine)) == std::string::npos)
			tempPath = FileIO::GetPath(FileIO::DirectoryType::Engine) + path;
		else
			tempPath = path;

		const auto& cachePair = m_Cache.find(tempPath);

		if (cachePair != m_Cache.end())
		{
			IResourceType* resource = static_cast<IResourceType*>(cachePair->second.get());
			resource->Unload();
			resource->m_Loaded = false;
			return;
		}

		ERROR(LOG_RESOURCE, "Attempted to unload '%s' which is not loaded", tempPath.c_str());
	}

	template<typename T>
	bool ResourceManager<T>::IsLoaded(const std::string& path)
	{
		// Check if the resource path has already been prefixed
		std::string tempPath;
		if (path.find(FileIO::GetPath(FileIO::DirectoryType::Engine)) == std::string::npos)
			tempPath = FileIO::GetPath(FileIO::DirectoryType::Engine) + path;
		else
			tempPath = path;

		auto cachePair = m_Cache.find(tempPath);
		if (cachePair == m_Cache.end())
			return false;

		return cachePair->second->m_Loaded;
	}

	template<typename T>
	void ResourceManager<T>::Clear()
	{
		m_Cache.clear();
	}

	template<typename T>
	void ResourceManager<T>::UnloadAndClearAll()
	{
		for (auto& it : m_Cache)
		{
			if (it.second->IsLoaded())
				ResourceManager<T>::Get(it.first)->Unload();
		}
		ResourceManager<T>::Clear();
	}

	template<typename T>
	int ResourceManager<T>::Size()
	{
		return m_Cache.size();
	}
} // namespace Ball