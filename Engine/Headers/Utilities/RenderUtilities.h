#pragma once
#include <string>
#include <unordered_map>

#ifndef SHIPPING
#define PUSH_GPU_MARKER(cmdList, name) Ball::Utilities::PushGPUMarker(cmdList, name)
#define POP_GPU_MARKER(cmdList) Ball::Utilities::PopGPUMarker(cmdList)
#else
#define PUSH_GPU_MARKER(cmdList, name)
#define POP_GPU_MARKER(cmdList)
#endif

namespace Ball
{
	class CommandList;

	namespace Utilities
	{
		struct TimestampData
		{
			std::string name;
			float timeInMs;
		};

		enum class MarkerColors : uint32_t
		{
			RED = 0xFF0000,
			GREEN = 0x00FF00,
			PINK = 0xFFC0CB,
			BLUE = 0x0000FF,
			ORANGE = 0xFFA500,
			PURPLE = 0x800080,
			YELLOW = 0xFFFF00,
			CYAN = 0x00FFFF,
			MAGENTA = 0xFF00FF,
			LIME = 0x00FF00,
			BROWN = 0xA52A2A,
			GOLD = 0xFFD700,
			SILVER = 0xC0C0C0,
			TEAL = 0x008080,
		};

		const std::unordered_map<std::string, MarkerColors> g_MarkerColors = {
			{"Generate", MarkerColors::RED},
			{"Extend", MarkerColors::GREEN},
			{"Shade", MarkerColors::PINK},
			{"Connect", MarkerColors::BLUE},
			{"Finalize", MarkerColors::ORANGE},
			{"Copy Ray Batch", MarkerColors::PURPLE},
			{"Reproject", MarkerColors::YELLOW},
			{"CalculateWeights", MarkerColors::CYAN},
			{"ATrous", MarkerColors::MAGENTA},
			{"Modulate", MarkerColors::LIME},
			{"OutlineObjects", MarkerColors::BROWN},
			{"Blending", MarkerColors::GOLD},
			{"Copy To RenderTarget", MarkerColors::SILVER}};

		/// <summary>
		/// Sets a single GPU marker which can be seen in debugging tools like PIX or NVIDIA Nsight
		/// </summary>
		/// <param name="cmdList">Command list used for dispatching marked GPU calls</param>
		/// <param name="name">Name of the marker</param>
		void SetGPUMarker(CommandList* cmdList, const std::string& name);
		/// <summary>
		/// Pushes a GPU marker, beginning a region of GPU work which can be visualized in debugging tools like PIX or
		/// NVIDIA Nsight
		/// </summary>
		/// <param name="cmdList">Command list used for dispatching marked GPU calls</param>
		/// <param name="name">Name of the marker</param>
		void PushGPUMarker(CommandList* cmdList, const std::string& name);
		/// <summary>
		/// Pops the GPU marker, ending a region of GPU work which can be visualized in debugging tools like PIX or
		/// NVIDIA Nsight
		/// </summary>
		/// <param name="cmdList">Command list used for dispatching marked GPU calls</param>
		void PopGPUMarker(CommandList* cmdList);

		// Returns the index of the timestamp query
		uint32_t PushGPUTimestamp(CommandList* cmdList, const std::string& name);

		// startIndex refers to the index from `PushGPUTimestamp` to calculate the end product
		void PopGPUTimestamp(CommandList* cmdList, const uint32_t startIndex);

		void SaveGPUTimestampData(CommandList* cmdList);

		std::vector<TimestampData> ProcessReadbackBuffer();

	} // namespace Utilities
} // namespace Ball