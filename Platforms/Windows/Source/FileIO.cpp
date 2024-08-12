#include "FileIO.h"
#include <filesystem>
#include <fstream>
#include <sstream>

#include <Windows.h>
#include <cassert>
#include <locale>
#include <unordered_map>

#include "Log.h"

#include <shlobj_core.h>

namespace Ball
{
	static std::unordered_map<FileIO::DirectoryType, std::string> filePaths{};

	bool FileIO::Init()
	{
		char exeFilePath[MAX_PATH];
		[[maybe_unused]] DWORD status = GetModuleFileName(NULL, (LPWSTR)exeFilePath, MAX_PATH);
		assert(status != 0);

		{
			PWSTR path_tmp;
			HRESULT folderPathResult = SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &path_tmp);
			if (folderPathResult != S_OK)
				ASSERT_MSG(LOG_FILEIO,
						   false,
						   "Failed to get known folder path for \"FOLDERID_RoamingAppData\". Return code: %i",
						   int(folderPathResult));

			// Check if the known roaming directory does infact exist.
			std::string roamingFolderPath = std::filesystem::path(path_tmp).string();
			if (!std::filesystem::exists(roamingFolderPath))
			{
				ERROR(LOG_FILEIO,
					  "Failed to find Roaming directory: %s. Trying to create new directory...",
					  roamingFolderPath.c_str());
				std::filesystem::create_directory(roamingFolderPath);
			}

			// Create a custom ball folder in the known roaming directory if it doesn't exist.
			roamingFolderPath += "/OnTheBubble/";
			if (!std::filesystem::exists(roamingFolderPath))
			{
				INFO(LOG_FILEIO, "Creating \"OnTheBubble\" folder in known directory: ", roamingFolderPath.c_str());
				std::filesystem::create_directory(roamingFolderPath);
			}

			// Files that should be stored in a known directory (like the roaming directory)
			filePaths.insert({DirectoryType::CommunitySave, ""}); // TODO don't hardcoded path to mods on windows
			filePaths.insert({DirectoryType::LocalLevel, std::string(roamingFolderPath + "LocalLevel/")});
			filePaths.insert({DirectoryType::TempData, std::string(roamingFolderPath + "TempData/")});
		}

		const auto applicationPath = std::filesystem::path(exeFilePath).parent_path().string();

		filePaths.insert({DirectoryType::CampaignSave, applicationPath + "Resources/CampaignSaves/"});

		filePaths.insert({DirectoryType::Engine, applicationPath + "Resources/"});
		filePaths.insert({DirectoryType::Prefabs, applicationPath + "Resources/Prefabs/"});
		filePaths.insert({DirectoryType::Shaders, applicationPath + "Shaders/"});
		filePaths.insert({DirectoryType::PlatformSpecificShaders, applicationPath + "ShadersWin/"});
		filePaths.insert({DirectoryType::Audio, applicationPath + "Resources/Audio/"});
		filePaths.insert({DirectoryType::Log, applicationPath + "Log/"});

		filePaths.insert({DirectoryType::ToolPreset, applicationPath + "Resources/ToolSettings"});

