#include "Rendering/Denoiser.h"
#include "Rendering/BEAR/Buffer.h"
#include "Rendering/BEAR/Texture.h"
#include "Rendering/BEAR/CommandList.h"
#include "Rendering/BEAR/ComputePipelineDescription.h"
#include "Shaders/ShaderHeaders/CameraGPU.h"
#include <imgui.h>

#include "Rendering/BufferManager.h"
#include "Utilities/RenderUtilities.h"

using namespace Ball;

void Denoiser::Initialize(int windowWidth, int windowHeight)
{
	DenoiseBuffers type;

	uint32_t numPrimaryRays = windowWidth * windowHeight; // aka num primary rays
	size_t stride;
	size_t count = numPrimaryRays;
	BufferFlags flags = (BufferFlags::UAV | BufferFlags::ALLOW_UA | BufferFlags::DEFAULT_HEAP);
	std::string name;

	for (unsigned int i = 0; i < static_cast<int>(DenoiseBuffers::COUNT); i++)
	{
		type = static_cast<DenoiseBuffers>(i);
		bool screensize = true;
		switch (type)
		{
		case DenoiseBuffers::CURRENT_MOTION:
			stride = sizeof(float) * 2;
			name = "CURRENT_MOTION";
			break;
		case DenoiseBuffers::CURRENT_NORMAL:
			stride = sizeof(float) * 3;
			name = "CURRENT_NORMAL";
			break;
		case DenoiseBuffers::CURRENT_DEPTH:
			stride = sizeof(float);
			name = "CURRENT_DEPTH";
			break;
		case DenoiseBuffers::CURRENT_ID:
			stride = sizeof(uint32_t);
			name = "CURRENT_ID";
			break;
		case DenoiseBuffers::CURRENT_HISTORY:
			stride = sizeof(uint32_t);
			name = "CURRENT_HISTORY";
			break;
		case DenoiseBuffers::CURRENT_MOMENTS:
			stride = sizeof(float) * 2;
			name = "CURRENT_MOMENTS";
			break;
		case DenoiseBuffers::CURRENT_ILLUMINATION:
			stride = sizeof(float) * 4;
			name = "CURRENT_ILLUMINATION";
			break;
		case DenoiseBuffers::PREVIOUS_MOTION:
			stride = sizeof(float) * 2;
			name = "PREVIOUS_MOTION";
			break;
		case DenoiseBuffers::PREVIOUS_NORMAL:
			stride = sizeof(float) * 3;
			name = "PREVIOUS_NORMAL";
			break;
		case DenoiseBuffers::PREVIOUS_DEPTH:
			stride = sizeof(float);
			name = "PREVIOUS_DEPTH";
			break;
		case DenoiseBuffers::PREVIOUS_ID:
			stride = sizeof(uint32_t);
			name = "PREVIOUS_ID";
			break;
		case DenoiseBuffers::PREVIOUS_HISTORY:
			stride = sizeof(uint32_t);
			name = "PREVIOUS_HISTORY";
			break;
		case DenoiseBuffers::PREVIOUS_MOMENTS:
			stride = sizeof(float) * 2;
			name = "PREVIOUS_MOMENTS";
			break;
		case DenoiseBuffers::PREVIOUS_ILLUMINATION:
			stride = sizeof(float) * 4;
			name = "PREVIOUS_ILLUMINATION";
			break;
		case DenoiseBuffers::WORLDSPACE_INTERSECTION_POINTS:
			stride = sizeof(float) * 4;
			name = "Intersection Points";
			break;

		case DenoiseBuffers::PRIMARY_ALBEDO:
			stride = sizeof(float) * 4;
			name = "Primary Albedo";
			break;
		case DenoiseBuffers::EMISSION:
			stride = sizeof(float) * 4;
			name = "Emission";
			break;
		case DenoiseBuffers::WEIGHTED_ILLUMINATION:
			stride = sizeof(float) * 4;
			name = "Weighted Illumination";
			break;
		case DenoiseBuffers::A_TROUS_RESULT:
			stride = sizeof(float) * 4;
			name = "A-Trous Result";
			break;
		case DenoiseBuffers::VIEW_PYRAMID:
			stride = sizeof(ViewPyramid);
			count = 1;
			flags = BufferFlags::SRV | BufferFlags::UPLOAD_HEAP;
			screensize = false;
			name = "View Pyramid";
			break;
		case DenoiseBuffers::CAMERAS:
			stride = sizeof(CameraGPU);
			count = 2;
			flags = BufferFlags::SRV | BufferFlags::UPLOAD_HEAP;
			screensize = false;
			name = "Cameras";
			break;
		case DenoiseBuffers::COUNT:
			break;
		default:
			break;
		}

		if (screensize)
			m_Buffers[i] =
				BufferManager::CreateBuffer(nullptr, stride, count, (flags | BufferFlags::SCREENSIZE), name.c_str());
		else
			m_Buffers[i] = BufferManager::CreateBuffer(nullptr, stride, count, flags, name.c_str());
	}
}

