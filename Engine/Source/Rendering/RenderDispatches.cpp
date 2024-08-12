#include "Rendering/Denoiser.h"
#include "Rendering/Renderer.h"
#include "Rendering/BEAR/Buffer.h"
#include "Rendering/BEAR/CommandList.h"
#include "Rendering/BEAR/ComputePipelineDescription.h"
#include "Rendering/BEAR/ShaderLayout.h"
#include "Rendering/BEAR/Texture.h"
#include "Rendering/ModelLoading/ModelManager.h"
#include "GameObjects/GameObject.h"

#include "Utilities/RenderUtilities.h"

namespace Ball
{
	ComputePipelineDescription* RenderAPI::GenerateGenerateRaysPipeline()
	{
		const auto cpd = new ComputePipelineDescription();
		ShaderLayout shaderLayout;
		shaderLayout.Add32bitConstParameter(sizeof(CameraGPU) / sizeof(uint32_t)); // Camera
		shaderLayout.Add32bitConstParameter(2); // Accum Frames ?, Frame Idx
		shaderLayout.AddParameter(ShaderParameter::UAV); // Ray Batch
		shaderLayout.AddParameter(ShaderParameter::UAV); // Ray Count - to reset
		shaderLayout.AddParameter(ShaderParameter::UAV); // Wavefront Output - to reset

		shaderLayout.AddParameter(ShaderParameter::UAV); // Current Normal Buffer - to reset
		shaderLayout.AddParameter(ShaderParameter::UAV); // Previous Normal Buffer - update with cur
		shaderLayout.AddParameter(ShaderParameter::UAV); // Current ID Buffer - to reset
		shaderLayout.AddParameter(ShaderParameter::UAV); // Previous ID Buffer - update with cur
		shaderLayout.AddParameter(ShaderParameter::UAV); // Current History Buffer - to reset
		shaderLayout.AddParameter(ShaderParameter::UAV); // Previous History Buffer - update with cur
		shaderLayout.AddParameter(ShaderParameter::UAV); // Current Moments Buffer - to reset
		shaderLayout.AddParameter(ShaderParameter::UAV); // Previous Moments Buffer - to reset

		shaderLayout.AddParameter(ShaderParameter::UAV); // Current Illumination Buffer - to reset

		shaderLayout.AddParameter(ShaderParameter::UAV); // Indirect Lighting - to reset
		shaderLayout.AddParameter(ShaderParameter::UAV); // Direct Lighting - to reset

		shaderLayout.AddParameter(ShaderParameter::UAV); // Emission Buffer - to reset
		shaderLayout.AddParameter(ShaderParameter::UAV); // Filter Luminance Buffer - to reset

		shaderLayout.Initialize();
		cpd->Initialize("Generate", shaderLayout);
		return cpd;
	}

