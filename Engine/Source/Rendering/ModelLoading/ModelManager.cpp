#include "Headers/Rendering/ModelLoading/ModelManager.h"
#include "Headers/Rendering/ModelLoading/Model.h"

#include "Headers/Rendering/BEAR/Buffer.h"
#include "Headers/Rendering/BEAR/ResourceDescriptorHeap.h"
#include "Headers/Rendering/BEAR/TLAS.h"
#include "Headers/Rendering/BEAR/Texture.h"

#include "Headers/Engine.h"
#include "Headers/GameObjects/ObjectManager.h"
#include "Headers/Levels/Level.h"

#include "Headers/GameObjects/GameObject.h"
#include "FileIO.h"
#include "Shaders/ShaderHeaders/WavefrontStructsGPU.h"

#include "ResourceManager/ResourceManager.h"
#include "Utilities/LaunchParameters.h"
#include "Rendering/AnimationController.h"

#include "Rendering/Renderer.h"

#include <ImGui/imgui.h>

#include "Timer.h"
#include <unordered_set>

#include "Rendering/BufferManager.h"

#ifdef USE_THREADED_MODEL_LOADING
#include <execution>
#include <mutex>
#endif

namespace Ball
{
	ModelManager::ModelManager()
	{
	}

	ModelManager::~ModelManager()
	{
		delete m_TLAS;
		BufferManager::Destroy(m_LightData);
		BufferManager::Destroy(m_ModelHeapLocationBuffer);
		BufferManager::Destroy(m_InstanceTransformsBuffer);
	}

	void ModelManager::RequestReloadModels()
	{
		// We should only request to reload models when headless is not enabled.
		if (!LaunchParameters::Contains("Headless"))
			m_ReloadModels = true;
	}

	void ModelManager::ProcessModelLoadingQueue(ResourceDescriptorHeap& rdhToStoreModels)
	{
		m_AnimatedGameObjects.clear();
		std::unordered_set<std::string> modelsToLoad;
		for (auto gameObject : GetLevel().GetObjectManager())
		{
			// Don't load models with empty paths
			if (gameObject->GetModelPath().empty())
				continue;

			if (!FileIO::Exist(FileIO::Engine, gameObject->GetModelPath()))
			{
				ERROR(LOG_RESOURCE, "%s - does not exist!", gameObject->GetModelPath().c_str());
				continue;
			}

			if (!ResourceManager<Model>::IsLoaded(gameObject->GetModelPath()))
			{
				modelsToLoad.insert(gameObject->GetModelPath());
			}
		}

#ifdef USE_THREADED_MODEL_LOADING

		START_TIMER(LoadingModelTime);

		if (!modelsToLoad.empty())
		{
			std::for_each(std::execution::par,
						  std::begin(modelsToLoad),
						  std::end(modelsToLoad),
						  [&](std::string path)
						  {
							  INFO(LOG_RESOURCE, "Loading model: %s", path.c_str());
							  ResourceManager<Model>::Load(path).Get();
						  });
			modelsToLoad.clear();
		}

		// Now go over all models and add animation instances
		for (auto object : GetLevel().GetObjectManager())
		{
			if (object->GetModelPath().empty())
				continue;
			Model* loadedModel = ResourceManager<Model>::Get(object->GetModelPath()).Get();
			if (loadedModel->HasAnimation())
			{
				m_AnimatedGameObjects.push_back(object);
				if (object->GetAnimationControllerPtr() == nullptr)
				{
					object->SetAnimationControllerPtr(new AnimationController(loadedModel));
				}
			}
		}

		END_TIMER_MSG(LoadingModelTime, "Finished loading all models");

#else
		for (auto gameObject : GetLevel().GetObjectManager())
		{
			if (gameObject->GetModelPath().empty())
				continue;

			if (!ResourceManager<Model>::IsLoaded(gameObject->GetModelPath()))
			{
				INFO(LOG_RESOURCE, "Loading model: %s", gameObject->GetModelPath().c_str());
				Model* newModel = ResourceManager<Model>::Load(gameObject->GetModelPath()).Get();

				// Add animation instances
				if (newModel->HasAnimation())
				{
					m_AnimatedGameObjects.push_back(gameObject);
					if (gameObject->GetAnimationControllerPtr() == nullptr)
					{
						gameObject->SetAnimationControllerPtr(new AnimationController(newModel));
					}
				}
			}
			else
			{
				Model* loadedModel = ResourceManager<Model>::Get(gameObject->GetModelPath()).Get();
				// Add animation instances
				if (loadedModel->HasAnimation())
				{
					m_AnimatedGameObjects.push_back(gameObject);
					if (gameObject->GetAnimationControllerPtr() == nullptr)
					{
						gameObject->SetAnimationControllerPtr(new AnimationController(loadedModel));
					}
				}
			}
		}
#endif

		// Create the Resource Desriptor Heap with all required loaded models
		int currentAddedModelsId = 0;
		std::vector<ModelHeapLocation> cpuBuffer;
		const auto loadedModels = ResourceManager<Model>::GetAllPaths();
		for (const auto& path : loadedModels)
		{
			auto model = ResourceManager<Model>::Get(path);

			if (model.IsLoaded())
			{
				for (auto* texture : model.Get()->m_Textures)
				{
					if ((texture->GetSpec().m_Flags & TextureFlags::MIPMAP_GENERATE) == TextureFlags::MIPMAP_GENERATE)
						texture->GenerateMips();
				}

				auto info = AddModel(rdhToStoreModels, model);
				cpuBuffer.push_back(info);
				model->m_ModelIndexID = currentAddedModelsId;
				currentAddedModelsId++;
			}
		}

		BufferManager::Destroy(m_ModelHeapLocationBuffer);
		m_ModelHeapLocationBuffer = BufferManager::Create(cpuBuffer.data(),
														  sizeof(cpuBuffer[0]),
														  cpuBuffer.size(),
														  BufferFlags::SRV | BufferFlags::DEFAULT_HEAP,
														  "World Info Buffer");

		rdhToStoreModels.Switch(*m_ModelHeapLocationBuffer, RDH_MODEL_DATA);

		RebuildTLAS(rdhToStoreModels);
		m_ReloadModels = false;
	}
	void ModelManager::UpdateAnimations()
	{
		for (auto gameObject : GetLevel().GetObjectManager())
		{
			if (gameObject->GetAnimationControllerPtr() != nullptr)
			{
				// HACK : as we don't have a fixed timestep anymore
				gameObject->GetAnimationControllerPtr()->Update(1 / 50.f);
			}
		}
	}

