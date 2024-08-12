#include "Engine.h"

#include <chrono>
#include <unordered_set>

#include "BaseGame.h"
#include "Input/Input.h"
#include "Rendering/Renderer.h"
#include "Window.h"

#include <iostream>
#include "FileIO.h"

#include "Levels/Level.h"

#include "GameObjects/Serialization/ObjectSerializer.h"
#include "Rendering/ModelLoading/Model.h"

#include <cstdio>

#include "ResourceManager/ResourceManager.h"
#include "Utilities/LaunchParameters.h"

#include "Timer.h"
#include "Tools/ToolManager.h"

#include "AudioSystem.h"

#include "GameObjects/Types/Camera.h"
#include "GameObjects/Types/FreeCamera.h"

#include <Catch2/catch_amalgamated.hpp>

#include "UnitTesting.h"
#include "Logger/LoggerSystem.h"
#include "Utilities/FileWatch.h"

using namespace Ball;

void Engine::OnResize(const uint32_t width, const uint32_t height) const
{
	// If we resize to 0 scam graphics, and don't tell them
	if (!(width == 0 || height == 0))
	{
		m_Renderer->OnResize(width, height);
	}
}

bool Ball::Engine::Initialize(const ApplicationConfig& config)
{
	START_TIMER(engine_init);
	m_Logger->Init(); // default constructed logger needs to be initialized

	// Initialize file IO
	if (!FileIO::Init())
	{
		printf("Error: Failed to initialize FileIO. \n");
		return false;
	}

	// Only after FileIO has been enabled allow writing to file..
	m_Logger->AllowFileWriting();

	m_FileWatch = new FileWatchSystem();
	m_FileWatch->Init();

	// Input has to be created before window, as we depend on windproc
	m_Input = new Input();

	// Initialise window
	m_Window = new Window(config.m_Width, config.m_Height, config.m_Title);

	// Apply launch parameters to window title
	if (LaunchParameters::Count() > 0)
		GetWindow().SetName(std::string(GetWindow().GetName() + " : " + LaunchParameters::GetDisplayString()));

	// Initialise Renderer
	m_Renderer = new RenderAPI();
	m_Renderer->Init(m_Window);

	// Window init has to be called after all systems that use OnResize have been initialized
	m_Window->Init();

	m_Input->Init();

	// Set up an empty level.
	//  Calling HandleLevelSwitching() is valid here, since we're not in the update loop
	LoadLevel<Level>("", LevelSaveType::None);
	HandleLevelSwitching();

	// Initialise Tool Manager
	m_ToolManager = new ToolManager();
	m_ToolManager->Init();

	m_Audio = new AudioSystem();
	m_Audio->Init();

	// Check if game exists
	if (config.m_Game == nullptr)
	{
		// no game linked so end application
		// There should be an engine call for throw or assert here instead
		m_Window->Shutdown();
	}
	else
	{
		// Initialize Game
		m_Game = config.m_Game;
		m_Game->Initialize();
	}

	END_TIMER_MSG(engine_init, "Initialized Engine with Window %i x %i", m_Window->GetWidth(), m_Window->GetHeight());
	return true;
}

void Engine::Shutdown()
{
	// GPU frame is over
	if (m_Renderer != nullptr)
	{
		m_Renderer->WaitForExecution();
	}

	// Unload all models
	ResourceManager<Model>::UnloadAndClearAll();

	// Shutdowns in opposite order of initialize
	m_Level->Shutdown();
	delete m_Level;

	if (m_Game != nullptr)
		m_Game->Shutdown();
	delete m_Game;

	delete m_Input;

	m_ToolManager->Shutdown();
	delete m_ToolManager;

	m_Window->Shutdown();
	delete m_Window;

	m_Logger->Save();

	m_Audio->Shutdown();
	delete m_Audio;

	if (m_Renderer != nullptr)
	{
		m_Renderer->Shutdown();
		delete m_Renderer;
	}

	// Filewatch dependent on shader files from Renderer
	m_FileWatch->ShutDown();
	delete m_FileWatch;

	if (!FileIO::Shutdown())
		std::cerr << "Error: Failed to shutdown FileIO." << std::endl;
}

Engine::Engine() : m_Logger(new LoggerSystem(false))
{
}

