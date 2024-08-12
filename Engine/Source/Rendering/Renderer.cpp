#include "Rendering/Renderer.h"
#include "Engine.h"
#include "FileIO.h"
#include "Input/Input.h"
#include "Window.h"

#include "Rendering/Denoiser.h"
#include "Rendering/BackEndRenderer.h"
#include "Rendering/BEAR/CommandList.h"
#include "Rendering/BEAR/ComputePipelineDescription.h"
#include "Rendering/BEAR/ResourceDescriptorHeap.h"
#include "Rendering/BEAR/SamplerDescriptorHeap.h"
#include "Rendering/BEAR/ShaderLayout.h"
#include "Rendering/BEAR/TLAS.h"
#include "Rendering/BEAR/Texture.h"
#include "Rendering/ModelLoading/ModelManager.h"
#include "Rendering/ModelLoading/Primitive.h"

#include "GameObjects/Types/Camera.h"
#include "Utilities/LaunchParameters.h"
#include "Levels/Level.h"
#include "ResourceManager/ResourceManager.h"
#include "Utilities/RenderUtilities.h"

#include <ImGui/imgui.h>
#include <TinyglTF/tiny_gltf.h>
#include <glm/ext/matrix_transform.hpp>

#include <stb/stb_image_write.h>
#include "ShaderHeaders/BloomStructsGPU.h"

namespace Ball
{
	inline TonemapParameters SetDefaultTonemapValues();

	LineDrawer* g_LineDrawer = nullptr;
	static bool g_DrawLines = true;
	static bool g_PreparingScreenshotData = false;
	constexpr int g_RDHSize = 65536;