Denoiser::~Denoiser()
{
	for (Buffer* buffer : m_Buffers)
	{
		BufferManager::DestroyBuffer(buffer);
	}

	delete m_CalculateWeightPipeline;
	delete m_ReprojectPipeline;
	delete m_ATrousPipeline;
	delete m_ModulatePipeline;
}

void Denoiser::GenerateReprojectPipeline()
{
	m_ReprojectPipeline = new ComputePipelineDescription();
	ShaderLayout shaderLayout;

	shaderLayout.AddParameter(ShaderParameter::SRV); // World Space Intersection Points
	shaderLayout.AddParameter(ShaderParameter::SRV); // Current Normal Buffer
	shaderLayout.AddParameter(ShaderParameter::SRV); // Previous Normal Buffer
	shaderLayout.AddParameter(ShaderParameter::SRV); // Current ID Buffer
	shaderLayout.AddParameter(ShaderParameter::SRV); // Previous ID Buffer
	shaderLayout.AddParameter(ShaderParameter::UAV); // Update History Buffer
	shaderLayout.AddParameter(ShaderParameter::SRV); // Previous History Buffer
	shaderLayout.AddParameter(ShaderParameter::UAV); // Current Moments Buffer
	shaderLayout.AddParameter(ShaderParameter::SRV); // Previous Moments Buffer
	shaderLayout.AddParameter(ShaderParameter::UAV); // Current Illumination
	shaderLayout.AddParameter(ShaderParameter::SRV); // Previous Illumination
	shaderLayout.AddParameter(ShaderParameter::SRV); // We read from A-Trous the current frame data

	shaderLayout.AddParameter(ShaderParameter::SRV); // Primary Albedo
	shaderLayout.AddParameter(ShaderParameter::SRV); // Emission
	shaderLayout.AddParameter(ShaderParameter::SRV); // Cameras Buffer

	shaderLayout.Add32bitConstParameter(3); // Reprojection Alpha, Moments Alpha, Normal Threshold
	shaderLayout.Initialize();
	m_ReprojectPipeline->Initialize("Reproject", shaderLayout);
}

void Denoiser::DispatchReproject(uint32_t numGroups1D, CommandList* cmdList, Buffer* prevReservoirs,
								 Buffer* curReservoirss) const
{
	const auto reprojectTs = Utilities::PushGPUTimestamp(cmdList, "Reproject");
	cmdList->SetComputePipeline(*m_ReprojectPipeline);

	cmdList->BindResourceSRV(0, *GetBufferAt(DenoiseBuffers::WORLDSPACE_INTERSECTION_POINTS));

	cmdList->BindResourceSRV(1, *GetBufferAt(DenoiseBuffers::CURRENT_NORMAL));
	cmdList->BindResourceSRV(2, *GetBufferAt(DenoiseBuffers::PREVIOUS_NORMAL));
	cmdList->BindResourceSRV(3, *GetBufferAt(DenoiseBuffers::CURRENT_ID));
	cmdList->BindResourceSRV(4, *GetBufferAt(DenoiseBuffers::PREVIOUS_ID));
	cmdList->BindResourceUAV(5, *GetBufferAt(DenoiseBuffers::CURRENT_HISTORY));
	cmdList->BindResourceSRV(6, *GetBufferAt(DenoiseBuffers::PREVIOUS_HISTORY));
	cmdList->BindResourceUAV(7, *GetBufferAt(DenoiseBuffers::CURRENT_MOMENTS));
	cmdList->BindResourceSRV(8, *GetBufferAt(DenoiseBuffers::PREVIOUS_MOMENTS));
	cmdList->BindResourceUAV(9, *GetBufferAt(DenoiseBuffers::CURRENT_ILLUMINATION));
	cmdList->BindResourceSRV(10, *GetBufferAt(DenoiseBuffers::PREVIOUS_ILLUMINATION));
	cmdList->BindResourceSRV(11, *GetBufferAt(DenoiseBuffers::A_TROUS_RESULT));

	cmdList->BindResourceSRV(12, *GetBufferAt(DenoiseBuffers::PRIMARY_ALBEDO));
	cmdList->BindResourceSRV(13, *GetBufferAt(DenoiseBuffers::EMISSION));
	cmdList->BindResourceSRV(14, *GetBufferAt(DenoiseBuffers::CAMERAS));

	const float settings[3] = {m_Alpha, m_MomentsAlpha, m_NormalThreshold};
	cmdList->BindResource32BitConstants(15, settings, sizeof(settings) / sizeof(float));

	cmdList->Dispatch(numGroups1D, 1, 1, true);
	Utilities::PopGPUTimestamp(cmdList, reprojectTs);
}

