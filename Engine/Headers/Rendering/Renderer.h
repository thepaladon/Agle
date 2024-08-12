#pragma once
#include <cstdint>
#include <vector>
#include <string>

#include "ShaderHeaders/GpuModelStruct.h"
#include "ShaderHeaders/CameraGPU.h"
#include "ShaderHeaders/WavefrontStructsGPU.h"

#include "Rendering/LineDrawer.h"
#include "ShaderHeaders/GpuGridStruct.h"
#include "ShaderHeaders/TonemapStructsGPU.h"
#include "Utilities/RenderUtilities.h"

#include <functional>
// Number of Back Buffers we'll use in the Renderer
#define NUM_RT_BUFFERS 2

namespace Ball
{
	class Camera;
	class ModelManager;
	class Texture;
	class Buffer;
	class BackEndRenderer;
	class Window;
	class CommandList;
	class ComputePipelineDescription;
	class ResourceDescriptorHeap;
	class SamplerDescriptorHeap;
	class Denoiser;
	class GameObject;

	struct BloomSettings
	{
		float m_Intensity = 0.75f;
		float m_Radius = 2.0f;
		bool m_Enabled = true;
	};

	class RenderAPI
	{
	public:
		RenderAPI() = default;
		~RenderAPI() = default;

		// Deleting the copy constructor
		RenderAPI(const RenderAPI&) = delete; // Disabled Copy constructor
		RenderAPI(RenderAPI&&) = delete; // Disabled Move constructor
		RenderAPI& operator=(const RenderAPI&) = delete; // Disabled Copy assignment Operator
		RenderAPI& operator=(RenderAPI&&) = delete; // Disabled Move assignment Operator

		ResourceDescriptorHeap* GetResourceDescriptorHeap() { return m_ResourceHeap; }
		SamplerDescriptorHeap* GetSamplerDescriptorHeap() { return m_SamplerHeap; }

		ModelManager* GetModelManager() { return m_ModelManager; }
		uint32_t GetFrameNumber() const { return m_NumTotalFrames; }

		void TakeScreenshot(const std::string& pathAndName, std::function<void()> onComplete = {},
							glm::ivec2 size = glm::ivec2(0));

		static void DrawDebugLine(Line line);
		static void DrawDebugLine(glm::vec3 start, glm::vec3 end, glm::vec3 color = glm::vec3(0.1f, 1.0f, 0.1f));
		static void DrawDebugAABB(class AABB* aabb, glm::vec3 color);

		void SetRunGridShader(bool runGridShader) { m_RunGridShader = runGridShader; }
		GridShaderSettings& GetGridShaderSettings() { return m_GridSettings; }
		void SetSelectedObject(GameObject* object);
		void SetHoveredObject(GameObject* object);
		void SetDispatchOutlineObjects(bool dispatch);

		Denoiser* GetDenoiserPtr() { return m_Denoiser; }

		bool m_DrawWireframe = false;
		glm::vec3 m_CollidersColor = glm::vec3(0.0, 1.0, 0.0);
		glm::vec3 m_WireframeTriangleLinesColor = glm::vec3(0.0, 0.0, 1.0);

		// The path is used as whether a skybox should be loaded or not
		// If the path is not empty, no new skyboxes is loaded
		void LoadSkybox(const std::string& path) { m_UpdateNewSkyboxPath = path; }

		// Resizing
		struct ScreensizeTexturePtr
		{
			Texture* m_Texture = nullptr;
			ResourceDescriptorHeap* m_Heap = nullptr;
			int m_HeapId = -1;
		};

		struct ScreensizeBufferPtr
		{
			Buffer* m_Buffer = nullptr;
			ResourceDescriptorHeap* m_Heap = nullptr;
			int m_HeapId = -1;
		};
		void AddScreensize(Texture* texture);
		void AddScreensize(Buffer* buffer);

		void RemoveScreensize(Texture* texture);
		void RemoveScreensize(Buffer* buffer);

		void MakeScreensizeHeapLink(ScreensizeTexturePtr texture);
		void MakeScreensizeHeapLink(ScreensizeBufferPtr buffer);

		void RemoveScreensizeHeapLink(ScreensizeTexturePtr texture);
		void RemoveScreensizeHeapLink(ScreensizeBufferPtr buffer);