	void RenderAPI::Init(Window* window)
	{
		if (LaunchParameters::Contains("Headless"))
			return;

		m_ReStirSettings.m_RISRandomLights = 16; // Paper recommends 16
		m_ReStirSettings.m_UseReSTIR = 4; // RIS + Temporal + Spatial by default
		m_ReStirSettings.m_CurrentLightClamp = 20; // Look at combobox in RenderModeUI for more info

		m_TonemapParams = SetDefaultTonemapValues();

		if (LaunchParameters::Contains("Wireframe"))
			m_DrawWireframe = true;

		if (LaunchParameters::Contains("Raytracing"))
			m_RenderMode = RenderModes::RM_RAY_TRACE;

		// Initialize Back End Renderer API(s)
		m_BackEndAPI = new BackEndRenderer();
		m_CmdList = new CommandList();
		m_SamplerHeap = new SamplerDescriptorHeap();
		m_SimpleRayTracer = new ComputePipelineDescription();
		m_BlendingPipeline = new ComputePipelineDescription();
		m_Denoiser = new Denoiser();

		// Init Resource heap
		m_ResourceHeap = new ResourceDescriptorHeap(g_RDHSize);
		// Hacky fix to be able to resize on first frame, it copies impl from Render()
		m_ResourceHeap->ReserveSpace(RDH_HEADER_SIZE);

		const auto windowWidth = window->GetWidth();
		const auto windowHeight = window->GetHeight();

		TextureSpec renderTargetSpec;
		renderTargetSpec.m_Width = windowWidth;
		renderTargetSpec.m_Height = windowHeight;
		renderTargetSpec.m_Format = TextureFormat::R8G8B8A8_UNORM;
		renderTargetSpec.m_Type = TextureType::RENDER_TARGET;
		renderTargetSpec.m_Flags = TextureFlags::NONE;

		for (int i = 0; i < NUM_RT_BUFFERS; i++)
			m_RenderTargets[i] = new Texture(nullptr, renderTargetSpec, "Render Target [" + std::to_string(i) + "]");

		renderTargetSpec.m_Type = TextureType::RW_TEXTURE;
		renderTargetSpec.m_Flags = TextureFlags::ALLOW_UA | TextureFlags::SCREENSIZE;
		m_TransferToRTTexture = new Texture(nullptr, renderTargetSpec, "Intermediate Texture");

		// Initialize Command List and Render Targets (as Textures)
		m_CmdList->Initialize(m_TransferToRTTexture);

		// Initializing global directX objects
		m_BackEndAPI->Initialize(window, m_RenderTargets, m_CmdList);
		m_SamplerHeap->Initialize(36, true);

		m_Denoiser->Initialize(windowWidth, windowHeight);
		// Test Shader Layout that matches registers definition in the Shader
		ShaderLayout basicRayShader;
		basicRayShader.Add32bitConstParameter(sizeof(CameraGPU) / 4);
		basicRayShader.AddParameter(ShaderParameter::SRV);
		basicRayShader.Add32bitConstParameter(sizeof(DebugSettings) / 4);
		basicRayShader.Initialize();

		// Initialize Compute Pipeline Descriptor which we use to bind shaders at runtime
		m_SimpleRayTracer->Initialize("RayTraceTest", basicRayShader);

		TextureSpec intermediateOutputSpec;
		intermediateOutputSpec.m_Width = windowWidth;
		intermediateOutputSpec.m_Height = windowHeight;
		intermediateOutputSpec.m_Format = TextureFormat::R32_G32_B32_A32_FLOAT;
		intermediateOutputSpec.m_Type = TextureType::RW_TEXTURE;
		intermediateOutputSpec.m_Flags = (TextureFlags::ALLOW_UA | TextureFlags::SCREENSIZE);
		m_OutputTexture = new Texture(nullptr, intermediateOutputSpec, "Intermediate Output Texture");

		// Add samplers to (another) Bindless Heapkloofendal_48d_partly_cloudy_puresky_4k
		LoadBlueNoiseTextures();

		m_PreviousCamera = new Camera();

		m_ModelManager = new ModelManager();

		//  Blending PS
		{
			ShaderLayout blendShaderLayout;
			blendShaderLayout.Add32bitConstParameter(2); // Texture Dimensions
			blendShaderLayout.Initialize();

			// Initialize Compute Pipeline Descriptor which we use to bind shaders at runtime
			m_BlendingPipeline->Initialize("Blending", blendShaderLayout);
		}

		m_TonemappingPipeline = new ComputePipelineDescription();
		ShaderLayout tonemapperLayout;
		tonemapperLayout.Add32bitConstParameter(sizeof(TonemapParameters) / 4);
		tonemapperLayout.AddParameter(ShaderParameter::SRV);
		tonemapperLayout.Initialize();
		m_TonemappingPipeline->Initialize("Tonemapping", tonemapperLayout);

		// BLOOM STUFF
		m_BloomDownsamplePipeline = new ComputePipelineDescription();
		ShaderLayout downsample;
		downsample.Add32bitConstParameter(sizeof(DownsampleData) / 4);
		downsample.Initialize();
		m_BloomDownsamplePipeline->Initialize("BloomDownsample", downsample);

		m_BloomUpsamplePipeline = new ComputePipelineDescription();
		ShaderLayout upsample;
		upsample.Add32bitConstParameter(sizeof(UpsampleData) / 4);
		upsample.Initialize();
		m_BloomUpsamplePipeline->Initialize("BloomUpsample", upsample);

		auto bloomMips = m_OutputTexture->CalculateMipsNum() - 1;
		for (int i = 0; i < bloomMips; i++)
		{
			std::string name = "Intermediate Output Texture Mip " + std::to_string(i);
			intermediateOutputSpec.m_Flags = TextureFlags::ALLOW_UA; // Remove screensize flag, resized manually
			intermediateOutputSpec.m_Height = glm::max<int>(intermediateOutputSpec.m_Height >> 1, 1);
			intermediateOutputSpec.m_Width = glm::max<int>(intermediateOutputSpec.m_Width >> 1, 1);

			m_BloomIntermediateTextures[i] = new Texture(nullptr, intermediateOutputSpec, name);
		}

		// BLOOM STUF END

		BufferFlags defaultUAV = (BufferFlags::UAV | BufferFlags::ALLOW_UA | BufferFlags::DEFAULT_HEAP);

		const auto numPrimaryRays = windowWidth * windowHeight;

		m_InstanceIDs = new Buffer(
			nullptr, sizeof(uint32_t), numPrimaryRays, (defaultUAV | BufferFlags::SCREENSIZE), "Instance IDs");

		m_OutlineObjectsPipeline = GenerateOutlineObjectsPipeline();

		// ---------- WAVEFRONT -----------------
		{
			m_GenerateRaysPipeline = GenerateGenerateRaysPipeline();
			m_ExtendRaysPipeline = GenerateExtendRaysPipeline();
			m_ShadeRaysPipeline = GenerateShadeRaysPipeline();
			m_DirectIllumPipeline = GenerateDirectIllumPipeline();
			m_ConnectRaysPipeline = GenerateConnectRaysPipeline();
			m_ReprojectReSTIRPipeline = GenerateReprojectReSTIRPipeline();
			m_SpatialReSTIRPipeline = GenerateSpatialReSTIRPipeline();
			m_ShadeReSTIRPipeline = GenerateShadeReSTIRPipeline();
			m_FinalizeRTPipeline = GenerateFinalizePipeline();

			// Create Wavefront buffers

			for (int i = 0; i < std::size(m_RayBatch); i++)
			{
				std::string name = "Ray Batch " + std::to_string(i);
				m_RayBatch[i] =
					new Buffer(nullptr, sizeof(Ray), numPrimaryRays, (defaultUAV | BufferFlags::SCREENSIZE), name);
			}

			m_RayExtendBatch = new Buffer(nullptr,
										  sizeof(ExtendResult),
										  numPrimaryRays,
										  (defaultUAV | BufferFlags::SCREENSIZE),
										  "Extended Batch");

			m_ShadowRayBatch = new Buffer(
				nullptr, sizeof(ShadowRay), numPrimaryRays, (defaultUAV | BufferFlags::SCREENSIZE), "Shadow Batch");

			// Atomic Counters
			m_NewRaysAtomic = new Buffer(nullptr, sizeof(uint32_t), 1, defaultUAV, "Atomic New Rays");
			m_ShadowRaysAtomic = new Buffer(nullptr, sizeof(uint32_t), 2, defaultUAV, "Atomic Shadow Rays");

			// Counters
			m_RayCount = new Buffer(&numPrimaryRays, sizeof(uint32_t), 1, defaultUAV, "Ray Count");

			m_WavefrontOutput = new Buffer(
				nullptr, sizeof(glm::vec4), numPrimaryRays, (defaultUAV | BufferFlags::SCREENSIZE), "Wavefront Output");

			m_MaterialHitData = new Buffer(nullptr,
										   sizeof(MaterialHitData),
										   numPrimaryRays,
										   (defaultUAV | BufferFlags::SCREENSIZE),
										   "Material Hit Data");
		}

		// ---------- REPROJECT / DENOISE -----------------
		{
			m_Denoiser->GenerateReprojectPipeline();
			m_Denoiser->GenerateCalculateWeightPipeline();
			m_Denoiser->GenerateATrousPipeline();
			m_Denoiser->GenerateModulatePipeline();
		}
		// ------------------------------------------

		g_LineDrawer = new LineDrawer();
		g_LineDrawer->Init(windowWidth, windowHeight);

		m_GridShaderPipeline = new ComputePipelineDescription();
		ShaderLayout gridShaderLayout;
		gridShaderLayout.Add32bitConstParameter(sizeof(CameraGPU) / 4);
		gridShaderLayout.Add32bitConstParameter(sizeof(GridShaderSettings) / 4);
		gridShaderLayout.AddParameter(ShaderParameter::SRV);
		gridShaderLayout.Initialize();
		m_GridShaderPipeline->Initialize("Grid", gridShaderLayout);
		m_GridSettings = {0.1f, glm::vec2(0.1, 0.1), 0.5f, glm::vec3(0.0, 0.0, 0.0)};

		// We need to execute after making all resources to upload them to GPU (only Windows)
		m_CmdList->Execute();
	}