		return true;
	}

	bool FileIO::Shutdown()
	{
		return true;
	}

	bool FileIO::Exist(DirectoryType type, const std::string& targetFile)
	{
		return std::filesystem::exists(GetPath(type, targetFile));
	}

	bool FileIO::Write(DirectoryType type, const std::string& relativePath, const std::string& data, bool appendData)
	{
		if (!HasWriteAccess(type))
		{
			ERROR(LOG_FILEIO, "DirectoryType '%i' is not savable", type);
			return false;
		}

		const std::string filePath = GetPath(type, relativePath);

		if (!std::filesystem::exists(std::filesystem::path(filePath).parent_path()))
		{
			std::filesystem::create_directories(std::filesystem::path(filePath).parent_path());
		}

		// Open file. Assign the right flags, based on whether to append or truncate the data.
		std::fstream file{};

		std::ios_base::openmode mode = std::fstream::out;
		mode |= appendData ? std::fstream::app : std::fstream::trunc;

		file.open(filePath, mode);

		if (!file.is_open())
		{
			ERROR(LOG_FILEIO, "Failed to open '%s' for writing.\n", filePath.c_str());
			return false;
		}

		file << data;
		file.close();

		INFO(LOG_FILEIO, "Wrote to file: %s", filePath.c_str());

		return true;
	}

	bool FileIO::WriteBinary(DirectoryType type, const std::string& relativePath, const void* data, size_t size)
	{
		ASSERT_MSG(LOG_FILEIO, data, "Called FileIO::write with a invalid data pointer");

		if (!HasWriteAccess(type))
		{
			ERROR(LOG_FILEIO, "DirectoryType '%i' is not savable", type);
			return false;
		}

		std::string filePath = GetPath(type, relativePath);

		if (!std::filesystem::exists(std::filesystem::path(filePath).parent_path()))
		{
			std::filesystem::create_directories(std::filesystem::path(filePath).parent_path());
		}

		// Open file. Assign the right flags, based on whether to append or truncate the data.
		std::fstream file{};

		file.open(filePath, std::fstream::out | std::fstream::trunc | std::ios::binary);

		if (!file.is_open())
		{
			ERROR(LOG_FILEIO, "Failed to open '%s' for writing.\n", filePath.c_str());
			return false;
		}

		file.write(static_cast<const char*>(data), size);
		file.close();

		INFO(LOG_FILEIO, "Wrote to binary file: %s", filePath.c_str());
		return true;
	}

	std::string FileIO::Read(DirectoryType type, const std::string& relativePath)
	{
		// Confirm that the file exists
		if (!Exist(type, relativePath))
		{
			ERROR(LOG_FILEIO, "Attempted to read from '%s' which doesnt exist !", GetPath(type, relativePath).c_str());
			return "";
		}

		auto p = GetPath(type, relativePath);
		// Open and read file
		std::fstream file(p, std::fstream::in);
		if (!file.good())
		{
			ERROR(LOG_FILEIO, "Failed to open '%s' for reading.\n", GetPath(type, relativePath).c_str());
			return "";
		}

		// Read entire file
		std::stringstream ss;
		ss << file.rdbuf();
		std::string data = ss.str();
		file.close();
		return data;
	}

	bool FileIO::ReadBinary(DirectoryType type, const std::string& relativePath, void* targetBuffer,
							std::streamsize targetBufferSize)
	{
		ASSERT_MSG(LOG_FILEIO, targetBuffer, "Called FileIO::Read with a invalid targetbuffer");
		// Confirm that the file exists
		if (!Exist(type, relativePath))
		{
			ASSERT_MSG(LOG_FILEIO, false, "ReadFromFile failed, '%s' does not exist.", relativePath.c_str());
			return false;
		}

		std::ifstream file(GetPath(type, relativePath), std::ios::binary);
		if (!file.is_open())
		{
			ERROR(LOG_FILEIO, "file '%s' not found!", relativePath.c_str());
			return false;
		}

		auto size = std::filesystem::file_size(GetPath(type, relativePath));

		if (targetBufferSize > size)
		{
			ERROR(LOG_FILEIO, "Target buffer is to big to read the file into");
			return false;
		}

		file.read(static_cast<char*>(targetBuffer), targetBufferSize);
		file.close();

		return true;
	}

	std::vector<std::string> FileIO::GetDirectoryContent(DirectoryType type, const std::string& directoryPath)
	{
		std::vector<std::string> paths;

		auto path = FileIO::GetPath(type, directoryPath);

		auto itt = std::filesystem::directory_iterator(path);
		for (const auto& entry : itt)
		{
			paths.push_back(entry.path().lexically_relative(path).string());
		}

		return paths;
	}

	std::vector<std::string> FileIO::GetDirectoryContent(DirectoryType type, const std::string& directoryPath,
														 const std::string& extension)
	{
		std::vector<std::string> paths;

		auto path = FileIO::GetPath(type, directoryPath);

		auto itt = std::filesystem::directory_iterator(path);
		for (const auto& entry : itt)
		{
			if (entry.path().extension() == extension)
				paths.push_back(entry.path().lexically_relative(path).string());
		}

		return paths;
	}

	bool FileIO::Delete(DirectoryType type, const std::string& relativePath)
	{
		if (!Exist(type, relativePath))
			return false;

		return std::filesystem::remove(GetPath(type, relativePath));
	}

	size_t FileIO::GetSize(DirectoryType type, const std::string& relativePath)
	{
		return std::filesystem::file_size(GetPath(type, relativePath));
	}

	std::string FileIO::GetPath(DirectoryType type)
	{
		const auto rootPath = filePaths.find(type);
		assert(rootPath != filePaths.end()); // If this asserts it needs to be implemented in initialize most likely !

		return rootPath->second;
	}

	std::string FileIO::GetPath(DirectoryType type, const std::string& relativePath)
	{
		return std::filesystem::path(GetPath(type)).append(relativePath).string();
	}

	bool FileIO::HasWriteAccess(FileIO::DirectoryType type)
	{
		return type == FileIO::CampaignSave || type == FileIO::LocalLevel || type == FileIO::CommunitySave ||
			type == FileIO::Log || type == FileIO::TempData || FileIO::ToolPreset;
	}
} // namespace Ball