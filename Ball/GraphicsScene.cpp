#include "GraphicsScene.h"

#include "DebugInputSetup.h"
#include "Engine.h"
#include "Levels/Level.h"

#include "GameObjects/Serialization/PrefabReader.h"
#include "GameObjects/Types/TestObject.h"
#include "GameObjects/Types/FreeCamera.h"

#include "Rendering/Renderer.h"
#include "Rendering/ModelLoading/ModelManager.h"

[[maybe_unused]] constexpr int g_numObjects = 128;
[[maybe_unused]] constexpr int g_numModels = 4;
const std::string randomSmallModels[g_numModels] = {"Models/Spark/BlueSpark.glb",
													"Models/Spark/GreenSpark.glb",
													"Models/Spark/OrangeSpark.glb",
													"Models/Spark/RedSpark.glb"};

// Between -1 and 1
float neg_randf()
{
	return 2.0f * static_cast<float>(rand()) / static_cast<float>(RAND_MAX) - 1.0f; // Generate
}

float randf()
{
	return static_cast<float>(rand()) / static_cast<float>(RAND_MAX); // Generate
}

void GraphicsScene::Initialize()
{
	DebugInputSetup();
	LOG(LOG_GENERIC, "Loading the Graphics Scene");

	// Add a camera to the level
	Ball::FreeCamera* camera = Ball::GetLevel().AddObject<Ball::FreeCamera>();
	camera->GetTransform().SetPosition(glm::vec3(-6.0f, 2.f, 0.f));
	camera->GetTransform().SetRotation(glm::vec3(3.14f, -1.57f, 3.14f));
	Ball::Camera::SetActiveCamera(camera); // Make this the active camera for the renderer

	auto sponza = Ball::GetLevel().AddObject<TestCubeEntity>("");
	sponza->SetModel("Models/Sponza/Sponza.gltf");

	constexpr int spawnModels = g_numObjects;
	for (int i = 0; i < spawnModels; i++)
	{
		const auto modelID = i % g_numModels;
		const auto randomModels = Ball::GetLevel().AddObject<TestCubeEntity>("");
		randomModels->SetModel(randomSmallModels[modelID]);

		randomModels->GetTransform().SetPosition(
			glm::vec3(neg_randf() * 10.f, randf() * 2.f + 1.0f, neg_randf() * 10.f));
		randomModels->GetTransform().SetEulerAngles(randf() * 3.14f * glm::vec3(randf(), randf(), randf()));
		randomModels->GetTransform().SetScale(glm::vec3(0.1f + randf() * 0.2f));
	}
}

void GraphicsScene::Update()
{
}

void GraphicsScene::Shutdown()
{
}

GraphicsScene::~GraphicsScene()
{
}