	void RenderAPI::DispatchGeneratePrimaryRays(int numGroupsX, int numGroupsY, CameraGPU cam) const
	{
		const auto generateTs = Utilities::PushGPUTimestamp(m_CmdList, "Generate");
		m_CmdList->SetComputePipeline(*m_GenerateRaysPipeline);
		m_CmdList->SetDescriptorHeaps(m_ResourceHeap, m_SamplerHeap);

		m_CmdList->BindResource32BitConstants(0, &cam, sizeof(CameraGPU) / sizeof(uint32_t));
		uint32_t accumFrames[2] = {m_AccumFramesEnabled, m_NumTotalFrames};
		m_CmdList->BindResource32BitConstants(1, &accumFrames, sizeof(accumFrames) / sizeof(uint32_t));

		m_CmdList->BindResourceUAV(2, *m_RayBatch[0]); // We write to the first Ray Batch
		m_CmdList->BindResourceUAV(3, *m_RayCount);
		m_CmdList->BindResourceUAV(4, *m_WavefrontOutput);

		m_CmdList->BindResourceUAV(5, *m_Denoiser->GetBufferAt(DenoiseBuffers::CURRENT_NORMAL));
		m_CmdList->BindResourceUAV(6, *m_Denoiser->GetBufferAt(DenoiseBuffers::PREVIOUS_NORMAL));
		m_CmdList->BindResourceUAV(7, *m_Denoiser->GetBufferAt(DenoiseBuffers::CURRENT_ID));
		m_CmdList->BindResourceUAV(8, *m_Denoiser->GetBufferAt(DenoiseBuffers::PREVIOUS_ID));
		m_CmdList->BindResourceUAV(9, *m_Denoiser->GetBufferAt(DenoiseBuffers::CURRENT_HISTORY));
		m_CmdList->BindResourceUAV(10, *m_Denoiser->GetBufferAt(DenoiseBuffers::PREVIOUS_HISTORY));
		m_CmdList->BindResourceUAV(11, *m_Denoiser->GetBufferAt(DenoiseBuffers::CURRENT_MOMENTS));
		m_CmdList->BindResourceUAV(12, *m_Denoiser->GetBufferAt(DenoiseBuffers::PREVIOUS_MOMENTS));
		m_CmdList->BindResourceUAV(13, *m_Denoiser->GetBufferAt(DenoiseBuffers::CURRENT_ILLUMINATION));
		m_CmdList->BindResourceUAV(14, *m_Denoiser->GetBufferAt(DenoiseBuffers::EMISSION));
		m_CmdList->BindResourceUAV(15, *m_Denoiser->GetBufferAt(DenoiseBuffers::WEIGHTED_ILLUMINATION));

		m_CmdList->Dispatch(numGroupsX, numGroupsY, true);
		Utilities::PopGPUTimestamp(m_CmdList, generateTs);
	}

	ComputePipelineDescription* RenderAPI::GenerateExtendRaysPipeline()
	{
		auto cpd = new ComputePipelineDescription();
		ShaderLayout shaderLayout;
		shaderLayout.AddParameter(ShaderParameter::SRV); // TLAS
		shaderLayout.AddParameter(ShaderParameter::SRV); // Ray Batch
		shaderLayout.AddParameter(ShaderParameter::SRV); // Ray Count
		shaderLayout.AddParameter(ShaderParameter::UAV); // Extended Batch
		shaderLayout.AddParameter(ShaderParameter::UAV); // Atomic Shadow Rays - to reset
		shaderLayout.AddParameter(ShaderParameter::UAV); // Atomic New Rays - to reset
		shaderLayout.Initialize();
		cpd->Initialize("Extend", shaderLayout);
		return cpd;
	}

	void RenderAPI::DispatchExtendRays(uint32_t numGroups1D, uint32_t wavefrontBounceNum) const
	{
		const auto extendTs = Utilities::PushGPUTimestamp(m_CmdList, "Extend  " + std::to_string(wavefrontBounceNum));
		m_CmdList->SetComputePipeline(*m_ExtendRaysPipeline);
		m_CmdList->SetDescriptorHeaps(m_ResourceHeap, m_SamplerHeap);

		const auto readIndex = wavefrontBounceNum % 2;
		m_CmdList->BindResourceSRV(0, m_ModelManager->GetTLASRef());
		m_CmdList->BindResourceSRV(1, *m_RayBatch[readIndex]);
		m_CmdList->BindResourceSRV(2, *m_RayCount);
		m_CmdList->BindResourceUAV(3, *m_RayExtendBatch);
		m_CmdList->BindResourceUAV(4, *m_ShadowRaysAtomic);
		m_CmdList->BindResourceUAV(5, *m_NewRaysAtomic);

		m_CmdList->Dispatch(numGroups1D, 1, 1, true);
		Utilities::PopGPUTimestamp(m_CmdList, extendTs);
	}