int Engine::Run(const ApplicationConfig& config)
{
	// Log launch parameters to console
	if (LaunchParameters::Count() > 0)
		LaunchParameters::PrintLaunchParameters();

	if (!Initialize(config))
		return -1;

	if (LaunchParameters::Contains("RunTests"))
	{
		int returnValue = UnitTesting::RunAllTests();
		// The combo of headless and running tests, we assume it's the server validating functionality..
		// And it's time to go home
		if (LaunchParameters::Contains("Headless"))
		{
			Shutdown();
			return returnValue;
		}
	}

	// Basic Frame Tracking
	auto lastTime = std::chrono::high_resolution_clock::now();
	auto lastSecondTime = lastTime;
	double totalTime = 0.0;
	int frameCount = 0;

	while (m_Window->IsAlive())
	{
		// Calculate delta time
		auto currentTime = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::milli> deltaTime = currentTime - lastTime;
		lastTime = currentTime;

		m_DeltaTime = static_cast<float>(deltaTime.count());
		const float deltaTimeInMiliseconds = m_DeltaTime * 0.001f;

		totalTime += deltaTime.count();
		frameCount++;

		// Check if one second has passed
		std::chrono::duration<double> elapsed = currentTime - lastSecondTime;
		if (elapsed.count() >= 1.0)
		{
			[[maybe_unused]] double averageDeltaTime = totalTime / frameCount;

			// Reset for next second
			totalTime = 0.0;
			frameCount = 0;
			lastSecondTime = currentTime;
		}

		m_Renderer->BeginFrame();

		// Here, we check if the level needs to be switched/reloaded/unloaded
		HandleLevelSwitching();

		// Here we have the toggling of the free camera, this also pauses the game
		// Note: This should be removed from here at some point, FreeCam has no job existing in
		// the basis of the engine - Angel [03/06/24]
#ifndef SHIPPING
		if (m_Input->GetActionDown("FreeCameraToggle"))
		{
			ToggleFreeCam();
		}
#endif

		if (m_InFreeCamera)
		{
			m_FreeCamera->Update(deltaTimeInMiliseconds);
		}

		// Updates
		m_Window->Update(); // This needs to be called early, otherwise Imgui input does not work.
		m_Input->Update();

		m_Game->Update();

		if (!m_Paused)
		{
			m_Level->Update(deltaTimeInMiliseconds);
		}

		m_Audio->Update(m_DeltaTime);

#ifndef NO_IMGUI
		// Imgui rendering is done after all updates, This so that no other functions can access Imgui..
		if (!Ball::LaunchParameters::Contains("Headless"))
		{
			m_Renderer->ImGuiBeginFrame();
			m_ToolManager->OnImgui();
			m_Level->OnImGui();
			m_Game->OnImGui();
		}
#endif

		m_Renderer->Render();
		m_FileWatch->Update();

		// This should be the last function call in the loop!
		m_DeltaTime = static_cast<float>(deltaTime.count());
	}

	Shutdown();
	return 0;
}

void Ball::Engine::SaveLevel(const std::string& filePath, const LevelSaveType& levelType)
{
	if (m_Level != nullptr)
		ObjectSerializer::SaveLevel(filePath, levelType);
}

void Ball::Engine::ReloadLevel()
{
	m_NextLevelPath = m_Level->GetLevelPath();
	m_HandleLevelSwitch = true;
}

void Ball::Engine::ToggleFreeCam()
{
	m_InFreeCamera = !m_InFreeCamera;

	if (!m_InFreeCamera)
	{
		if (m_PreviousCamera != nullptr)
		{
			Camera::SetActiveCamera(m_PreviousCamera);
		}
	}
	else
	{
		m_PreviousCamera = Camera::GetActiveCamera();

		if (m_FreeCamera == nullptr)
			m_FreeCamera = Ball::GetLevel().AddObject<Ball::FreeCamera>();

		if (m_PreviousCamera)
		{
			m_FreeCamera->GetTransform().SetPosition(m_PreviousCamera->GetTransform().GetPosition());
			m_FreeCamera->GetTransform().SetRotation(m_PreviousCamera->GetTransform().GetRotation());
		}

		Camera::SetActiveCamera(m_FreeCamera);
	}

	PauseGame(m_InFreeCamera);
}

void Ball::Engine::HandleLevelSwitching()
{
	if (!m_HandleLevelSwitch)
		return;

	// Here, we check what model paths are used in a level.
	// In order to decrease duplicate code, a lambda is used for this.
	auto GetAllUsedModelPaths = [&m_Level = m_Level]() -> std::set<std::string>
	{
		std::set<std::string> usedModelPaths;
		if (m_Level != nullptr && m_Level->GetObjectManager().Size() > 0)
		{
			for (const auto& it : m_Level->GetObjectManager())
				if (!it->GetModelPath().empty())
					usedModelPaths.insert(it->GetModelPath());
		}

		return usedModelPaths;
	};

	// Get all object paths before reloading so that we can compare it later.
	std::set<std::string> m_PreLoadModelPaths = m_LoadedModelPaths;

	// Delete the level and recreate it.
	if (m_Level != nullptr)
		m_Level->Shutdown();
	delete m_Level;

	m_Level = CreateLevelObject();
	m_Level->Init();

	// Set freecam to nullpointer else it will try to update the freecam from previous scenes
	m_FreeCamera = nullptr;
	m_InFreeCamera = false;

	// Enable freecam after loading in a level when freecam is set
	if (Ball::LaunchParameters::Contains("FreeCam"))
	{
		ToggleFreeCam();
	}

	// Get all the model paths after initializing the level
	std::set<std::string> m_PostLoadModelPaths = GetAllUsedModelPaths();
	if (!m_LoadingLevelSwitch)
		m_LoadedModelPaths = m_PostLoadModelPaths;
	// Get the difference between the two sets
	std::set<std::string> difference;
	std::set_difference(m_PreLoadModelPaths.begin(),
						m_PreLoadModelPaths.end(),
						m_PostLoadModelPaths.begin(),
						m_PostLoadModelPaths.end(),
						std::inserter(difference, difference.begin()));

	if (!m_LoadingLevelSwitch)
	{
		// Print out difference for debugging
		for (const auto& it : difference)
		{
			LOG(LOG_GAMEOBJECTS, "Unloading unused model: %s.", it.c_str());
			ResourceManager<Model>::Unload(it);
		}
	}

	m_NextLevelPath = "";
	m_HandleLevelSwitch = false;
	m_LoadingLevelSwitch = false;
}

namespace Ball
{
	Engine& GetEngine()
	{
		static Engine engine;
		return engine;
	}
} // namespace Ball