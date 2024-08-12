#pragma once

#include <unordered_map>
#include <string>

#include "ResourceManager/Resource.h"
#include "ShaderHeaders/GpuModelStruct.h"

namespace Ball
{
	struct TlasInstanceData;
	class TLAS;
	class Buffer;
	class ResourceDescriptorHeap;
	class Model;
	class GameObject;

	class ModelManager
	{
	public:
		ModelManager();
		~ModelManager();

		void RequestReloadModels();
		void ProcessModelLoadingQueue(ResourceDescriptorHeap& rdhToStoreModels);
		void UpdateInstanceTransformsBuffer();
		void UpdateAnimations();
		void UpdateAnimationsGPU();
		void AnimationImGui();
		// Adds a Models' Buffers and Textures and saves them as added
		ModelHeapLocation AddModel(ResourceDescriptorHeap& rdhToStoreModels, const Resource<Model> model);

		// Getters
		const TLAS& GetTLAS() const { return *m_TLAS; }
		TLAS& GetTLASRef() { return *m_TLAS; }
		Buffer* GetLightData() const { return m_LightData; }
		const uint32_t GetNumLightsInScene() const;

		bool ReloadingModels() const { return m_ReloadModels; }

	private:
		void RebuildTLAS(ResourceDescriptorHeap& rdhToStoreTLASBuffers);

		void FillInLights();

		std::vector<TlasInstanceData*> CreateTlasInstanceData();

		bool m_ReloadModels = false;

		// Model related buffers for Rendering
		Buffer* m_ModelHeapLocationBuffer = nullptr;
		Buffer* m_InstanceTransformsBuffer = nullptr;
		Buffer* m_LightData = nullptr;

		// TLAS and Instances Ownership
		TLAS* m_TLAS = nullptr;

		// Animations
		std::vector<GameObject*> m_AnimatedGameObjects;
	};
} // namespace Ball