	ComputePipelineDescription* RenderAPI::GenerateShadeRaysPipeline()
	{
		auto cpd = new ComputePipelineDescription();
		ShaderLayout shaderLayout;
		shaderLayout.AddParameter(ShaderParameter::UAV); // Ray Batch
		shaderLayout.AddParameter(ShaderParameter::SRV); // Ray Count
		shaderLayout.AddParameter(ShaderParameter::SRV); // Extended Batch
		shaderLayout.AddParameter(ShaderParameter::UAV); // Atomic count new rays
		shaderLayout.AddParameter(ShaderParameter::UAV); // Atomic count shadow rays
		shaderLayout.AddParameter(ShaderParameter::UAV); // New Ray Batch
		shaderLayout.Add32bitConstParameter(2); // Seeding values
		shaderLayout.AddParameter(ShaderParameter::UAV); // RT Output for writing lights

		shaderLayout.AddParameter(ShaderParameter::UAV); // World Space Intersection Points
		shaderLayout.AddParameter(ShaderParameter::UAV); // Current Depth Buffer
		shaderLayout.AddParameter(ShaderParameter::UAV); // Previous Depth Buffer
		shaderLayout.AddParameter(ShaderParameter::UAV); // Current Normal Buffer
		shaderLayout.AddParameter(ShaderParameter::UAV); // Current ID Buffer
		shaderLayout.AddParameter(ShaderParameter::UAV); // Primary Albedo
		shaderLayout.AddParameter(ShaderParameter::UAV); // Emission

		shaderLayout.AddParameter(ShaderParameter::UAV); // Instance IDs
		shaderLayout.AddParameter(ShaderParameter::UAV); // Material Hit Data

		shaderLayout.Add32bitConstParameter(sizeof(GameplaySkyMat) / sizeof(uint32_t)); // Gameplay Skybox Offset
		shaderLayout.Add32bitConstParameter(3); // Brightness threshold, Cone Spread Angle, Tracing Distance Multiplier

		shaderLayout.Initialize();
		cpd->Initialize("Shade", shaderLayout);
		return cpd;
	}

	void RenderAPI::DispatchShadeRays(uint32_t numGroups1D, uint32_t wavefrontBounceNum, GameplaySkyMat skyMat,
									  float coneSpreadAngle) const
	{
		const auto shadeTs = Utilities::PushGPUTimestamp(m_CmdList, "Shade " + std::to_string(wavefrontBounceNum));
		m_CmdList->SetComputePipeline(*m_ShadeRaysPipeline);
		m_CmdList->SetDescriptorHeaps(m_ResourceHeap, m_SamplerHeap);

		const auto readIndex = wavefrontBounceNum % 2;
		const auto writeIndex = (wavefrontBounceNum + 1) % 2;

		m_CmdList->BindResourceUAV(0, *m_RayBatch[readIndex]);
		m_CmdList->BindResourceSRV(1, *m_RayCount);
		m_CmdList->BindResourceSRV(2, *m_RayExtendBatch);
		m_CmdList->BindResourceUAV(3, *m_NewRaysAtomic);
		m_CmdList->BindResourceUAV(4, *m_ShadowRaysAtomic);
		m_CmdList->BindResourceUAV(5, *m_RayBatch[writeIndex]);

		const ShadeSeedData shadeSeeding = {m_NumTotalFrames, wavefrontBounceNum};
		m_CmdList->BindResource32BitConstants(6, &shadeSeeding, sizeof(ShadeSeedData) / sizeof(uint32_t));

		m_CmdList->BindResourceUAV(7, *m_WavefrontOutput);

		m_CmdList->BindResourceUAV(8, *m_Denoiser->GetBufferAt(DenoiseBuffers::WORLDSPACE_INTERSECTION_POINTS));
		m_CmdList->BindResourceUAV(9, *m_Denoiser->GetBufferAt(DenoiseBuffers::CURRENT_DEPTH));
		m_CmdList->BindResourceUAV(10, *m_Denoiser->GetBufferAt(DenoiseBuffers::PREVIOUS_DEPTH));
		m_CmdList->BindResourceUAV(11, *m_Denoiser->GetBufferAt(DenoiseBuffers::CURRENT_NORMAL));
		m_CmdList->BindResourceUAV(12, *m_Denoiser->GetBufferAt(DenoiseBuffers::CURRENT_ID));
		m_CmdList->BindResourceUAV(13, *m_Denoiser->GetBufferAt(DenoiseBuffers::PRIMARY_ALBEDO));
		m_CmdList->BindResourceUAV(14, *m_Denoiser->GetBufferAt(DenoiseBuffers::EMISSION));

		m_CmdList->BindResourceUAV(15, *m_InstanceIDs);
		m_CmdList->BindResourceUAV(16, *m_MaterialHitData);

		m_CmdList->BindResource32BitConstants(17, &skyMat, sizeof(skyMat) / sizeof(uint32_t));
		const ShadeSettings shadeSettings = {m_BrightnessThreshold, coneSpreadAngle, m_TracingDistanceMultiplier};
		m_CmdList->BindResource32BitConstants(18, &shadeSettings, sizeof(ShadeSettings) / sizeof(float));

		m_CmdList->Dispatch(numGroups1D, 1, 1, true);

		Utilities::PopGPUTimestamp(m_CmdList, shadeTs);
	}

