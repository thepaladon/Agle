#include "Utilities/FileWatch.h"

#include <filesystem>
#include "Engine.h"
#include "FileIO.h"
#include "Log.h"

Ball::IFileWatchListener::~IFileWatchListener()
{
	GetEngine().GetFileWatchSystem().RemoveListener(this);
}

void Ball::IFileWatchListener::AddFileWatch(const FileIO::DirectoryType& type, const std::string& dir)
{
	if (!FileIO::Exist(type, dir))
	{
		ERROR(LOG_FILEIO,
			  "Attempted to set file watch for '%s', which is a invalid file",
			  FileIO::GetPath(type, dir).c_str());
		return;
	}

	auto& fws = GetEngine().GetFileWatchSystem();

	fws.RegisterListener(FileIO::GetPath(type, dir), this);
}

void Ball::IFileWatchListener::RemoveFileWatch(const FileIO::DirectoryType& type, const std::string& dir)
{
	auto& fws = GetEngine().GetFileWatchSystem();
	fws.RemoveListener(this, FileIO::GetPath(type, dir));
}

void Ball::IFileWatchListener::ResetFileWatch()
{
	auto& fws = GetEngine().GetFileWatchSystem();
	fws.RemoveListener(this);
}

const std::set<std::string>& Ball::IFileWatchListener::GetTargetFiles() const
{
	auto& fws = GetEngine().GetFileWatchSystem();

	return fws.m_ListenerPathLookup.at(this);
}

void Ball::FileWatchSystem::Init()
{
}

void Ball::FileWatchSystem::ShutDown()
{
}
#ifdef ENABLE_FILE_WATCH
void Ball::FileWatchSystem::Update()
{
	for (auto& DirListener : m_Listeners)
	{
		auto p = std::filesystem::path(DirListener.first);

		// In some rare case file does not exist (Deleted/reloaded)
		if (!std::filesystem::exists(p))
		{
			WARN(LOG_FILEIO, "File watch file '%s' is unavailable", p.c_str());
			return;
		}

		auto fileSize = std::filesystem::file_size(p);

		auto& watchInfo = m_WatchInfo.find(DirListener.first)->second;

		if (watchInfo.m_FileSize != fileSize)
		{
			// Process the notification
			// For example, you might print change details to the console
			for (const auto& listener : DirListener.second)
			{
				listener->OnFileWatchEvent(DirListener.first);
			}

			watchInfo.m_FileSize = fileSize;
			watchInfo.m_LastFileWrite = std::filesystem::last_write_time(p).time_since_epoch().count();
		}
		else
		{
			long long lastFileWrite = std::filesystem::last_write_time(p).time_since_epoch().count();
			if (watchInfo.m_LastFileWrite != lastFileWrite)
			{
				// Process the notification
				// For example, you might print change details to the console
				for (const auto& listener : DirListener.second)
				{
					listener->OnFileWatchEvent(DirListener.first);
				}

				watchInfo.m_LastFileWrite = lastFileWrite;
			}
		}
	}
}

void Ball::FileWatchSystem::RegisterListener(const std::string& path, IFileWatchListener* listener)
{
	/*ASSERT_MSG(LOG_GENERIC,
			   m_ListenerPathLookup.find(listener) == m_ListenerPathLookup.end(),
			   "Attempted to register a listener while it already has a listener");*/
	ASSERT_MSG(LOG_GENERIC, std::filesystem::exists(path), "Target file does not exist: %s", path.c_str());

	if (m_ListenerPathLookup.find(listener) != m_ListenerPathLookup.end())
	{
		m_ListenerPathLookup.find(listener)->second.insert(path);
	}
	else
		m_ListenerPathLookup.insert({listener, {path}});

	auto foundListenerMap = m_Listeners.find(path);
	if (foundListenerMap == m_Listeners.end())
	{
		m_Listeners.insert({path, {listener}});

		long long size = std::filesystem::last_write_time(path).time_since_epoch().count();
		size_t fileSize = std::filesystem::file_size(path);

		m_WatchInfo.insert({path, {size, fileSize}});
	}
	else
	{
		foundListenerMap->second.insert(listener);
	}
}
void Ball::FileWatchSystem::RemoveListener(IFileWatchListener* listener, const std::string& path)
{
	auto foundFilePath = m_ListenerPathLookup.find(listener);

	if (foundFilePath == m_ListenerPathLookup.end())
		return; // First time setting cool

	auto foundListenerMap = m_Listeners.find(path);

	ASSERT_MSG(LOG_GENERIC,
			   foundListenerMap != m_Listeners.end(),
			   "Failed to remove FileWatchListener, it doesnt exist in listener lookup ?");

	auto& listenerMap = foundListenerMap->second;

	auto listenerMapIndex = listenerMap.find(listener);

	listenerMap.erase(listenerMapIndex);

	if (listenerMap.empty())
	{
		// This should be correct.
		m_Listeners.erase(foundListenerMap);
		m_WatchInfo.erase(m_WatchInfo.find(path));
	}
}

void Ball::FileWatchSystem::RemoveListener(IFileWatchListener* listener)
{
	auto foundFilePath = m_ListenerPathLookup.find(listener);

	if (foundFilePath == m_ListenerPathLookup.end())
		return; // First time setting cool

	for (auto& element : foundFilePath->second)
	{
		auto foundListenerMap = m_Listeners.find(element);

		ASSERT_MSG(LOG_GENERIC,
				   foundListenerMap != m_Listeners.end(),
				   "Failed to remove FileWatchListener, it doesnt exist in listener lookup ?");

		auto& listenerMap = foundListenerMap->second;

		auto listenerMapIndex = listenerMap.find(listener);
		listenerMap.erase(listenerMapIndex);

		if (listenerMap.empty())
		{
			// This should be correct.
			m_Listeners.erase(foundListenerMap);
			m_WatchInfo.erase(m_WatchInfo.find(element));
		}
	}

	m_ListenerPathLookup.erase(foundFilePath);
}

#else
void Ball::FileWatchSystem::RegisterListener(const std::string& path, IFileWatchListener* listener)
{
}
void Ball::FileWatchSystem::RemoveListener(IFileWatchListener* listener)
{
}
void Ball::FileWatchSystem::RemoveListener(IFileWatchListener* listener, const std::string& path)
{
}
void Ball::FileWatchSystem::Update()
{
}
#endif