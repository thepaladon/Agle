#pragma once
#include <map>
#include <string>
#include <vector>
#include <set>
#include "FileIO.h"

namespace Ball
{
	class IFileWatchListener
	{
	public:
		virtual ~IFileWatchListener();
		/// <summary>
		/// This function sets the file to listen to and will cause the OnFileChangeEvent to be triggered if a change is
		/// detected.
		/// </summary>
		/// <param name="dir"></param>
		void AddFileWatch(const FileIO::DirectoryType& type, const std::string& dir);

		void RemoveFileWatch(const FileIO::DirectoryType& type, const std::string& dir);
		void ResetFileWatch();

		/// <summary>
		/// Get all the files this file watch is listening to
		/// </summary>
		/// <returns></returns>
		const std::set<std::string>& GetTargetFiles() const;

		/// <summary>
		/// Event that gets called when the file size has changed
		/// </summary>
		/// <param name="path">the path of the file that got Changed</param>
		virtual void OnFileWatchEvent(const std::string& path) = 0;
	};

	/// <summary>
	/// File watch system allows you to detect changes made to files at runtime.
	///	Implement <ref> IFileWatcherListener</ref>  in any class to re
	/// </summary>
	class FileWatchSystem
	{
	public:
		void Init();
		void ShutDown();
		void Update();

	private:
		friend IFileWatchListener;

		void RegisterListener(const std::string& path, IFileWatchListener* listener);

		void RemoveListener(IFileWatchListener* listener);
		void RemoveListener(IFileWatchListener* listener, const std::string& path);

		/// <summary>
		/// Structure defining the info we track to detect a file change.
		/// </summary>
		struct WatchInfo
		{
			long long m_LastFileWrite;
			size_t m_FileSize;
		};
		std::map<std::string, WatchInfo> m_WatchInfo;

		/// <summary>
		/// Lookup table to see to which path a listener is pointing to, slower for looping, but used for removal
		/// </summary>
		std::map<const IFileWatchListener*, std::set<std::string>> m_ListenerPathLookup;

		/// <summary>
		/// Directory, to listeners
		/// </summary>
		std::map<std::string, std::set<IFileWatchListener*>> m_Listeners{};
	};
} // namespace Ball