	void RenderAPI::ImGuiBeginFrame()
	{
		m_BackEndAPI->ImguiBeginFrame();
	}
	void RenderAPI::Resize()
	{
		if (m_ResizeDirty)
		{
			m_ResizeDirty = false;

			m_BackEndAPI->WaitForCmdQueueExecute();
			m_CmdList->Reset();

			uint32_t const width = m_ResizeDims[0];
			uint32_t const height = m_ResizeDims[1];

			for (auto& texItem : m_ScreensizeTextures)
			{
				texItem.m_Texture->Resize(width, height);
				if (texItem.m_Heap != nullptr && texItem.m_HeapId >= 0)
				{
					texItem.m_Heap->Switch(*texItem.m_Texture, texItem.m_HeapId);
				}
			}

			for (auto& buffItem : m_ScreensizeBuffers)
			{
				buffItem.m_Buffer->Resize(width * height);
				if (buffItem.m_Heap != nullptr && buffItem.m_HeapId >= 0)
				{
					buffItem.m_Heap->Switch(*buffItem.m_Buffer, buffItem.m_HeapId);
				}
			}

			m_BackEndAPI->ResizeFrameBuffers(width, height);

			auto spec = m_OutputTexture->GetSpec();
			auto bloompMips = m_OutputTexture->CalculateMipsNum() - 1;
			for (int i = 0; i < bloompMips; i++)
			{
				spec.m_Height = glm::max<int>(1, spec.m_Height >> 1);
				spec.m_Width = glm::max<int>(1, spec.m_Width >> 1);

				if (spec.m_Width == 0 || spec.m_Height == 0)
				{
					ASSERT_MSG(LOG_GRAPHICS, false, "bloomMips is incorrect");
					break;
				}

				// Determine whether the texture exists or not
				if (m_BloomIntermediateTextures[i] == nullptr)
				{
					// Texture doesn't exist, add it.
					std::string name = "Intermediate Output Texture Mip " + std::to_string(i);
					m_BloomIntermediateTextures[i] = new Texture(nullptr, spec, name);
				}
				else
				{
					// Texture exists, resize it
					m_BloomIntermediateTextures[i]->Resize(spec.m_Width, spec.m_Height);
				}
			}

			AddBloomTexturesToRDH();

			m_CmdList->Execute();

			m_NumTotalFrames = 0;
			m_AccumFramesNum = 0;
		}
	}

	void RenderAPI::WaitForExecution()
	{
		if (LaunchParameters::Contains("Headless"))
			return;
		m_BackEndAPI->WaitForCmdQueueExecute();
	}

	void RenderAPI::OnResize(const uint32_t width, const uint32_t height)
	{
		m_ResizeDirty = true;

		// Needed for ultralight data to be alighned to 512 bytes per row
		m_ResizeDims[0] = width;
		m_ResizeDims[1] = height;
	}

	void RenderAPI::TakeScreenshot(const std::string& pathAndName, std::function<void()> onComplete, glm::ivec2 size)
	{
		m_ScreenShot.requested = true;
		m_ScreenShot.path = pathAndName;
		m_ScreenShot.size = size;
		m_ScreenShot.onComplete = onComplete;
	}

	void RenderAPI::DrawDebugLine(Line line)
	{
		if (g_DrawLines)
			g_LineDrawer->AddLine(line);
	}

	void RenderAPI::DrawDebugLine(glm::vec3 start, glm::vec3 end, glm::vec3 color)
	{
		if (g_DrawLines)
			g_LineDrawer->AddLine({start, end, color});
	}

	// TEMPORARY SOLUTION - replace with the dirty flag, when cam transform is added
	bool CamChanged(CameraGPU& cur, CameraGPU& last)
	{
		return (cur.m_Pos != last.m_Pos || cur.m_ImagePlanePos != last.m_ImagePlanePos || cur.m_xAxis != last.m_xAxis ||
				cur.m_yAxis != last.m_yAxis);
	}

	void RenderAPI::BeginFrame()
	{
		if (LaunchParameters::Contains("Headless"))
			return;

		// Wait for Previous frame to Complete
		m_BackEndAPI->WaitForCmdQueueExecute();
		m_BackEndAPI->BeginFrame();
	}