	void ModelManager::UpdateAnimationsGPU()
	{
		for (int i = 0; i < m_AnimatedGameObjects.size(); i++)
		{
			if (m_AnimatedGameObjects[i] != nullptr)
			{
				if (m_AnimatedGameObjects[i]->GetAnimationControllerPtr() != nullptr)
				{
					m_AnimatedGameObjects[i]->GetAnimationControllerPtr()->RebuildModelBlas();
				}
			}
		}
	}
	void ModelManager::UpdateInstanceTransformsBuffer()
	{
		std::vector<GameObject*> objectsWithModels;
		uint32_t index = 0;
		for (auto const& object : GetLevel().GetObjectManager())
		{
			if (object->GetModelPath().length() > 0)
			{
				objectsWithModels.emplace_back(object);
				index++;
			}

			if (object == GetRenderer().m_SelectedObject)
				GetRenderer().m_SelectedObjectID = index;

			if (object == GetRenderer().m_HoveredObject)
				GetRenderer().m_HoveredObjectID = index;
		}

		// WARNING: This works under the assumption that game objects do not jump around
		// this vector but stay in a consistent position. If the number of game object changes,
		// this will trigger a full rebuild of the model queue so then it should be fine

		// I do not like this approach, but I like the idea directly updating a TlasInstanceData*
		// that lives in the Renderer from GameObject even less. - [ Angel 19/01/24 ]
		std::vector<glm::mat4> instanceTransforms;
		instanceTransforms.reserve(GetLevel().GetObjectManager().Size());

		for (int i = 0; i < objectsWithModels.size(); i++)
		{
			auto newMat = objectsWithModels[i]->GetTransform().GetModelMatrix();
			instanceTransforms.push_back(newMat);
			m_TLAS->SetInstanceTransform(newMat, i);
		}

		m_InstanceTransformsBuffer->UpdateData(instanceTransforms.data(),
											   instanceTransforms.size() * sizeof(glm::mat4));
	}

	ModelHeapLocation ModelManager::AddModel(ResourceDescriptorHeap& rdhToStoreModels, const Resource<Model> model)
	{
		ModelHeapLocation info;
		info.m_ModelStart = rdhToStoreModels.GetNumElements();
		rdhToStoreModels.Add(*model->m_GPUPrimitiveBuffer);
		rdhToStoreModels.Add(*model->m_GPUMaterialBuffer);

		{
			// Push Back the Buffers
			for (const auto& buffer : model->m_Buffers)
			{
				rdhToStoreModels.Add(*buffer);
			}
		}

		// Location of First Texture
		{
			info.m_TextureStart = rdhToStoreModels.GetNumElements();

			// Push Back the Textures
			for (const auto& texture : model->m_Textures)
			{
				rdhToStoreModels.Add(*texture);
			}
		}

		return info;
	}

	inline std::pair<bool, int> HasModelBeenCollectedAndGiveID(std::unordered_map<std::string, int>& pathCollected,
															   int& nextID, const std::string& path)
	{
		const auto pathFound = pathCollected.find(path);
		if (pathFound != pathCollected.end())
		{
			// Model already collected, return its ID.
			return {true, pathFound->second};
		}

		// New model. Model hasn't been collected
		// - increment Model counter
		const int id = nextID;
		pathCollected[path] = nextID++;
		return {false, id};
	}

