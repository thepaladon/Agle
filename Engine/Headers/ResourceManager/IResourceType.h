#pragma once
#include <string>

namespace Ball
{
	/// <summary>
	/// Base class of any resource, defines the default functions and sets up copy/move constructors.
	/// </summary>
	class IResourceType
	{
	public:
		virtual ~IResourceType() = default;
		// Cannot copy or move the actual resource
		IResourceType(const IResourceType& baseResource) = delete;
		IResourceType& operator=(const IResourceType& baseResource) = delete;
		IResourceType(IResourceType&& other) = delete;
		IResourceType& operator=(IResourceType&& other) = delete;

		/// <summary>
		/// Returns true if the Resource itself is loaded, if its not its unsafe to use any of the resource data.
		/// </summary>
		/// <returns>true if its loaded</returns>
		bool IsLoaded() const;

		/// <summary>
		/// Returns the resource path on disk (Aka the path used to loaded the asset)
		/// </summary>
		const std::string& GetPath() const;

	protected:
		template<typename T>
		friend class ResourceManager;
		template<typename T>
		friend class Resource;

		IResourceType(const std::string& path) : m_ResourcePath(path) {}

		/// <summary>
		/// Called when a asset is being loaded by the resource manager
		/// </summary>
		virtual void Load() = 0;
		/// <summary>
		/// Called when the resource manager unloads this resource.
		///	Its possible this gets called but the deconstructor does not.
		///	(As that only gets called before reloading the asset)
		/// </summary>
		virtual void Unload() = 0;

		std::string m_ResourcePath{};

	private:
		bool m_Loaded = false;
	};

	inline bool IResourceType::IsLoaded() const
	{
		return m_Loaded;
	}

	inline const std::string& IResourceType::GetPath() const
	{
		return m_ResourcePath;
	}
}; // namespace Ball