	void RenderAPI::Render()
	{
		if (LaunchParameters::Contains("Headless"))
			return;

		auto renderStartTs = Utilities::PushGPUTimestamp(m_CmdList, "[GL] Render Loop");

		auto sceneSetupTSs =
			Utilities::PushGPUTimestamp(m_CmdList, "[GL] SceneUpdate - TLAS Rebuild, Blas Refits, [...]");

		Camera* activeCamera = Camera::GetActiveCamera();

		ASSERT_MSG(LOG_GRAPHICS, activeCamera != nullptr, "No active camera is set in the scene!");

		const auto windowWidth = m_OutputTexture->GetWidth();
		const auto windowHeight = m_OutputTexture->GetHeight();

		ProcessScreenshotLogic();

		LoadSkyboxLogic();

		// Add Models from Queue if necessary
		if (m_ModelManager->ReloadingModels())
		{
			delete m_ResourceHeap;
			m_ResourceHeap = new ResourceDescriptorHeap(g_RDHSize);

			// Add Reserved Header of our Bindless Heap (MODEL_DATA_HEAP_OFFSET)
			// ORDER IS IMPORTANT! Check GpuModelStruct
			m_ResourceHeap->ReserveSpace(RDH_HEADER_SIZE);
			m_ResourceHeap->Switch(*m_TransferToRTTexture, RDH_TRANSFER); // Output Texture
			m_ResourceHeap->Switch(*m_SkyTexture, RDH_SKYBOX); // Sky Texture
			// HACK : Fix this gap
			// m_ResourceHeap->Switch(*GetUISystem().GetTexture(), RDH_ULTRALIGHT); // UI Texture
			m_ResourceHeap->Switch(*m_OutputTexture, RDH_OUTPUT); // Output Texture

			AddBloomTexturesToRDH();

			ASSERT_MSG(LOG_GRAPHICS,
					   m_ResourceHeap->GetNumElements() == RDH_HEADER_SIZE,
					   "RDH Heap Header doesn't match macro");

			m_ModelManager->ProcessModelLoadingQueue(*m_ResourceHeap);

			// Hacky fix to rest the GPU buffers for ReSTIR whenever we change level
			delete m_Reservoirs;
			delete m_PrevReservoirs;
			BufferFlags defaultUAV = (BufferFlags::UAV | BufferFlags::ALLOW_UA | BufferFlags::DEFAULT_HEAP);
			const auto numPrimaryRays = windowWidth * windowHeight;
			m_Reservoirs = new Buffer(
				nullptr, sizeof(Reservoir), numPrimaryRays, (defaultUAV | BufferFlags::SCREENSIZE), "ReSTIR Reservoir");
			m_PrevReservoirs = new Buffer(
				nullptr, sizeof(Reservoir), numPrimaryRays, (defaultUAV | BufferFlags::SCREENSIZE), "ReSTIR Reservoir");
		}

		m_ResourceHeap->Switch(*m_BlueNoiseTextures[m_NumTotalFrames & NUM_BLUENOISE - 1],
							   RDH_BLUENOISE); // Blue Noise Texture

		// UPDATES
		auto cam = activeCamera->GetGPUCam(windowWidth, windowHeight);
		auto prevCam = m_PreviousCamera->GetGPUCam(windowWidth, windowHeight);
		CameraGPU cams[2] = {prevCam, cam};
		m_Denoiser->GetBufferAt(DenoiseBuffers::CAMERAS)->UpdateData(&cams, sizeof(cams));

		m_ModelManager->UpdateAnimationsGPU();
		m_ModelManager->UpdateInstanceTransformsBuffer();
		m_ModelManager->GetTLASRef().Update();

		// Automatic frame accum for non-animated scene (TEMPORARY SOLUTION)
		if (m_StationaryCamAccumEnabled)
		{
			m_AccumFramesEnabled = CamChanged(cam, m_PreviousCamData) ? false : true;
		}
		else
		{
			m_AccumFramesEnabled = false;
		}

		m_NumTotalFrames++;

		// Reset counter if we're not accumulating
		if (m_ShouldClearAccum || !m_AccumFramesEnabled)
		{
			ClearAccumFrames();
			m_ShouldClearAccum = false;
		}

		m_Denoiser->GetBufferAt(DenoiseBuffers::VIEW_PYRAMID)
			->UpdateData(&m_PreviousCamera->m_ViewPyramid, sizeof(ViewPyramid));

#ifndef SHIPPING
		auto& input = GetEngine().GetInput();
		if (input.GetActionDown("On_Off_Debug_Lines"))
		{
			g_DrawLines = !g_DrawLines;
		}
#endif

		// Set Bindless Descriptor Heaps
		m_CmdList->SetDescriptorHeaps(m_ResourceHeap, m_SamplerHeap);

		// Calculate the number of thread groups needed to cover the RenderTarget
		constexpr float threadGroupSize = 16.0;
		constexpr float threadGroupSize1D = 256.f;
		const int numGroupsX = int(ceil(float(windowWidth) / threadGroupSize));
		const int numGroupsY = int(ceil(float(windowHeight) / threadGroupSize));
		const int numGroups1D = int(ceil(float(windowWidth * windowHeight) / threadGroupSize1D));

		Utilities::PopGPUTimestamp(m_CmdList, sceneSetupTSs);

		if (m_RenderMode != RenderModes::RM_PATH_TRACE)
		{
			m_CmdList->SetComputePipeline(*m_SimpleRayTracer);

			// Binding Register Resources
			m_CmdList->BindResource32BitConstants(0, &cam, sizeof(CameraGPU) / 4);
			m_CmdList->BindResourceSRV(1, m_ModelManager->GetTLASRef());
			m_CmdList->BindResource32BitConstants(2, &m_RenderMode, sizeof(DebugSettings) / 4);
			m_CmdList->Dispatch(numGroupsX, numGroupsY, 1, true);
		}
		else
		// -------- WAVEFRONT ------------
		{
			if (m_AccumFramesNum < m_MaxAccumulatedFrames || !m_AccumFrameCapEnabled)
			{
				m_AccumFramesNum++;

				// Generate
				DispatchGeneratePrimaryRays(numGroupsX, numGroupsY, cam);

				// Bounces
				for (uint32_t i = 0; i < m_MaxRecursionDepth; i++)
				{
					const auto loopTs = Utilities::PushGPUTimestamp(m_CmdList, "[GL] Wavefront " + std::to_string(i));

					// Hack : Gameplay skybox was the code for the special rotating skybox that we now no longer have
					bool gameplaySkybox = false;
					GameplaySkyMat skyMat = {activeCamera->GetGameplaySkyboxRotMat(),
											 uint32_t(gameplaySkybox),
											 m_HDRILightingStrength,
											 m_HDRIBackgroundStrength};
					// Extend
					DispatchExtendRays(numGroups1D, i);

					// Shade
					DispatchShadeRays(numGroups1D, i, skyMat, cam.m_PrimaryConeSpreadAngle);

					// Direct Illumination
					DispatchDirectIllum(numGroups1D, i);

					// Connect
					DispatchConnectRays(numGroups1D, i);

					// Primary rays evaluations
					if (i == 0)
					{
						// Temporal RIS + Reproject pixel calc
						DispatchReprojectReSTIR(numGroups1D);

						// Spatial Reuse
						DispatchSpatialReSTIR(numGroups1D);

						// Add Direct Lighting
						DispatchShadeReSTIR(numGroups1D, i);
					}

					Utilities::PopGPUTimestamp(m_CmdList, loopTs);
				}

				DispatchFinalize(numGroupsX, numGroupsY);

				// ------------- REPROJECT -----------------------
				if (!m_AccumFramesEnabled && m_ReprojectionEnabled)
				{
					m_Denoiser->DispatchReproject(numGroups1D, m_CmdList, m_PrevReservoirs, m_Reservoirs);

					if (m_DenoisingEnabled)
					{
						m_Denoiser->DispatchCalculateWeight(numGroups1D, cam, m_CmdList);
						m_Denoiser->DispatchATrous(numGroups1D, cam, m_CmdList);
						m_Denoiser->DispatchModulate(numGroups1D, cam, m_CmdList);
					}
				}
			}

			if (m_DispatchOutlineObjects && !m_ScreenShot.requested)
				DispatchOutlineObjects(numGroups1D);
		}

		if (m_RunGridShader && !m_ScreenShot.requested)
		{
			const auto gridShader = Utilities::PushGPUTimestamp(m_CmdList, "Grid Shader");
			m_CmdList->SetComputePipeline(*m_GridShaderPipeline);
			// Binding Register Resources
			m_CmdList->BindResource32BitConstants(0, &cam, sizeof(CameraGPU) / 4);
			m_CmdList->BindResource32BitConstants(1, &m_GridSettings, sizeof(GridShaderSettings) / 4);
			m_CmdList->BindResourceSRV(2, *m_Denoiser->GetBufferAt(DenoiseBuffers::CURRENT_DEPTH));
			m_CmdList->Dispatch(numGroupsX, numGroupsY, 1, true);
			Utilities::PopGPUTimestamp(m_CmdList, gridShader);
		}

		// Bloom Start
		// Downsample
		if (m_BloomSettings.m_Enabled)
		{
			auto bloomTs = Utilities::PushGPUTimestamp(m_CmdList, "Bloom");
			auto bloompMips = m_OutputTexture->CalculateMipsNum() - 1;
			for (int targetMip = 1; targetMip <= bloompMips; targetMip++)
			{
				DownsampleData dsdata;
				auto targetMipIndex = targetMip - 1;
				Texture* dstTexture = m_BloomIntermediateTextures[targetMipIndex];
				Texture* srcTexture = nullptr;
				bool useKaris;
				if (targetMip == 1)
				{
					srcTexture = m_OutputTexture;
					useKaris = true;
				}
				else
				{
					srcTexture = m_BloomIntermediateTextures[targetMipIndex - 1];
					useKaris = false;
				}

				dsdata.m_MipEvaulating = targetMip;
				dsdata.m_SrcDimension = (srcTexture->GetHeight() & 1) << 1 | (srcTexture->GetWidth() & 1);
				const float texelSizeX = 1.f / dstTexture->GetWidth();
				const float texelSizeY = 1.f / dstTexture->GetHeight();
				const glm::vec2 texelSize = {texelSizeX, texelSizeY};
				dsdata.m_TexelSize = texelSize;
				dsdata.m_UseKaris13Fetch = useKaris;

				m_CmdList->SetComputePipeline(*m_BloomDownsamplePipeline);
				m_CmdList->SetDescriptorHeaps(m_ResourceHeap, m_SamplerHeap);
				m_CmdList->BindResource32BitConstants(0, &dsdata, sizeof(DownsampleData) / 4);
				const int dispatchX = int(ceil(float(dstTexture->GetWidth()) / threadGroupSize));
				const int dispatchY = int(ceil(float(dstTexture->GetHeight()) / threadGroupSize));
				m_CmdList->Dispatch(dispatchX, dispatchY, 1, true);
			}

			// Upsample
			for (int srcTextureIdx = bloompMips - 1; srcTextureIdx >= 0; srcTextureIdx--)
			{
				UpsampleData updata;
				auto dstMipIndex = srcTextureIdx - 1;
				Texture* dstTexture = nullptr;
				Texture* srcTexture = m_BloomIntermediateTextures[srcTextureIdx];

				// Mip Level of -1 of m_BloomIntermediateTextures is m_OutputTexture
				if (dstMipIndex < 0)
				{
					dstTexture = m_OutputTexture;
				}
				else
				{
					dstTexture = m_BloomIntermediateTextures[dstMipIndex];
				}

				updata.m_Intensity = m_BloomSettings.m_Intensity;
				updata.m_InvMipCount = 1.0f / (float)(bloompMips); // Not sure

				const glm::vec2 invSrcDims = {1.f / (float)srcTexture->GetWidth(),
											  1.f / (float)srcTexture->GetHeight()};
				updata.m_InvSrcDims = invSrcDims;
				updata.m_Radius = m_BloomSettings.m_Radius < 1.0f ? 1 : static_cast<uint>(m_BloomSettings.m_Radius);
				updata.m_MipEvaulating = dstMipIndex + 1; // Because the RDH is offset by one

				const glm::vec2 texelSize = {(float)srcTexture->GetWidth() / (float)dstTexture->GetWidth(),
											 (float)srcTexture->GetHeight() / (float)dstTexture->GetHeight()};

				updata.m_TexelSize = texelSize;

				m_CmdList->SetComputePipeline(*m_BloomUpsamplePipeline);
				m_CmdList->SetDescriptorHeaps(m_ResourceHeap, m_SamplerHeap);
				m_CmdList->BindResource32BitConstants(0, &updata, sizeof(UpsampleData) / 4);
				const int dispatchX = int(ceil(float(dstTexture->GetWidth()) / threadGroupSize));
				const int dispatchY = int(ceil(float(dstTexture->GetHeight()) / threadGroupSize));
				m_CmdList->Dispatch(dispatchX, dispatchY, 1, true);
			}
			Utilities::PopGPUTimestamp(m_CmdList, bloomTs);
		}
		// Bloom End

		// Tonemap & write to m_TransferToRTTexture
		auto tonemappingTs = Utilities::PushGPUTimestamp(m_CmdList, "Tonemapping");
		m_CmdList->SetComputePipeline(*m_TonemappingPipeline);
		m_CmdList->SetDescriptorHeaps(m_ResourceHeap, m_SamplerHeap);
		m_CmdList->BindResource32BitConstants(0, &m_TonemapParams, sizeof(TonemapParameters) / 4);
		m_CmdList->BindResourceSRV(1, *m_Denoiser->GetBufferAt(DenoiseBuffers::EMISSION));
		m_CmdList->Dispatch(numGroupsX, numGroupsY, 1, true);
		Utilities::PopGPUTimestamp(m_CmdList, tonemappingTs);

		if (m_ScreenShot.requested)
		{
			m_TransferToRTTexture->RequestDataOnCPU();
			g_PreparingScreenshotData = true;
		}

		auto uiRenderTs = Utilities::PushGPUTimestamp(m_CmdList, "Ui Render");
		Utilities::PopGPUTimestamp(m_CmdList, uiRenderTs);

		// ------------- COPY -----------------------
		// Copy information to the Output RenderTarget
		const auto frameIdx = m_BackEndAPI->GetCurrentBackBufferIndex();
		auto copyToRtTs = Utilities::PushGPUTimestamp(m_CmdList, "Copy To RenderTarget");
		PUSH_GPU_MARKER(m_CmdList, "Copy To RenderTarget");
		m_CmdList->CopyResource(*m_RenderTargets[frameIdx], *m_TransferToRTTexture);
		POP_GPU_MARKER(m_CmdList);
		Utilities::PopGPUTimestamp(m_CmdList, copyToRtTs);

		PUSH_GPU_MARKER(m_CmdList, "End Tracing");
		m_BackEndAPI->EndTracing();
		POP_GPU_MARKER(m_CmdList);

		if (m_DrawWireframe)
		{
			DrawTriangleWireframeCPU();
		}

		auto lineTs = Utilities::PushGPUTimestamp(m_CmdList, "Line Rendering");
		g_LineDrawer->DrawLines(activeCamera);
		Utilities::PopGPUTimestamp(m_CmdList, lineTs);

		m_PreviousCamData = cam;

		auto imguiRenderTs = Utilities::PushGPUTimestamp(m_CmdList, "ImGui Render");
		PUSH_GPU_MARKER(m_CmdList, "ImGui End Frame");
		m_BackEndAPI->ImguiEndFrame();
		POP_GPU_MARKER(m_CmdList);
		Utilities::PopGPUTimestamp(m_CmdList, imguiRenderTs);

		PUSH_GPU_MARKER(m_CmdList, "End Frame");
		m_BackEndAPI->EndFrame();
		POP_GPU_MARKER(m_CmdList);

		Utilities::PopGPUTimestamp(m_CmdList, renderStartTs);
		Utilities::SaveGPUTimestampData(m_CmdList);
		m_Data = Utilities::ProcessReadbackBuffer();
		m_CmdList->Execute();

		m_BackEndAPI->PresentFrame();

		// Camera is updated in game loop which happens before render
		*m_PreviousCamera = *activeCamera;
		m_AccumEnabledLastFrame = m_AccumFramesEnabled;

		Resize();
	}