	ComputePipelineDescription* RenderAPI::GenerateDirectIllumPipeline()
	{
		auto cpd = new ComputePipelineDescription();
		ShaderLayout shaderLayout;
		shaderLayout.AddParameter(ShaderParameter::SRV); // Ray Batch
		shaderLayout.AddParameter(ShaderParameter::UAV); // Atomic count shadow rays
		shaderLayout.AddParameter(ShaderParameter::UAV); // Shade Ray Batch
		shaderLayout.Add32bitConstParameter(3); // Seeding values
		shaderLayout.AddParameter(ShaderParameter::SRV); // Lighting data
		shaderLayout.AddParameter(ShaderParameter::SRV); // Material Hit Data

		shaderLayout.AddParameter(ShaderParameter::UAV); // ReSTIR Current Frame Reservoir
		shaderLayout.Add32bitConstParameter(sizeof(ReStirSettings) / sizeof(uint32_t)); // ReSTIR Settings

		shaderLayout.Initialize();
		cpd->Initialize("DirectIllumination", shaderLayout);
		return cpd;
	}

	void RenderAPI::DispatchDirectIllum(uint32_t numGroups1D, uint32_t wavefrontBounceNum) const
	{
		const auto diTs = Utilities::PushGPUTimestamp(m_CmdList, "Direct Illum " + std::to_string(wavefrontBounceNum));

		m_CmdList->SetComputePipeline(*m_DirectIllumPipeline);
		m_CmdList->SetDescriptorHeaps(m_ResourceHeap, m_SamplerHeap);

		const auto readIndex = wavefrontBounceNum % 2;
		m_CmdList->BindResourceSRV(0, *m_RayBatch[readIndex]);
		m_CmdList->BindResourceUAV(1, *m_ShadowRaysAtomic);
		m_CmdList->BindResourceUAV(2, *m_ShadowRayBatch);

		const auto lightData = m_ModelManager->GetLightData();
		const DISeedData shadeSeeding = {m_NumTotalFrames, wavefrontBounceNum, lightData->GetNumElements()};
		m_CmdList->BindResource32BitConstants(3, &shadeSeeding, sizeof(DISeedData) / sizeof(uint32_t));
		m_CmdList->BindResourceSRV(4, *lightData);
		m_CmdList->BindResourceSRV(5, *m_MaterialHitData);

		// ReSTIR:
		m_CmdList->BindResourceUAV(6, *m_Reservoirs);
		m_CmdList->BindResource32BitConstants(7, &m_ReStirSettings, sizeof(ReStirSettings) / sizeof(uint32_t));

		m_CmdList->Dispatch(numGroups1D, 1, 1, true);

		Utilities::PopGPUTimestamp(m_CmdList, diTs);
	}

