#pragma once
#include <set>
#include <string>
#include <chrono>
#include <functional>

namespace Ball
{
	enum class LevelSaveType;
	// Predefined to minimize header includes
	class BaseGame;
	class Window;
	class Input;
	class ToolManager;
	class RenderAPI;
	class BaseGame;
	class LoggerSystem;
	class Level;
	class AudioSystem;
	class FileWatchSystem;
	class Camera;

	struct ApplicationConfig
	{
		uint32_t m_Width;
		uint32_t m_Height;
		std::string m_Title;
		BaseGame* m_Game;
	};

	class Engine
	{
	public:
		Engine(const Engine&) = delete;
		~Engine() = default;

		int Run(const ApplicationConfig& config);

		Level& GetLevel() const { return *m_Level; }
		Input& GetInput() const { return *m_Input; }
		LoggerSystem& GetLogger() const { return *m_Logger; }
		RenderAPI& GetRenderer() const { return *m_Renderer; }
		AudioSystem& GetAudio() const { return *m_Audio; }
		BaseGame& GetGame() const { return *m_Game; }

		// Bug: this shouldn't be done here. It should be handled better
		// If frame takes more than X ms, just treat it like it has taken X ms.
		// This helps things not to jump around when lag / loading
		static constexpr float deltaTimeCap = 100.0f;
		float GetDeltaTime() const
		{
			if (m_DeltaTime > deltaTimeCap)
				return deltaTimeCap;
			return m_DeltaTime;
		}

		/// <summary>
		/// Returns TimePoint for when the engine.Initialize got called.
		/// </summary>
		/// <returns></returns>
		const auto& GetStartupTime() const { return m_StartupTime; }
		void PauseGame(bool pause) { m_Paused = pause; }
		bool GetPauseState() const { return m_Paused; }

		/// <summary>
		/// Handels the showing of the loading screen which takes care of what type of level to load.
		/// This needs a template because the loading screen is a part of the game.
		/// </summary>
		/// <typeparam name="T"></typeparam>
		/// <param name="filePath">A path to the file that the data needs to be loaded from.</param>
		/// <param name="levelType">Determines whether to look for a campaign / official level or a mod level.</param>
		/// <param name="openEditor">Determines whether to open the level editor or a gameplay level.</param>
		template<typename T>
		void LoadLoadingScreen(const std::string& filePath, bool openEditor, bool openMenu = false);

		/// <summary>
		/// Loads all GameObjects from a level file.
		/// </summary>
		/// <typeparam name="T"></typeparam>
		/// <param name="filePath">A path to the file that the data needs to be loaded from.</param>
		/// <param name="levelType">Determines whether to look for a campaign / official level or a mod level.</param>
		template<typename T>
		void LoadLevel(const std::string& filePath, const LevelSaveType& levelType);

		/// <summary>
		/// Save all the GameObjects to a level file
		/// </summary>
		/// <param name="filePath">A path to the file that the data needs to be saved to.</param>
		/// <param name="levelType">Determines whether to look for a campaign / official level or a mod level.</param>
		void SaveLevel(const std::string& filePath, const LevelSaveType& levelType);

		/// <summary>
		/// Unloads and loads the level, essentially reloading the level
		/// </summary>
		void ReloadLevel();

		Window& GetWindow() const { return *m_Window; }
		FileWatchSystem& GetFileWatchSystem() const { return *m_FileWatch; }

		/// <summary>
		/// Called when the Application window has been resized
		/// </summary>
		/// <param name="width">New window width of the game</param>
		/// <param name="height">New window height of the game</param>
		void OnResize(const uint32_t width, const uint32_t height) const;

		void ToggleFreeCam();

	private:
		// @Note,
		// Not const because we might want to change it in runtime from an editor tool.
		// - Jesper 04-03-2024

		bool Initialize(const ApplicationConfig& config);
		void Shutdown();

		void HandleLevelSwitching();
		friend Engine& GetEngine();
		Engine();

	private:
		bool m_Paused = false;
		const std::chrono::time_point<std::chrono::system_clock> m_StartupTime = std::chrono::system_clock::now();
		float m_DeltaTime = 0;

		Window* m_Window = nullptr;
		Input* m_Input = nullptr;
		ToolManager* m_ToolManager = nullptr;
		RenderAPI* m_Renderer = nullptr;
		BaseGame* m_Game = nullptr;
		LoggerSystem* m_Logger = nullptr;
		AudioSystem* m_Audio = nullptr;
		FileWatchSystem* m_FileWatch = nullptr;

		Level* m_Level = nullptr;

		std::function<Level*()> CreateLevelObject = nullptr;

		bool m_HandleLevelSwitch = false;
		bool m_LoadingLevelSwitch = false;
		std::string m_NextLevelPath = "";
		LevelSaveType m_NextLevelType;
		std::set<std::string> m_LoadedModelPaths;

		bool m_InFreeCamera = false;
		Camera* m_FreeCamera = nullptr;
		Camera* m_PreviousCamera = nullptr;
	};

	Engine& GetEngine();

	inline LoggerSystem& GetLogger()
	{
		return GetEngine().GetLogger();
	}

	inline Level& GetLevel()
	{
		return GetEngine().GetLevel();
	}

	inline Input& GetInput()
	{
		return GetEngine().GetInput();
	}

	inline Window& GetWindow()
	{
		return GetEngine().GetWindow();
	}

	inline RenderAPI& GetRenderer()
	{
		return GetEngine().GetRenderer();
	}

	template<typename T>
	inline void Engine::LoadLoadingScreen(const std::string& filePath, bool openEditor, bool openMenu)
	{
		static_assert(std::is_base_of_v<Level, T> || std::is_same_v<Level, T>,
					  "Tried to set level with a type that does not derive from Level");

		m_NextLevelPath = "";
		m_HandleLevelSwitch = true;
		m_LoadingLevelSwitch = true;

		CreateLevelObject = [filePath, openEditor, openMenu]() { return new T(filePath, openEditor, openMenu); };
	}

	template<typename T>
	inline void Engine::LoadLevel(const std::string& filePath, const LevelSaveType& levelType)
	{
		static_assert(std::is_base_of_v<Level, T> || std::is_same_v<Level, T>,
					  "Tried to set level with a type that does not derive from Level");

		m_NextLevelPath = filePath;
		m_NextLevelType = levelType;
		m_HandleLevelSwitch = true;

		CreateLevelObject = [&path = m_NextLevelPath, &type = m_NextLevelType]() { return new T(path, type); };
	}

} // namespace Ball