	void RenderAPI::Shutdown()
	{
		if (LaunchParameters::Contains("Headless"))
			return;

		delete m_ModelManager;
		delete m_CmdList;
		delete m_ResourceHeap;
		delete m_SamplerHeap;
		delete m_GridShaderPipeline;
		delete m_SimpleRayTracer;
		delete m_OutputTexture;
		delete m_SkyTexture;
		delete m_Denoiser;
		delete m_TransferToRTTexture;

		for (auto& m_RenderTarget : m_RenderTargets)
			delete m_RenderTarget;

		for (auto& rb : m_RayBatch)
			delete rb;

		for (auto& bloomTex : m_BloomIntermediateTextures)
			delete bloomTex;

		for (auto& noiseTex : m_BlueNoiseTextures)
			delete noiseTex;

		delete m_BloomDownsamplePipeline;
		delete m_BloomUpsamplePipeline;

		delete m_GpuModelInfo;
		delete m_RayExtendBatch;
		delete m_ShadowRayBatch;
		delete m_NewRaysAtomic;
		delete m_ShadowRaysAtomic;
		delete m_RayCount;
		delete m_WavefrontOutput;
		delete m_Reservoirs;
		delete m_PrevReservoirs;
		delete m_MaterialHitData;
		delete m_InstanceIDs;
		delete m_GenerateRaysPipeline;
		delete m_ExtendRaysPipeline;
		delete m_ShadeRaysPipeline;
		delete m_DirectIllumPipeline;
		delete m_ConnectRaysPipeline;
		delete m_ReprojectReSTIRPipeline;
		delete m_SpatialReSTIRPipeline;
		delete m_ShadeReSTIRPipeline;
		delete m_FinalizeRTPipeline;
		delete m_BlendingPipeline;
		delete m_TonemappingPipeline;
		delete m_OutlineObjectsPipeline;
		delete m_PreviousCamera;
		g_LineDrawer->Shutdown();
		delete g_LineDrawer;

		m_BackEndAPI->Shutdown();
		delete m_BackEndAPI;
	}