	ComputePipelineDescription* RenderAPI::GenerateConnectRaysPipeline()
	{
		auto cpd = new ComputePipelineDescription();
		ShaderLayout shaderLayout;
		shaderLayout.AddParameter(ShaderParameter::SRV); // Shade Batch
		shaderLayout.AddParameter(ShaderParameter::SRV); // TLAS
		shaderLayout.AddParameter(ShaderParameter::SRV); // Atomic Shadow Rays
		shaderLayout.AddParameter(ShaderParameter::UAV); // RT Output
		shaderLayout.AddParameter(ShaderParameter::UAV); // Ray Count - to set to new rays
		shaderLayout.AddParameter(ShaderParameter::SRV); // Atomic New Rays
		shaderLayout.AddParameter(ShaderParameter::UAV); // ReSTIR Current Frame
		shaderLayout.Add32bitConstParameter(sizeof(ConnectSettings) /
											sizeof(uint32_t)); // ReSTIR mode, loop id, Threshold

		shaderLayout.Initialize();
		cpd->Initialize("Connect", shaderLayout);
		return cpd;
	}

	void RenderAPI::DispatchConnectRays(uint32_t numGroups1D, uint32_t wavefrontBounceNum) const
	{
		const auto connectTs = Utilities::PushGPUTimestamp(m_CmdList, "Connect " + std::to_string(wavefrontBounceNum));

		m_CmdList->SetComputePipeline(*m_ConnectRaysPipeline);
		m_CmdList->SetDescriptorHeaps(m_ResourceHeap, m_SamplerHeap);

		m_CmdList->BindResourceSRV(0, *m_ShadowRayBatch);
		m_CmdList->BindResourceSRV(1, m_ModelManager->GetTLASRef());
		m_CmdList->BindResourceSRV(2, *m_ShadowRaysAtomic);
		m_CmdList->BindResourceUAV(3, *m_WavefrontOutput);
		m_CmdList->BindResourceUAV(4, *m_RayCount);
		m_CmdList->BindResourceSRV(5, *m_NewRaysAtomic);
		m_CmdList->BindResourceUAV(6, *m_Reservoirs);
		ConnectSettings settings = {m_ReStirSettings.m_UseReSTIR, wavefrontBounceNum, m_BrightnessThreshold};
		m_CmdList->BindResource32BitConstants(7, &settings, sizeof(ConnectSettings) / sizeof(uint32_t));

		m_CmdList->Dispatch(numGroups1D, 1, 1, true);
		Utilities::PopGPUTimestamp(m_CmdList, connectTs);
	}

	ComputePipelineDescription* RenderAPI::GenerateReprojectReSTIRPipeline()
	{
		auto cpd = new ComputePipelineDescription();
		ShaderLayout shaderLayout;
		shaderLayout.AddParameter(ShaderParameter::SRV); // Cameras Buffer
		shaderLayout.AddParameter(ShaderParameter::UAV); // World Space Intersection Points
		shaderLayout.AddParameter(ShaderParameter::SRV); // Previous View Pyramid Buffer

		shaderLayout.AddParameter(ShaderParameter::SRV); // Current Depth Buffer
		shaderLayout.AddParameter(ShaderParameter::SRV); // Previous Depth Buffer
		shaderLayout.AddParameter(ShaderParameter::SRV); // Current Normal Buffer
		shaderLayout.AddParameter(ShaderParameter::SRV); // Previous Normal Buffer

		shaderLayout.AddParameter(ShaderParameter::UAV); // Next Frame ReSTIR Reservoirs
		shaderLayout.AddParameter(ShaderParameter::SRV); // Current Frame ReSTIR Reservoirs

		shaderLayout.Add32bitConstParameter(sizeof(ReproReSTIRSettings) /
											(sizeof(uint32_t))); // Thresholds and Seeding Vals

		shaderLayout.AddParameter(ShaderParameter::SRV); // Ray Count - to sync ToDo: remove
		shaderLayout.Initialize();
		cpd->Initialize("ReprojectReSTIR", shaderLayout);
		return cpd;
	}