void Denoiser::GenerateCalculateWeightPipeline()
{
	m_CalculateWeightPipeline = new ComputePipelineDescription();
	ShaderLayout shaderLayout;
	shaderLayout.Add32bitConstParameter(sizeof(CameraGPU) / sizeof(uint32_t)); // Camera Buffer
	shaderLayout.Add32bitConstParameter(sizeof(DenoiseRenderData) / sizeof(float)); // Render Data
	shaderLayout.Add32bitConstParameter(sizeof(DenoiseDebugSettings) / sizeof(uint32_t)); // Debug settings

	shaderLayout.AddParameter(ShaderParameter::SRV); // Illumination
	shaderLayout.AddParameter(ShaderParameter::SRV); // Moments
	shaderLayout.AddParameter(ShaderParameter::SRV); // History
	shaderLayout.AddParameter(ShaderParameter::SRV); // Depth
	shaderLayout.AddParameter(ShaderParameter::SRV); // Normals

	shaderLayout.AddParameter(ShaderParameter::UAV); // Filter Result
	shaderLayout.Add32bitConstParameter(1); // History

	shaderLayout.Initialize();
	m_CalculateWeightPipeline->Initialize("CalculateWeights", shaderLayout);
}

void Denoiser::DispatchCalculateWeight(uint32_t numGroups1D, const CameraGPU& cam, CommandList* cmdList) const
{
	const auto calcWeightsTs = Utilities::PushGPUTimestamp(cmdList, "Calculate Weights");

	cmdList->SetComputePipeline(*m_CalculateWeightPipeline);

	cmdList->BindResource32BitConstants(0, &cam, sizeof(CameraGPU) / sizeof(uint32_t));
	float renderData[2] = {m_PhiNormal, m_PhiIllumination};
	cmdList->BindResource32BitConstants(1, renderData, sizeof(renderData) / sizeof(uint32_t));

	uint32_t settings[4] = {static_cast<uint32_t>(m_DebugSettings.m_DemodulateIndirectLighting),
							static_cast<uint32_t>(m_DebugSettings.m_DemodulateDirectLighting),
							static_cast<uint32_t>(m_DebugSettings.m_VisualizeVariance),
							static_cast<uint32_t>(m_DebugSettings.m_VisualizeWeights)};
	cmdList->BindResource32BitConstants(2, settings, sizeof(settings) / sizeof(uint32_t));

	cmdList->BindResourceSRV(3, *GetBufferAt(DenoiseBuffers::CURRENT_ILLUMINATION));
	cmdList->BindResourceSRV(4, *GetBufferAt(DenoiseBuffers::CURRENT_MOMENTS));
	cmdList->BindResourceSRV(5, *GetBufferAt(DenoiseBuffers::CURRENT_HISTORY));
	cmdList->BindResourceSRV(6, *GetBufferAt(DenoiseBuffers::CURRENT_DEPTH));
	cmdList->BindResourceSRV(7, *GetBufferAt(DenoiseBuffers::CURRENT_NORMAL));

	cmdList->BindResourceUAV(8, *GetBufferAt(DenoiseBuffers::WEIGHTED_ILLUMINATION));

	cmdList->BindResource32BitConstants(9, &m_History, 1);

	cmdList->Dispatch(numGroups1D, 1, 1, true);
	Utilities::PopGPUTimestamp(cmdList, calcWeightsTs);
}