		GameObject* m_SelectedObject = nullptr;
		GameObject* m_HoveredObject = nullptr;
		uint32_t m_SelectedObjectID = 0;
		uint32_t m_HoveredObjectID = 0;

		void WaitForExecution();

	protected:
		friend class Engine;
		friend class RenderModeUI;
		friend class GridSettingsUI;
		friend class BloomSettingsUI;
		friend class TonemapperSettings;
		friend class GpuMarkerVisualizer;
		friend class SceneCompare;

		// Functions Used by Engine
		void Init(Window* window);
		void ImGuiBeginFrame();

		void BeginFrame();
		void Render();

		void Shutdown();

		void OnResize(const uint32_t width, const uint32_t height);

		// For Tools, friend of RenderAPI
		// ReadOnly:
		bool m_AccumFramesEnabled = true;
		bool m_AccumEnabledLastFrame = false;
		uint32_t m_AccumFramesNum = 0;
		uint32_t m_NumTotalFrames = 0;

		// ReadWrite:
		std::vector<Utilities::TimestampData> m_Data;
		GridShaderSettings m_GridSettings;
		TonemapParameters m_TonemapParams;
		uint32_t m_MaxRecursionDepth = 5;
		float m_BrightnessThreshold = 6.f;
		float m_HDRILightingStrength = 0.6f;
		float m_HDRIBackgroundStrength = 0.75f;
		int m_MaxAccumulatedFrames = 512;

		float m_NormalThresholdReSTIR = 0.25;
		float m_DepthThreshold = 0.1f;
		int m_NumSpatialSamples = 5;
		float m_SpatialRadius = 30.f;

		bool m_ShouldClearAccum = false;
		ReStirSettings m_ReStirSettings;
		BloomSettings m_BloomSettings;
		RenderModes m_RenderMode = RenderModes::RM_PATH_TRACE;
		bool m_StationaryCamAccumEnabled = false;
		bool m_ReprojectionEnabled = true;
		bool m_AccumFrameCapEnabled = false;
		bool m_DenoisingEnabled = true;
		bool m_RunGridShader = false;
		bool m_DispatchOutlineObjects = false;

		float3 m_OutlinesSelectedColor = float3(0.0, 1.0, 0.0);
		float3 m_OutlinesHoveredColor = float3(1.0, 1.0, 1.0);
		float m_LineThickness = 1.f;
		float m_TracingDistanceMultiplier = 300.f;

		void ClearAccumFrames()
		{
			m_AccumFramesEnabled = false;
			m_AccumFramesNum = 0;
		}

	private:
		void Resize();
		bool m_ResizeDirty = false;
		std::string m_UpdateNewSkyboxPath = "";
		uint32_t m_ResizeDims[2] = {1, 1};

		// Manually draws all triangles wirefrime line by line
		void DrawTriangleWireframeCPU();

		void ProcessScreenshotLogic();
		void LoadSkyboxLogic();

		std::vector<ScreensizeTexturePtr> m_ScreensizeTextures;
		std::vector<ScreensizeBufferPtr> m_ScreensizeBuffers;

		Camera* m_PreviousCamera = nullptr;
		CameraGPU m_PreviousCamData;

		Denoiser* m_Denoiser = nullptr;

		// Wrappers over most of GPU Code
		BackEndRenderer* m_BackEndAPI;
		CommandList* m_CmdList;

		Texture* m_RenderTargets[NUM_RT_BUFFERS] = {nullptr};
		Texture* m_TransferToRTTexture = nullptr;

		// Resource heap for the bindless ray tracing
		ResourceDescriptorHeap* m_ResourceHeap = nullptr;

		// Sampler heap for the bindless ray tracing
		SamplerDescriptorHeap* m_SamplerHeap = nullptr;

		// Clear screen shader pipeline
		ComputePipelineDescription* m_SimpleRayTracer = nullptr;

		// Grid Shader
		ComputePipelineDescription* m_GridShaderPipeline = nullptr;
		Texture* m_OutputTexture = nullptr;

		Texture* m_SkyTexture = nullptr;
		Buffer* m_GpuModelInfo = nullptr;

		// Manages TLAS, Creation of Models from GameObjects
		ModelManager* m_ModelManager = nullptr;