	void RenderAPI::DrawTriangleWireframeCPU()
	{
		for (auto obj : GetLevel().GetObjectManager())
		{
			auto t = ResourceManager<Model>::Get(obj->GetModelPath());
			auto modelMat = obj->GetTransform().GetModelMatrix();
			if (t.IsLoaded())
			{
				CpuPhysicsData& data = t.Get()->GetCPUPhysicsData();

				for (const auto& prim : *data.m_PrimitiveBufferGPU)
				{
					uint64_t key = static_cast<uint64_t>(prim.GetPositionIndex()) << 32 | prim.GetIndexBufferIndex();
					auto trigs = data.m_CPUTris.at(key);

					for (Triangle& tri : trigs)
					{
						glm::mat4 mat = modelMat * prim.GetMatrix();
						tri.Draw(mat);
					}
				}
			}
		}
	}

	void RenderAPI::AddScreensize(Texture* texture)
	{
		if (texture != nullptr)
		{
			ScreensizeTexturePtr newPtr;
			newPtr.m_Texture = texture;
			m_ScreensizeTextures.push_back(newPtr);
		}
	}

	void RenderAPI::AddScreensize(Buffer* buffer)
	{
		if (buffer != nullptr)
		{
			ScreensizeBufferPtr newPtr;
			newPtr.m_Buffer = buffer;
			m_ScreensizeBuffers.push_back(newPtr);
		}
	}