	void RenderAPI::DispatchReprojectReSTIR(uint32_t numGroups1D) const
	{
		const auto reSTIRReprojectTs = Utilities::PushGPUTimestamp(m_CmdList, "ReSTIR Reproject");

		m_CmdList->SetComputePipeline(*m_ReprojectReSTIRPipeline);

		m_CmdList->BindResourceSRV(0, *m_Denoiser->GetBufferAt(DenoiseBuffers::CAMERAS));
		m_CmdList->BindResourceUAV(1, *m_Denoiser->GetBufferAt(DenoiseBuffers::WORLDSPACE_INTERSECTION_POINTS));
		m_CmdList->BindResourceSRV(2, *m_Denoiser->GetBufferAt(DenoiseBuffers::VIEW_PYRAMID));

		m_CmdList->BindResourceSRV(3, *m_Denoiser->GetBufferAt(DenoiseBuffers::CURRENT_DEPTH));
		m_CmdList->BindResourceSRV(4, *m_Denoiser->GetBufferAt(DenoiseBuffers::PREVIOUS_DEPTH));

		m_CmdList->BindResourceSRV(5, *m_Denoiser->GetBufferAt(DenoiseBuffers::CURRENT_NORMAL));
		m_CmdList->BindResourceSRV(6, *m_Denoiser->GetBufferAt(DenoiseBuffers::PREVIOUS_NORMAL));

		m_CmdList->BindResourceUAV(7, *m_Reservoirs);
		m_CmdList->BindResourceSRV(8, *m_PrevReservoirs);

		ReproReSTIRSettings settings = {m_NumTotalFrames,
										m_ReStirSettings.m_UseReSTIR,
										m_ReStirSettings.m_CurrentLightClamp,
										m_NormalThresholdReSTIR,
										m_DepthThreshold};
		m_CmdList->BindResource32BitConstants(9, &settings, sizeof(ReproReSTIRSettings) / (sizeof(uint32_t)));
		m_CmdList->BindResourceSRV(10, *m_RayCount);

		m_CmdList->Dispatch(numGroups1D, 1, 1, true);
		Utilities::PopGPUTimestamp(m_CmdList, reSTIRReprojectTs);
	}

	ComputePipelineDescription* RenderAPI::GenerateSpatialReSTIRPipeline()
	{
		auto cpd = new ComputePipelineDescription();
		ShaderLayout shaderLayout;

		shaderLayout.AddParameter(ShaderParameter::SRV); // Current Depth Buffer
		shaderLayout.AddParameter(ShaderParameter::SRV); // Current Normal Buffer

		shaderLayout.AddParameter(ShaderParameter::SRV); // Camera

		shaderLayout.AddParameter(ShaderParameter::UAV); // Previous frame ReSTIR Reservoirs
		shaderLayout.AddParameter(ShaderParameter::SRV); // Current Frame ReSTIR Reservoirs

		shaderLayout.Add32bitConstParameter(sizeof(SpatialReSTIRSettings) /
											(sizeof(uint32_t))); // Thresholds and Seeding Vals
		shaderLayout.Initialize();
		cpd->Initialize("SpatialReSTIR", shaderLayout);
		return cpd;
	}

	void RenderAPI::DispatchSpatialReSTIR(uint32_t numGroups1D) const
	{
		const auto reSTIRStatialTs = Utilities::PushGPUTimestamp(m_CmdList, "ReSTIR Spatial");

		m_CmdList->SetComputePipeline(*m_SpatialReSTIRPipeline);

		m_CmdList->BindResourceSRV(0, *m_Denoiser->GetBufferAt(DenoiseBuffers::CURRENT_DEPTH));
		m_CmdList->BindResourceSRV(1, *m_Denoiser->GetBufferAt(DenoiseBuffers::CURRENT_NORMAL));

		m_CmdList->BindResourceSRV(2, *m_Denoiser->GetBufferAt(DenoiseBuffers::CAMERAS));

		m_CmdList->BindResourceUAV(3, *m_PrevReservoirs);
		m_CmdList->BindResourceSRV(4, *m_Reservoirs);

		SpatialReSTIRSettings settings = {m_NumTotalFrames,
										  m_ReStirSettings.m_UseReSTIR,
										  m_NumSpatialSamples,
										  m_NormalThresholdReSTIR,
										  m_DepthThreshold,
										  m_SpatialRadius};
		m_CmdList->BindResource32BitConstants(5, &settings, sizeof(SpatialReSTIRSettings) / (sizeof(uint32_t)));

		m_CmdList->Dispatch(numGroups1D, 1, 1, true);
		Utilities::PopGPUTimestamp(m_CmdList, reSTIRStatialTs);
	}