	const uint32_t ModelManager::GetNumLightsInScene() const
	{
		return m_LightData->GetNumElements();
	}

	void ModelManager::RebuildTLAS(ResourceDescriptorHeap& rdhToStoreTLASBuffers)
	{
		delete m_TLAS;
		std::vector<TlasInstanceData*> tlasConstructionData = CreateTlasInstanceData();

		// Get All TLAS Model Matrices
		std::vector<glm::mat4> instanceTransforms;
		instanceTransforms.reserve(tlasConstructionData.size());
		for (const auto& instance : tlasConstructionData)
		{
			instanceTransforms.push_back(instance->m_Transform);
		}

		BufferManager::Destroy(m_InstanceTransformsBuffer);
		m_InstanceTransformsBuffer = BufferManager::Create(instanceTransforms.data(),
														   sizeof(glm::mat4),
														   tlasConstructionData.size(),
														   BufferFlags::SRV | BufferFlags::UPLOAD_HEAP,
														   "Transform Buffer");
		rdhToStoreTLASBuffers.Switch(*m_InstanceTransformsBuffer, RDH_TRANSFORMS);

		m_TLAS = new TLAS(tlasConstructionData);

		FillInLights();
	}

	void ModelManager::FillInLights()
	{
		BufferManager::Destroy(m_LightData);

		std::vector<GameObject*> objectsWithModels;
		for (auto const& object : GetLevel().GetObjectManager())
		{
			if (object->GetModelPath().length() > 0)
				objectsWithModels.emplace_back(object);
		}

		// ModelID, InstanceID, PrimitiveID, LightsInPrim
		std::vector<LightPickData> lightDataCPU;

		for (uint32_t i = 0; i < m_TLAS->GetNumInstances(); i++)
		{
			const auto modelId = m_TLAS->GetInstanceModelId(i);

			auto tlasModel = objectsWithModels.at(i);
			auto& triData = ResourceManager<Model>::Get(tlasModel->GetModelPath())->GetLightsData();

			for (const auto primLightData : triData)
			{
				// We need the starting Pos to calculate for this Model and Primitive, which triangle we've got
				const auto startingPos = lightDataCPU.size();
				LightPickData data{modelId, i, primLightData.m_PrimitiveID, (uint32_t)startingPos};

				lightDataCPU.insert(lightDataCPU.end(), primLightData.m_NumTriangles, data);
			}
		}

		ASSERT_MSG(LOG_GRAPHICS, !lightDataCPU.empty(), "We do not support having no light sources in a scene!");

		m_LightData = BufferManager::Create(lightDataCPU.data(),
											sizeof(LightPickData),
											lightDataCPU.size(),
											(BufferFlags::SRV | BufferFlags::DEFAULT_HEAP),
											"Lights");
	}

	std::vector<TlasInstanceData*> ModelManager::CreateTlasInstanceData()
	{
		std::vector<TlasInstanceData*> tlasConstructionData;
		// From that object that'll participate in the scene we get out other data
		// We need TLAS m_ModelId to match the ModelHeapLocation[ModelID] Buffer
		[[maybe_unused]] auto cufsdlkf = GetLevel().GetObjectManager().Size();
		for (auto object : GetLevel().GetObjectManager())
		{
			// Skip game objects that don't have a model assigned
			if (object->GetModelPath().empty())
				continue;

			const auto modelRef = ResourceManager<Model>::Get(object->GetModelPath());
			if (!modelRef.IsLoaded())
				continue;

			const auto instance = new TlasInstanceData; // Deallocated in TLAS Destructor / Destroy()

			instance->m_Blas = &modelRef->GetBLAS();
			instance->m_Transform = object->GetTransform().GetModelMatrix();
			instance->m_ModelId = modelRef->m_ModelIndexID;

			tlasConstructionData.push_back(instance);
		}
		return tlasConstructionData;
	}
	void ModelManager::AnimationImGui()
	{
		// Animations
		if (ImGui::CollapsingHeader("Animations"))
		{
			for (int i = 0; i < m_AnimatedGameObjects.size(); i++)
			{
				ImGui::Checkbox((std::string("Pause ") + std::to_string(i)).c_str(),
								&m_AnimatedGameObjects[i]->GetAnimationControllerPtr()->m_Paused);
				ImGui::DragFloat((std::string("Speed ") + std::to_string(i)).c_str(),
								 &m_AnimatedGameObjects[i]->GetAnimationControllerPtr()->m_Speed,
								 0.1f);
				ImGui::DragFloat((std::string("Time Offset ") + std::to_string(i)).c_str(),
								 &m_AnimatedGameObjects[i]->GetAnimationControllerPtr()->m_TimeOffset,
								 0.01f);
				ImGui::Separator();
			}
		}
	}
} // namespace Ball