	void RenderAPI::RemoveScreensize(Texture* texture)
	{
		auto it =
			std::find_if(m_ScreensizeTextures.begin(),
						 m_ScreensizeTextures.end(),
						 [texture](const ScreensizeTexturePtr& sTexture) { return sTexture.m_Texture == texture; });
		if (it != m_ScreensizeTextures.end())
		{
			m_ScreensizeTextures.erase(it);
		}
	}

	void RenderAPI::RemoveScreensize(Buffer* buffer)
	{
		auto it = std::find_if(m_ScreensizeBuffers.begin(),
							   m_ScreensizeBuffers.end(),
							   [buffer](const ScreensizeBufferPtr& sBuffer) { return sBuffer.m_Buffer == buffer; });
		if (it != m_ScreensizeBuffers.end())
		{
			m_ScreensizeBuffers.erase(it);
		}
	}

	void RenderAPI::MakeScreensizeHeapLink(ScreensizeTexturePtr texture)
	{
		for (auto& sTexture : m_ScreensizeTextures)
		{
			if (sTexture.m_Texture == texture.m_Texture)
			{
				sTexture.m_Heap = texture.m_Heap;
				sTexture.m_HeapId = texture.m_HeapId;
				return;
			}
		}

		m_ScreensizeTextures.push_back(texture);
	}

	void RenderAPI::MakeScreensizeHeapLink(ScreensizeBufferPtr buffer)
	{
		for (auto& sBuffer : m_ScreensizeBuffers)
		{
			if (sBuffer.m_Buffer == buffer.m_Buffer)
			{
				sBuffer.m_Heap = buffer.m_Heap;
				sBuffer.m_HeapId = buffer.m_HeapId;
				return;
			}
		}

		m_ScreensizeBuffers.push_back(buffer);
	}

	void RenderAPI::RemoveScreensizeHeapLink(ScreensizeTexturePtr texture)
	{
		for (auto& sTexture : m_ScreensizeTextures)
		{
			if (sTexture.m_Heap == texture.m_Heap && sTexture.m_HeapId == texture.m_HeapId)
			{
				sTexture.m_Heap = nullptr;
				sTexture.m_HeapId = -1;
				return;
			}
		}
	}

	void RenderAPI::RemoveScreensizeHeapLink(ScreensizeBufferPtr buffer)
	{
		for (auto& sBuffer : m_ScreensizeBuffers)
		{
			if (sBuffer.m_Heap == buffer.m_Heap && sBuffer.m_HeapId == buffer.m_HeapId)
			{
				sBuffer.m_Heap = nullptr;
				sBuffer.m_HeapId = -1;
				return;
			}
		}
	}