	ComputePipelineDescription* RenderAPI::GenerateShadeReSTIRPipeline()
	{
		auto cpd = new ComputePipelineDescription();
		ShaderLayout shaderLayout;
		shaderLayout.AddParameter(ShaderParameter::SRV); // Ray Batch
		shaderLayout.AddParameter(ShaderParameter::SRV); // Diffuse Hit Num
		shaderLayout.Add32bitConstParameter(3); // Seeding values
		shaderLayout.AddParameter(ShaderParameter::SRV); // Lighting data
		shaderLayout.AddParameter(ShaderParameter::SRV); // Material Hit Data
		shaderLayout.AddParameter(ShaderParameter::UAV); // RT Output
		shaderLayout.Add32bitConstParameter(1); // Threshold

		shaderLayout.AddParameter(ShaderParameter::UAV); // ReSTIR Current Frame Reservoir
		shaderLayout.AddParameter(ShaderParameter::UAV); // ReSTIR Previous Frame Reservoir
		shaderLayout.Add32bitConstParameter(sizeof(ReStirSettings) / sizeof(uint32_t)); // ReSTIR Settings

		shaderLayout.Initialize();
		cpd->Initialize("ShadeReSTIR", shaderLayout);
		return cpd;
	}

	void RenderAPI::DispatchShadeReSTIR(uint32_t numGroups1D, uint32_t wavefrontBounceNum) const
	{
		const auto reSTIRFinalTs = Utilities::PushGPUTimestamp(m_CmdList, "ReSTIR Final");

		m_CmdList->SetComputePipeline(*m_ShadeReSTIRPipeline);
		m_CmdList->SetDescriptorHeaps(m_ResourceHeap, m_SamplerHeap);

		const auto readIndex = wavefrontBounceNum % 2;
		m_CmdList->BindResourceSRV(0, *m_RayBatch[readIndex]);
		m_CmdList->BindResourceSRV(1, *m_ShadowRaysAtomic);

		const auto lightData = m_ModelManager->GetLightData();
		const DISeedData shadeSeeding = {m_NumTotalFrames, wavefrontBounceNum, lightData->GetNumElements()};
		m_CmdList->BindResource32BitConstants(2, &shadeSeeding, sizeof(DISeedData) / sizeof(uint32_t));
		m_CmdList->BindResourceSRV(3, *lightData);
		m_CmdList->BindResourceSRV(4, *m_MaterialHitData);
		m_CmdList->BindResourceUAV(5, *m_WavefrontOutput);
		m_CmdList->BindResource32BitConstants(6, &m_BrightnessThreshold, 1);

		// ReSTIR:
		m_CmdList->BindResourceUAV(7, *m_Reservoirs);
		m_CmdList->BindResourceUAV(8, *m_PrevReservoirs);
		m_CmdList->BindResource32BitConstants(9, &m_ReStirSettings, sizeof(ReStirSettings) / sizeof(uint32_t));

		m_CmdList->Dispatch(numGroups1D, 1, 1, true);
		Utilities::PopGPUTimestamp(m_CmdList, reSTIRFinalTs);
	}

	ComputePipelineDescription* RenderAPI::GenerateFinalizePipeline()
	{
		auto cpd = new ComputePipelineDescription();
		ShaderLayout shaderLayout;
		shaderLayout.Add32bitConstParameter(2); // Screen Dimensions
		shaderLayout.AddParameter(ShaderParameter::SRV); // 1D color buffer
		shaderLayout.Add32bitConstParameter(2); // Accum frames?, accum frames num
		shaderLayout.AddParameter(ShaderParameter::UAV); // Current Illumination Buffer
		shaderLayout.AddParameter(ShaderParameter::UAV); // Current History Buffer

		shaderLayout.Initialize();
		cpd->Initialize("Finalize", shaderLayout);
		return cpd;
	}

