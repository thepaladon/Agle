#pragma once
#include <string>
#include <vector>

namespace Ball
{
	/// <summary>
	/// Our interface of interacting with the file system. Always use this and not std functions, as this will work
	/// cross-platform.
	/// </summary>
	class FileIO
	{
	public:
		/// <summary>
		/// Directory type, each type can be mapped to a different file location, based on platform
		///	Not all directory types support writing !
		/// </summary>
		enum DirectoryType
		{
			Engine, // Engine Resources, Cross platform content!
			ToolPreset,
			Shaders, // Cross-platform Shader file location.
			PlatformSpecificShaders, // Platform specific shader file location.
			CampaignSave, // File location for official levels that we ship with the game
			CommunitySave, // File location for community levels.
			LocalLevel, // File location for levels made in the editor that haven't been uploaded to the workshop/modio
			// yet
			TempData, // Temporary data that we shouldn't keep around for too long
			Prefabs, // GameObject prefab file location
			Log, // Console log file location
			Audio // Audio files including banks, events, etc.
		};
		FileIO() = delete;

		/// <summary>
		/// Initializes the file system, and setups directory type lookup
		/// </summary>
		/// <returns></returns>
		static bool Init();

		static bool Shutdown();

		/// <summary>
		/// Checks if <targetFile> exists in the directory type
		/// </summary>
		/// <param name="type">The directory type we are looking for</param>
		/// <param name="targetFile">Relative file path we are going to check</param>
		/// <returns></returns>
		static bool Exist(DirectoryType type, const std::string& targetFile);

		/// <summary>
		/// Writes a string to the given file.
		///	Note that you cannot write to every DirectoryType
		/// </summary>
		/// <param name="type">The directoryType we try to write to</param>
		/// <param name="relativePath">The filepath where we will save this file</param>
		/// <param name="data">The data we will be writing to the file</param>
		/// <param name="appendData">If this is true we append the data to end of the file, otherwise overwrite the
		/// whole file.</param> <returns>If we wrote to the file successfully</returns>
		static bool Write(DirectoryType type, const std::string& relativePath, const std::string& data,
						  bool appendData = false);
		/// <summary>
		/// Write binary data to a file
		/// </summary>
		/// <param name="type">The directoryType we try to write to</param>
		/// <param name="relativePath">The filepath where we will save this file</param>
		/// <param name="data">The memory pointer where we read the data from</param>
		/// <param name="size">How much bytes we will write to disk (Data ,Data+Size)</param>
		/// <returns>If we wrote to the file successfully</returns>
		static bool WriteBinary(DirectoryType type, const std::string& relativePath, const void* data, size_t size);

		/// <summary>
		/// Reads a file into a string
		/// </summary>
		/// <param name="type">The directoryType we try to write to</param>
		/// <param name="relativePath">The filepath where we will save this file</param>
		/// <returns>the read data, empty string if reading failed</returns>
		static std::string Read(DirectoryType type, const std::string& relativePath);
		/// <summary>
		/// Reads a file into the supplied targetBuffer.
		/// </summary>
		/// <param name="type">The directoryType we try to write to</param>
		/// <param name="relativePath">The filepath where we will save this file</param>
		/// <param name="targetBuffer">The buffer the file will be read into</param>
		/// <param name="targetBufferSize">The max amount of bytes we can write to the target buffer</param>
		/// <returns>If we read to the file successfully</returns>
		static bool ReadBinary(DirectoryType type, const std::string& relativePath, void* targetBuffer,
							   std::streamsize targetBufferSize);

		/// <summary>
		/// Deletes the given file, Note that not all directory types support deleting
		/// </summary>
		/// <param name="type">The directoryType we try to write to</param>
		/// <param name="relativePath">The file we want to delete</param>
		/// <returns>If we deleted the file Successfully</returns>
		static bool Delete(DirectoryType type, const std::string& relativePath);

		/// <summary>
		/// Get the size of the file
		/// </summary>
		/// <param name="type"></param>
		/// <param name="relativePath"></param>
		/// <returns>size of file in bytes</returns>
		static size_t GetSize(DirectoryType type, const std::string& relativePath);

		/// <summary>
		/// Get all files in given directory
		/// </summary>
		/// <param name="type">The directory type for which we want the absolute path</param>
		/// <param name="relativePath">The relative path to directory we want to search</param>
		/// <returns>all files in the directory</returns>
		static std::vector<std::string> GetDirectoryContent(DirectoryType type, const std::string& directoryPath);
		/// <summary>
		/// Get all files in given directory with a given file extension
		/// </summary>
		/// <param name="type">The directory type for which we want the absolute path</param>
		/// <param name="relativePath">The relative path to directory we want to search</param>
		/// <param name="ext">The extension to serach for eg ".gltf"</param>
		/// <returns>all files in the directory with given extension</returns>
		static std::vector<std::string> GetDirectoryContent(DirectoryType type, const std::string& directoryPath,
															const std::string& ext);

		/// <summary>
		/// Get the absolute path for the directory type
		/// </summary>
		/// <param name="type">The directory type for which we want the absolute path</param>
		/// <returns>The absolute file path as string</returns>
		static std::string GetPath(DirectoryType type);
		/// <summary>
		/// Get the absolute path for the directory type with the appended relative path
		/// </summary>
		/// <param name="type">The directory type for which we want the absolute path</param>
		/// <param name="relativePath">A generic filepath we append to the given string</param>
		/// <returns>The absolute file path as string</returns>
		static std::string GetPath(DirectoryType type, const std::string& relativePath);

		/// <summary>
		/// Can this directory be written to
		/// </summary>
		/// <param name="type">The directory type to check for</param>
		/// <returns>True if we can write to it</returns>
		static bool HasWriteAccess(FileIO::DirectoryType type);
	};
} // namespace Ball