	void RenderAPI::ProcessScreenshotLogic()
	{
		if (!g_PreparingScreenshotData)
			return;

		g_PreparingScreenshotData = false;
		m_ScreenShot.requested = false;

		const auto& tex = m_TransferToRTTexture;
		unsigned char* data = static_cast<unsigned char*>(tex->GetDataOnCPU());

		// Getting aligned width
		auto width = tex->GetAlignedWidth();
		auto height = tex->GetHeight();
		auto channels = tex->GetNumChannels();
		auto bytePerChannel = tex->GetBytesPerChannel();

		if (m_ScreenShot.size.x == 0 || m_ScreenShot.size.y == 0)
		{
			LOG(LOG_GRAPHICS, "Screenshot Saved : %f ", m_ScreenShot.path.c_str());
			int success = stbi_write_png(
				m_ScreenShot.path.c_str(), width, height, channels, data, width * channels * bytePerChannel);

			ASSERT(LOG_GRAPHICS, success != 0);

			tex->ReleaseDataOnCPU();
			m_ScreenShot.onComplete();

			return;
		}

		// Determine the crop area (100x100) centered in the original image
		int cropWidth = m_ScreenShot.size.x;
		int cropHeight = m_ScreenShot.size.y;
		int offsetX = (width - cropWidth) / 2;
		int offsetY = (height - cropHeight) / 2;

		// Ensure offsets are within bounds
		if (offsetX < 0 || offsetY < 0 || offsetX + cropWidth > width || offsetY + cropHeight > height)
		{
			LOG(LOG_GRAPHICS, "Error: The original image is too small for the requested crop size.");
			tex->ReleaseDataOnCPU();
			return;
		}

		// Allocate memory for the cropped image
		unsigned char* croppedData = new unsigned char[cropWidth * cropHeight * channels * bytePerChannel];

		// Copy the crop area from the original image
		for (int y = 0; y < cropHeight; ++y)
		{
			for (int x = 0; x < cropWidth; ++x)
			{
				for (int c = 0; c < channels; ++c)
				{
					croppedData[(y * cropWidth + x) * channels + c] =
						data[((y + offsetY) * width + (x + offsetX)) * channels + c];
				}
			}
		}

		// Save the cropped image as a PNG file
		int success = stbi_write_png(m_ScreenShot.path.c_str(),
									 cropWidth,
									 cropHeight,
									 channels,
									 croppedData,
									 cropWidth * channels * bytePerChannel);

		ASSERT(LOG_GRAPHICS, success != 0);

		tex->ReleaseDataOnCPU();
		delete[] croppedData;
		m_ScreenShot.onComplete();
	}

	void RenderAPI::LoadSkyboxLogic()
	{
		if (!m_UpdateNewSkyboxPath.empty())
		{
			delete m_SkyTexture;

			TextureSpec skyboxSpec;
			skyboxSpec.m_Format = TextureFormat::R32_G32_B32_A32_FLOAT;
			skyboxSpec.m_Type = TextureType::R_TEXTURE;
			skyboxSpec.m_Flags = TextureFlags::NONE;

			m_SkyTexture = new Texture(m_UpdateNewSkyboxPath, skyboxSpec, "Skybox");
			m_UpdateNewSkyboxPath.clear();
		}

		// Make sure we have a skybox texture loaded if we have none done so before
		if (m_SkyTexture == nullptr)
		{
			LoadSkybox(LaunchParameters::GetString("Skybox", "Images/HDRIs/space4.hdr"));
		}
	}

	void RenderAPI::AddBloomTexturesToRDH()
	{
		int rdh_index = RDH_OUTPUT + 1;
		auto bloompMips = m_OutputTexture->CalculateMipsNum() - 1;
		ASSERT_MSG(LOG_GRAPHICS,
				   bloompMips <= NUM_BLOOM,
				   "We don't support resolution above 4K, (a.k.a more than 13 bloom mips)");
		for (int i = 0; i < bloompMips; i++)
		{
			ASSERT(LOG_GRAPHICS, rdh_index <= RDH_HEADER_SIZE);

			m_ResourceHeap->Switch(*m_BloomIntermediateTextures[i], rdh_index); // Output Texture
			rdh_index++;
		}
	}

	void RenderAPI::LoadBlueNoiseTextures()
	{
		TextureSpec blueNoiseSpec;
		blueNoiseSpec.m_Format = TextureFormat::R32_G32_B32_A32_FLOAT;
		blueNoiseSpec.m_Type = TextureType::R_TEXTURE;
		blueNoiseSpec.m_Flags = TextureFlags::NONE;

		for (int i = 0; i < NUM_BLUENOISE; i++)
		{
			std::string path = std::string("Images/BlueNoise/product_" + std::to_string(i) + ".png");
			m_BlueNoiseTextures[i] = new Texture(path, blueNoiseSpec, std::string("BlueNoise_" + std::to_string(i)));
		}
	}

	inline TonemapParameters SetDefaultTonemapValues()
	{
		TonemapParameters tm;

		tm.m_TonemapMethod = TM_LINEAR;
		tm.m_Exposure = 0.2f;
		tm.m_Gamma = 1.9f;
		tm.m_MaxLuminance = 1.0f;
		tm.m_ReinhardConstant = 1.0f;
		tm.m_ShoulderStrength = 0.22f;
		tm.m_LinearStrength = 0.3f;
		tm.m_LinearAngle = 0.1f;
		tm.m_ToeStrength = 0.2f;
		tm.m_ToeNumerator = 0.01f;
		tm.m_ToeDenominator = 0.3f;
		tm.m_LinearWhite = 11.2f;

		return tm;
	}

	void RenderAPI::SetSelectedObject(GameObject* object)
	{
		m_SelectedObject = object;
	}

	void RenderAPI::SetHoveredObject(GameObject* object)
	{
		m_HoveredObject = object;
	}

	void RenderAPI::SetDispatchOutlineObjects(bool dispatch)
	{
		m_DispatchOutlineObjects = dispatch;
	}

} // namespace Ball