	void RenderAPI::DispatchFinalize(int numGroupsX, int numGroupsY) const
	{
		const auto finalizeTs = Utilities::PushGPUTimestamp(m_CmdList, "Finalize ");

		// Binding Register Resources
		const uint32_t dimensions[2] = {static_cast<uint32_t>(m_OutputTexture->GetWidth()),
										static_cast<uint32_t>(m_OutputTexture->GetHeight())};

		const uint32_t accumFrames[2] = {static_cast<uint32_t>(m_AccumFramesEnabled), m_AccumFramesNum};

		m_CmdList->SetComputePipeline(*m_FinalizeRTPipeline);
		m_CmdList->SetDescriptorHeaps(m_ResourceHeap, m_SamplerHeap);

		m_CmdList->BindResource32BitConstants(0, &dimensions[0], sizeof(dimensions) / sizeof(uint32_t));
		m_CmdList->BindResourceSRV(1, *m_WavefrontOutput);
		m_CmdList->BindResource32BitConstants(2, &accumFrames, 2);
		m_CmdList->BindResourceUAV(3, *m_Denoiser->GetBufferAt(DenoiseBuffers::A_TROUS_RESULT));
		m_CmdList->BindResourceUAV(4, *m_Denoiser->GetBufferAt(DenoiseBuffers::CURRENT_HISTORY));

		m_CmdList->Dispatch(numGroupsX, numGroupsY, 1, true);
		Utilities::PopGPUTimestamp(m_CmdList, finalizeTs);
	}

	ComputePipelineDescription* RenderAPI::GenerateOutlineObjectsPipeline()
	{
		auto cpd = new ComputePipelineDescription();
		ShaderLayout shaderLayout;

		shaderLayout.Add32bitConstParameter(2); // Screen Dimentions
		shaderLayout.Add32bitConstParameter(
			4); // Selected Object ID, Outline Selected?, Hovered Object ID, Outline Hovered?
		shaderLayout.Add32bitConstParameter(4); // Line Properties
		shaderLayout.Add32bitConstParameter(3); // Hovered object outline color
		shaderLayout.AddParameter(ShaderParameter::SRV); // Instance IDs

		shaderLayout.Initialize();
		cpd->Initialize("OutlineObjects", shaderLayout);
		return cpd;
	}

	void RenderAPI::DispatchOutlineObjects(uint32_t numGroups1D)
	{
		// Early return in case nothing is selected/hovered to avoid an unnecessary dispatch
		if (!m_SelectedObject && !m_HoveredObject)
			return;

		auto outlineShaderTs = Utilities::PushGPUTimestamp(m_CmdList, "Outline Shader");

		const uint32_t dimensions[2] = {static_cast<uint32_t>(m_OutputTexture->GetWidth()),
										static_cast<uint32_t>(m_OutputTexture->GetHeight())};

		glm::vec4 properties = glm::vec4(m_OutlinesSelectedColor, m_LineThickness);

		uint32_t outlineSelected = 0;
		uint32_t outlineHovered = 0;

		if (m_SelectedObject)
			outlineSelected = 1;
		if (m_HoveredObject)
			outlineHovered = 1;

		uint32_t outlineInfo[4] = {m_SelectedObjectID, outlineSelected, m_HoveredObjectID, outlineHovered};

		m_CmdList->SetComputePipeline(*m_OutlineObjectsPipeline);
		m_CmdList->BindResource32BitConstants(0, &dimensions[0], sizeof(dimensions) / sizeof(uint32_t));
		m_CmdList->BindResource32BitConstants(1, &outlineInfo, 4);
		m_CmdList->BindResource32BitConstants(2, &properties, 4);
		m_CmdList->BindResource32BitConstants(3, &m_OutlinesHoveredColor, 3);
		m_CmdList->BindResourceSRV(4, *m_InstanceIDs);

		m_CmdList->Dispatch(numGroups1D, 1, 1, true);

		Utilities::PopGPUTimestamp(m_CmdList, outlineShaderTs);
	}
} // namespace Ball