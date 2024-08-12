#pragma once
#include <vector>
#include "ShaderHeaders/CameraGPU.h"
#include "ShaderHeaders/DenoisingStructs.h"

namespace Ball
{
	class Buffer;
	class CommandList;
	class ComputePipelineDescription;

	enum class DenoiseBuffers
	{
		CURRENT_NORMAL, // Object space normals; ToDo: researh before or after normal map?
		CURRENT_DEPTH,
		CURRENT_MOTION,
		CURRENT_ID, // Model ID and primitive ID; ToDo: research do we need instance id or not?
		CURRENT_HISTORY, // Update for the history buffer
		CURRENT_MOMENTS,
		CURRENT_ILLUMINATION,

		PREVIOUS_NORMAL,
		PREVIOUS_DEPTH,
		PREVIOUS_MOTION,
		PREVIOUS_ID,
		PREVIOUS_HISTORY, // How many frames the data has been valid for
		PREVIOUS_MOMENTS,
		PREVIOUS_ILLUMINATION,

		WORLDSPACE_INTERSECTION_POINTS,
		PRIMARY_ALBEDO, // Albedo at primary intersection point
		EMISSION, // For demodulation
		WEIGHTED_ILLUMINATION, // Weighted illumination + weighted variance in alpha channel
		A_TROUS_RESULT,
		VIEW_PYRAMID, // Previous frame's view pyramid
		CAMERAS, // Previous and current camera

		COUNT
	};

	class Denoiser
	{
	public:
		void Initialize(int windowWidth, int windowHeight);
		~Denoiser();

		void GenerateReprojectPipeline();
		void DispatchReproject(uint32_t numGroups1D, CommandList* cmdList, Buffer* prevReservoirs,
							   Buffer* curReservoirss) const;

		void GenerateCalculateWeightPipeline();
		void DispatchCalculateWeight(uint32_t numGroups1D, const CameraGPU& cam, CommandList* cmdList) const;

		void GenerateATrousPipeline();
		void DispatchATrous(uint32_t numGroups1D, const CameraGPU& cam, CommandList* cmdList) const;

		void GenerateModulatePipeline();
		void DispatchModulate(uint32_t numGroups1D, const CameraGPU& cam, CommandList* cmdList) const;

		Buffer* GetBufferAt(DenoiseBuffers index) const { return m_Buffers[static_cast<uint32_t>(index)]; }

		struct DebugSettings
		{
			bool m_DemodulateIndirectLighting = false;
			bool m_DemodulateDirectLighting = false;
			bool m_VisualizeVariance = false;
			bool m_VisualizeWeights = false;
		} m_DebugSettings;

		float m_Alpha = 0.05f;
		float m_MomentsAlpha = 0.20f;
		float m_NormalThreshold = 0.4f;
		float m_PhiNormal = 128.0f;
		float m_PhiIllumination = 4.0f;
		int m_FilterIterations = 5;
		int m_History = 4;
		bool m_DemodulateIndirectLighting = false;
		bool m_DemodulateDirectLighting = false;

	private:
		Buffer* m_Buffers[static_cast<unsigned int>(DenoiseBuffers::COUNT)];

		ComputePipelineDescription* m_CalculateWeightPipeline = nullptr;
		ComputePipelineDescription* m_ReprojectPipeline = nullptr;
		ComputePipelineDescription* m_ATrousPipeline = nullptr;
		ComputePipelineDescription* m_ModulatePipeline = nullptr;
	};
} // namespace Ball