		// Batch buffers
		Buffer* m_RayBatch[2] = {};
		Buffer* m_RayExtendBatch = nullptr;
		Buffer* m_ShadowRayBatch = nullptr;

		// Atomic Counters
		Buffer* m_NewRaysAtomic = nullptr; // a single uint buffer
		Buffer* m_ShadowRaysAtomic = nullptr; // first uint - num diffuse hits, second num shadow rays from them

		// Counters
		Buffer* m_RayCount = nullptr; // a single uint buffer

		// 1D wavefront output buffer
		Buffer* m_WavefrontOutput = nullptr;

		// ReSTIR Data
		Buffer* m_Reservoirs = nullptr;
		Buffer* m_PrevReservoirs = nullptr;

		Buffer* m_InstanceIDs = nullptr;

		// Material Hit Data (Needed for BRDF calculation in RIS)
		Buffer* m_MaterialHitData = nullptr;

		// Wavefront Pipelines
		ComputePipelineDescription* m_GenerateRaysPipeline = nullptr;
		ComputePipelineDescription* GenerateGenerateRaysPipeline();
		void DispatchGeneratePrimaryRays(int numGroupsX, int numGroupsY, CameraGPU cam) const;

		ComputePipelineDescription* m_ExtendRaysPipeline = nullptr;
		ComputePipelineDescription* GenerateExtendRaysPipeline();
		void DispatchExtendRays(uint32_t numGroups1D, uint32_t wavefrontBounceNum) const;

		ComputePipelineDescription* m_ShadeRaysPipeline = nullptr;
		ComputePipelineDescription* GenerateShadeRaysPipeline();
		void DispatchShadeRays(uint32_t numGroups1D, uint32_t wavefrontBounceNum, GameplaySkyMat skyMat,
							   float coneSpreadAngle) const;

		ComputePipelineDescription* m_DirectIllumPipeline = nullptr;
		ComputePipelineDescription* GenerateDirectIllumPipeline();
		void DispatchDirectIllum(uint32_t numGroups1D, uint32_t wavefrontBounceNum) const;

		ComputePipelineDescription* m_ConnectRaysPipeline = nullptr;
		ComputePipelineDescription* GenerateConnectRaysPipeline();
		void DispatchConnectRays(uint32_t numGroups1D, uint32_t wavefrontBounceNum) const;

		ComputePipelineDescription* m_ReprojectReSTIRPipeline = nullptr;
		ComputePipelineDescription* GenerateReprojectReSTIRPipeline();
		void DispatchReprojectReSTIR(uint32_t numGroups1D) const;

		ComputePipelineDescription* m_SpatialReSTIRPipeline = nullptr;
		ComputePipelineDescription* GenerateSpatialReSTIRPipeline();
		void DispatchSpatialReSTIR(uint32_t numGroups1D) const;

		ComputePipelineDescription* m_ShadeReSTIRPipeline = nullptr;
		ComputePipelineDescription* GenerateShadeReSTIRPipeline();
		void DispatchShadeReSTIR(uint32_t numGroups1D, uint32_t wavefrontBounceNum) const;

		ComputePipelineDescription* m_FinalizeRTPipeline = nullptr;
		ComputePipelineDescription* GenerateFinalizePipeline();
		void DispatchFinalize(int numGroupsX, int numGroupsY) const;

		// Blending (for UI atm)
		ComputePipelineDescription* m_BlendingPipeline = nullptr;

		// Blending (for UI atm)
		ComputePipelineDescription* m_TonemappingPipeline = nullptr;

		ComputePipelineDescription* m_OutlineObjectsPipeline = nullptr;
		ComputePipelineDescription* GenerateOutlineObjectsPipeline();
		void DispatchOutlineObjects(uint32_t numGroups1D);

		ComputePipelineDescription* m_BloomUpsamplePipeline = nullptr;
		ComputePipelineDescription* m_BloomDownsamplePipeline = nullptr;
		void AddBloomTexturesToRDH();
		Texture* m_BloomIntermediateTextures[NUM_BLOOM] = {};

		void LoadBlueNoiseTextures();
		Texture* m_BlueNoiseTextures[NUM_BLUENOISE];

		struct
		{
			bool requested = false;
			std::string path = "null";
			glm::ivec2 size = glm::ivec2(0, 0);
			std::function<void()> onComplete{};
		} m_ScreenShot;
	};
} // namespace Ball