void Denoiser::GenerateATrousPipeline()
{
	m_ATrousPipeline = new ComputePipelineDescription();
	ShaderLayout shaderLayout;
	shaderLayout.Add32bitConstParameter(sizeof(CameraGPU) / sizeof(uint32_t)); // Camera Buffer
	shaderLayout.Add32bitConstParameter(sizeof(DenoiseRenderData) / sizeof(float)); // Render Data

	shaderLayout.AddParameter(ShaderParameter::SRV); // History
	shaderLayout.AddParameter(ShaderParameter::SRV); // Depth
	shaderLayout.AddParameter(ShaderParameter::SRV); // Normals
	shaderLayout.AddParameter(ShaderParameter::SRV); // Weighted illumination
	shaderLayout.AddParameter(ShaderParameter::UAV); // Previous illumination

	shaderLayout.AddParameter(ShaderParameter::UAV); // Output

	shaderLayout.Add32bitConstParameter(1); // Step size

	shaderLayout.Initialize();
	m_ATrousPipeline->Initialize("ATrous", shaderLayout);
}

void Denoiser::DispatchATrous(uint32_t numGroups1D, const CameraGPU& cam, CommandList* cmdList) const
{
	const auto atroisTs = Utilities::PushGPUTimestamp(cmdList, "Atrois Dispatches");

	cmdList->SetComputePipeline(*m_ATrousPipeline);

	cmdList->BindResource32BitConstants(0, &cam, sizeof(CameraGPU) / sizeof(uint32_t));
	float renderData[2] = {m_PhiNormal, m_PhiIllumination};
	cmdList->BindResource32BitConstants(1, renderData, sizeof(renderData) / sizeof(uint32_t));

	cmdList->BindResourceSRV(2, *GetBufferAt(DenoiseBuffers::CURRENT_HISTORY));
	cmdList->BindResourceSRV(3, *GetBufferAt(DenoiseBuffers::CURRENT_DEPTH));
	cmdList->BindResourceSRV(4, *GetBufferAt(DenoiseBuffers::CURRENT_NORMAL));

	for (int i = 0; i < m_FilterIterations; i++)
	{
		if (i % 2 == 0)
		{
			cmdList->BindResourceSRV(5, *GetBufferAt(DenoiseBuffers::WEIGHTED_ILLUMINATION));
			cmdList->BindResourceUAV(7, *GetBufferAt(DenoiseBuffers::A_TROUS_RESULT));
		}
		else
		{
			cmdList->BindResourceSRV(5, *GetBufferAt(DenoiseBuffers::A_TROUS_RESULT));
			cmdList->BindResourceUAV(7, *GetBufferAt(DenoiseBuffers::WEIGHTED_ILLUMINATION));
		}
		cmdList->BindResourceUAV(6, *GetBufferAt(DenoiseBuffers::PREVIOUS_ILLUMINATION));

		int n = 1 << i;
		cmdList->BindResource32BitConstants(8, &n, 1);

		cmdList->Dispatch(numGroups1D, 1, 1, true);
	}

	Utilities::PopGPUTimestamp(cmdList, atroisTs);
}

void Denoiser::GenerateModulatePipeline()
{
	m_ModulatePipeline = new ComputePipelineDescription();
	ShaderLayout shaderLayout;
	shaderLayout.Add32bitConstParameter(sizeof(CameraGPU) / sizeof(uint32_t)); // Camera Buffer

	shaderLayout.AddParameter(ShaderParameter::UAV); // Albedo Buffer
	shaderLayout.AddParameter(ShaderParameter::SRV); // Emission
	shaderLayout.AddParameter(ShaderParameter::SRV); // Illumination

	shaderLayout.Initialize();
	m_ModulatePipeline->Initialize("Modulate", shaderLayout);
}

void Denoiser::DispatchModulate(uint32_t numGroups1D, const CameraGPU& cam, CommandList* cmdList) const
{
	const auto modulateTs = Utilities::PushGPUTimestamp(cmdList, "Modulate");

	cmdList->SetComputePipeline(*m_ModulatePipeline);

	cmdList->BindResource32BitConstants(0, &cam, sizeof(CameraGPU) / sizeof(uint32_t));

	cmdList->BindResourceUAV(1, *GetBufferAt(DenoiseBuffers::PRIMARY_ALBEDO));
	cmdList->BindResourceSRV(2, *GetBufferAt(DenoiseBuffers::EMISSION));

	if (m_FilterIterations % 2 == 0)
		cmdList->BindResourceSRV(3, *GetBufferAt(DenoiseBuffers::WEIGHTED_ILLUMINATION));
	else
		cmdList->BindResourceSRV(3, *GetBufferAt(DenoiseBuffers::A_TROUS_RESULT));

	cmdList->Dispatch(numGroups1D, 1, 1, true);
	Utilities::PopGPUTimestamp(cmdList, modulateTs);
}